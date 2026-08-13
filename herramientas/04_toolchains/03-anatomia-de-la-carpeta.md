# Adentro de la carpeta del toolchain

En la [página 01](./01-que-es-un-toolchain.md) vimos qué programas forman un toolchain. Acá abrimos
la carpeta real y vemos **qué es cada archivo, quién lo llama y cuándo**. Usamos como ejemplo la
copia que trae MCUXpresso IDE. Otras distribuciones de GCC conservan las funciones y directorios
principales, pero pueden cambiar versiones, nombres, multilibs y componentes incluidos.

## Dónde está

En Linux, MCUXpresso instala el toolchain en:

```
/usr/local/mcuxpressoide-<version>/ide/tools/
```

Ese `ide/tools` es en realidad un **enlace simbólico** a un plugin de Eclipse:

```
ide/tools -> plugins/com.nxp.mcuxpresso.tools.linux_11.10.0.202311280810/tools
```

MCUXpresso se basa en Eclipse y distribuye varias herramientas dentro de plugins. Al actualizar el
IDE también puede cambiar la versión del toolchain, así que conviene registrar `gcc --version` en
los builds que necesiten ser reproducibles.

Para saber cuál es la tuya y qué versión es:

```bash
ls -d /usr/local/mcuxpressoide-*/ide/tools
/usr/local/mcuxpressoide-*/ide/tools/bin/arm-none-eabi-gcc --version
```

En la instalación usada para preparar esta guía devuelve `Arm GNU Toolchain 13.2.rel1 (Build
arm-13.7)`, GCC 13.2.1. NXP integró una distribución basada en Arm GNU Toolchain y agregó
componentes propios, que aparecen más abajo. Otra versión de MCUXpresso puede traer cifras y rutas
distintas. El archivo `13.2.Rel1-x86_64-arm-none-eabi-manifest.txt` en esa carpeta lo prueba:
tiene los `configure` originales del build de Arm.

## El mapa de la carpeta

```
ide/tools/
├── bin/                  416 MB   los programas que vos invocás
├── arm-none-eabi/        710 MB   el "sysroot": headers y librerías DEL MICRO
├── lib/                   81 MB   librerías internas de gcc (libgcc)
├── libexec/              136 MB   los programas que gcc invoca por dentro
├── include/                       headers de la API de gdb (para plugins)
├── redlib/               148 KB   headers de Redlib (agregado de NXP)
├── features/              56 KB   headers propietarios de NXP (CRP, MTB, secciones)
├── share/                 50 MB   documentación, manpages, scripts de gdb
└── licenses/                      las licencias de todo lo anterior
```

En esta versión, el total ronda **1,4 GB**. El desglose sirve para entender qué ocupa espacio y qué
recortó el paquete del repositorio; no es una garantía para otras versiones.

## `bin/`: lo que vos escribís en la terminal

Son los ejecutables **de tu PC** (x86-64) que producen código **para ARM**. Ese es el sentido de
"compilación cruzada". Todos llevan el prefijo `arm-none-eabi-`:

| Archivo | Qué es | Quién lo llama |
|---------|--------|----------------|
| `arm-none-eabi-gcc` | el **driver**: coordina preprocesador, compilador, ensamblador y linker | vos, o el `make` |
| `arm-none-eabi-g++` / `c++` | lo mismo para C++ | vos |
| `arm-none-eabi-cpp` | el preprocesador, invocable suelto (`gcc -E` hace lo mismo) | rara vez |
| `arm-none-eabi-as` | **ensamblador**: texto en assembler a `.o` | `gcc`, no vos |
| `arm-none-eabi-ld` / `ld.bfd` | **linker**: junta `.o`, aplica el linker script, produce el `.elf` | `gcc`, no vos |
| `arm-none-eabi-ar` | **archivador**: mete varios `.o` en un `.a` (una "librería estática") | el build, al armar `libCMSISv2p00_LPC17xx.a` |
| `arm-none-eabi-ranlib` | genera el índice de símbolos de un `.a` | `ar -s` lo hace solo |
| `arm-none-eabi-objcopy` | convierte formatos: `.elf` a `.bin` o `.hex` | vos, al final del build |
| `arm-none-eabi-objdump` | desensambla y vuelca secciones | vos, para investigar |
| `arm-none-eabi-nm` | lista símbolos (funciones, variables) | vos, para investigar |
| `arm-none-eabi-size` | tamaño de `.text` / `.data` / `.bss` | el post-build de MCUXpresso |
| `arm-none-eabi-readelf` | estructura interna del ELF (secciones, headers, segmentos) | vos |
| `arm-none-eabi-strip` | borra la info de debug de un binario | releases |
| `arm-none-eabi-addr2line` | dada una dirección, te dice archivo y línea | análisis de un HardFault |
| `arm-none-eabi-gdb` | el depurador | el IDE, o vos en la terminal |
| `arm-none-eabi-gcov` | cobertura de código | rara vez en embebidos |
| `arm-none-eabi-gfortran` | compilador Fortran | nadie, en este contexto |

De los 416 MB de `bin/`, **173 MB son `arm-none-eabi-gdb`**. Los binarios vienen con toda su
información de depuración adentro; si los pasás por `strip`, gdb baja de 173 MB a 12 MB. Es la razón
principal de que el toolchain completo sea tan pesado.

> `gcc` no es "el compilador". Es un **director de orquesta**: mira la extensión de cada archivo y
> las banderas, y decide a quién llamar. El compilador de verdad está en `libexec/`.

Conviene distinguir varios proyectos que se distribuyen juntos: **GCC** aporta los drivers,
front-ends, runtimes y herramientas como `gcov`; **binutils** aporta `as`, `ld`, `ar`, `objcopy`,
`objdump`, `nm`, `size`, `readelf` y `strip`; **GDB** es el depurador. `gfortran` es otro
front-end de GCC.

## `libexec/`: los programas que no ves nunca

```
libexec/gcc/arm-none-eabi/13.2.1/
├── cc1              32.7 MB   el compilador de C de verdad
├── cc1plus          35.0 MB   el compilador de C++
├── f951             33.5 MB   el compilador de Fortran
├── collect2          1.0 MB   envoltorio del linker
├── lto1             31.3 MB   compilador para Link Time Optimization
├── lto-wrapper       1.6 MB   coordina LTO durante el link
└── liblto_plugin.so           plugin de LTO para el linker
```

Están en `libexec` justamente porque **no son para el usuario**: los llama `gcc`. Podés verlo con
`-v`:

```bash
arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -c main.c -v
```

En la salida aparece la cadena real:

```
.../libexec/gcc/arm-none-eabi/13.2.1/cc1 -quiet -v -imultilib thumb/v7-m/nofp \
    -isysroot .../arm-none-eabi -D__USES_INITFINI__ main.c -o /tmp/ccACr1J0.s
.../arm-none-eabi/bin/as -march=armv7-m -mfloat-abi=soft -meabi=5 -o main.o /tmp/ccACr1J0.s
```

Ahí se ve todo: `cc1` toma tu `.c` y escribe **assembler** en un archivo temporal, y después `as`
lo convierte en `.o`. El `.s` intermedio se borra (con `-save-temps` lo conservás y lo podés leer).

Fijate también en `-imultilib thumb/v7-m/nofp`: `gcc` ya tradujo tu `-mcpu=cortex-m3` a "esta
variante de librerías". Es la clave de la sección siguiente.

## `arm-none-eabi/`: el sysroot, y por qué pesa 710 MB

Esta carpeta es el mundo **del micro**, no el de tu PC. Tiene tres partes:

```
arm-none-eabi/
├── bin/       14 MB    copias de as, ld, ar, objcopy... que invoca gcc internamente
├── include/   23 MB    los headers de la libc (stdio.h, string.h, stdint.h...)
└── lib/      670 MB    las librerías compiladas: libc.a, libm.a, libnosys.a...
```

### Por qué hay dos copias de `as` y `ld`

En `bin/arm-none-eabi-as` (con prefijo) y en `arm-none-eabi/bin/as` (sin prefijo). La primera es
para que vos la llames desde la terminal; la segunda es la que **encuentra gcc** en su búsqueda
interna. Es parte de la disposición que espera esta distribución. Borrar una de las copias puede romper la
búsqueda interna, aunque el efecto exacto depende de cómo se configuró GCC.

### El multilib: la razón de los 670 MB

En esta distribución, `lib/` contiene **39 entradas multilib** para distintas combinaciones de
arquitectura, repertorio de instrucciones y ABI de punto flotante:

```bash
arm-none-eabi-gcc -print-multi-lib | wc -l     # 39
ls arm-none-eabi/lib/thumb/
# nofp  v6-m  v7  v7-a  v7-a+fp  v7-a+simd  v7e-m  v7e-m+dp  v7e-m+fp  v7+fp  ...
```

Esto se llama **multilib**. La razón es que un `libc.a` compilado con instrucciones de FPU no corre
en un micro sin FPU, y uno compilado para ARMv6-M no aprovecha un ARMv7-M. Entonces el toolchain trae
todas las variantes precompiladas y elige la que corresponde según tus banderas.

Para el LPC1769 (Cortex-M3, ARMv7-M, sin FPU) la variante es:

```bash
arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -print-multi-directory
# thumb/v7-m/nofp
```

Esa sola carpeta pesa 18 MB. Las demás no participan del build del LPC1769. `tools/pack_toolchain.sh` conserva solo las
variantes necesarias para este repositorio y reduce el paquete de aproximadamente 1,4 GB a 170 MB
en la versión medida.

> Si trabajás con otra placa el directorio cambia solo: un STM32F4 (Cortex-M4F) da
> `thumb/v7e-m+fp/hard`, un RP2040 (Cortex-M0+) da `thumb/v6-m/nofp`. El mecanismo es idéntico.

### Qué hay adentro del multilib del Cortex-M3

```bash
ls arm-none-eabi/lib/thumb/v7-m/nofp/
```

| Archivo | Qué es |
|---------|--------|
| `libc.a` | **newlib**: la libc completa (`printf`, `malloc`, `strcpy`, `memcpy`...) |
| `libc_nano.a` | **newlib-nano**: variante orientada a menor tamaño; algunas funciones y opciones, como formato de punto flotante, se habilitan aparte |
| `libm.a` | funciones matemáticas (`sin`, `sqrt`, `pow`) |
| `libg.a` / `libg_nano.a` | variantes de `libc` con más info de debug |
| `libnosys.a` | **stubs vacíos** de las syscalls (`_write`, `_read`, `_sbrk`...) que newlib espera de un SO |
| `librdimon.a` | igual que `libnosys` pero las syscalls van por **semihosting** al depurador |
| `libstdc++.a` / `libsupc++.a` | la librería estándar de C++ |
| `crt0.o` | el arranque genérico de C (en bare-metal casi siempre lo reemplaza tu `startup.c`) |
| `*.specs` | archivos de configuración del linker (los vemos ahora) |

El detalle importante en bare metal es que newlib puede formatear un `printf`, pero no conoce el
dispositivo de salida de tu placa. Para escribir llama a `_write()`. Alguien debe proveer esa capa:
`libnosys.a` aporta stubs que normalmente informan que la operación no está implementada,
`librdimon.a` usa semihosting, o tu proyecto redirige la llamada a UART o RTT, como en el
[módulo 9 del curso](../../curso/09_uart/)).

### Los archivos `.specs`

Un `.specs` es un archivito de texto que le cambia a `gcc` los comandos que arma. Es cómo se
seleccionan las variantes de librería:

```bash
arm-none-eabi-gcc ... -specs=nano.specs -specs=nosys.specs -o app.elf
```

- `nano.specs`: usá `libc_nano.a` en vez de `libc.a`. Ahorra varios kB.
- `nosys.specs`: agrega `libnosys.a`, con stubs mínimos para las llamadas que espera newlib.
  Sin una implementación equivalente pueden aparecer referencias sin resolver como `_write`.
- `rdimon.specs`: en vez de vacías, semihosting.

Están en `arm-none-eabi/lib/` (los generales) y repetidos en cada multilib. Si te falta el archivo,
gcc corta con `cannot read spec file 'nano.specs'`.

## `lib/`: las librerías internas de gcc

```
lib/gcc/arm-none-eabi/13.2.1/
├── include/            headers que provee el compilador (stdint.h, stdbool.h, stdarg.h)
├── include-fixed/      headers del sistema "parchados" por gcc
├── crtbegin.o crtend.o crti.o crtn.o    arranque de C++ (constructores globales)
├── libgcc.a            rutinas de soporte del compilador
└── thumb/v7-m/nofp/    la copia de libgcc.a para Cortex-M3
```

Dos cosas que conviene distinguir:

**Los headers del compilador y de la libc cooperan.** GCC incluye wrappers y headers propios en
`lib/gcc/.../include`, mientras newlib aporta los del sysroot. Según la distribución, `stdint.h`
puede pasar de una capa a la otra para obtener tipos coherentes con el target.

**`libgcc.a` no es la libc.** Es el "pegamento" del compilador: rutinas que gcc necesita cuando el
procesador no tiene una instrucción para algo. El Cortex-M3 no tiene instrucción de división de
enteros de 64 bits ni de punto flotante, así que si escribís `a / b` con `int64_t`, gcc emite una
llamada a `__aeabi_ldivmod`, que vive en `libgcc.a`. El driver la agrega normalmente. Con
`-nostdlib` o `-nodefaultlibs` puede ser necesario linkear `-lgcc` de forma explícita.

## `redlib/` y `features/`: agregados de NXP

Estas carpetas contienen componentes que no forman parte de la distribución estándar de Arm.

**Redlib** es una implementación alternativa de la libc, hecha por Code Red (la empresa que NXP
compró y de donde salió MCUXpresso). Es más chica que newlib porque no intenta ser POSIX: sirve para
bare-metal y nada más. En `redlib/include/` están sus headers (`stdio.h`, `string.h`, etc., versión
Redlib) y las librerías compiladas son los `libcr_*.a` que viste en el multilib:

| Librería | Variante |
|----------|----------|
| `libcr_c.a` | el núcleo de Redlib |
| `libcr_nohost.a` | Redlib sin E/S (el `printf` no va a ningún lado) |
| `libcr_semihost.a` | Redlib con E/S por semihosting |
| `libcr_newlib_*.a` | capas de compatibilidad para usar newlib con el runtime de Code Red |

Se activa con `-specs=redlib.specs` y el define `__REDLIB__`. Si compilaste la librería CMSIS de
este repo desde MCUXpresso, en el comando de compilación aparece exactamente eso:

```
arm-none-eabi-gcc -D__REDLIB__ -DDEBUG -D__CODE_RED ... -specs=redlib.specs ...
```

**`features/include/`** tiene headers propietarios chicos pero muy usados en proyectos LPC:

| Header | Para qué |
|--------|----------|
| `cr_section_macros.h` | macros `__DATA(RAM2)`, `__BSS(RAM2)`, `__NOINIT` para poner variables en un banco de RAM específico |
| `NXP/crp.h` | la macro `__CRP` para el *Code Read Protect* del LPC (protección de lectura de la Flash) |
| `cr_mtb_buffer.h` | buffer del *Micro Trace Buffer* (trace en Cortex-M0+) |

> Si un proyecto usa Redlib o estos headers, no compilará sin cambios con una distribución que no
> los incluya. Es una de las dependencias posibles de MCUXpresso y se resuelve reemplazando
> `-specs=redlib.specs` por `-specs=nano.specs -specs=nosys.specs` y las macros de sección por
> atributos estándar de gcc (`__attribute__((section(".data.$RAM2")))`).

## `share/`, `include/`, `licenses/`

- `share/`: 33 MB de documentación (`share/doc`), 13 MB de manuales info y 3.9 MB de manpages, más
  `share/gdb/python` con los scripts de *pretty printing* que usa gdb. Nada de esto hace falta para
  compilar.
- `include/gdb/`: headers para escribir plugins de gdb. No lo usa casi nadie.
- `licenses/`: las licencias. GCC, binutils y newlib son software libre (GPL con excepción de
  runtime, y licencias BSD). Redlib y los headers de `features/` son de NXP y están cubiertos por el
  EULA de MCUXpresso, que **no** permite redistribuirlos. Por eso el paquete que arma
  `tools/pack_toolchain.sh` los excluye.

## Cómo encuentra gcc cada pieza

Todo lo anterior funciona porque gcc calcula las rutas **relativas a su propio ejecutable**. Podés
verlo:

```bash
arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -print-search-dirs
```

```
install:  .../tools/bin/../lib/gcc/arm-none-eabi/13.2.1/
programs: .../tools/bin/../libexec/gcc/arm-none-eabi/13.2.1/
          .../tools/bin/../arm-none-eabi/bin/
libraries: .../tools/bin/../lib/gcc/arm-none-eabi/13.2.1/thumb/v7-m/nofp/
           .../tools/bin/../arm-none-eabi/lib/thumb/v7-m/nofp/
```

Notá el patrón `bin/../`: gcc parte de dónde está él mismo y sube. La consecuencia práctica es
útil: las distribuciones binarias de Arm GNU Toolchain están preparadas para ser reubicables. Podés
descomprimirlas en otro directorio si conservás la estructura interna. Wrappers o agregados de un
fabricante sí pueden introducir dependencias externas, por lo que conviene ejecutar una compilación
de prueba después de moverlas.

Es exactamente lo que aprovecha `tools/install_toolchain.sh` para dejarlo en `tools/toolchain/` sin
tocar el sistema ni pedir `sudo`.

## Lo mínimo que necesitás

Si tuvieras que armar el paquete a mano, para compilar y linkear un Cortex-M3 alcanza con:

| Pieza | Por qué |
|-------|---------|
| `bin/arm-none-eabi-{gcc,as,ld,ar,objcopy,size}` | la cadena mínima de build |
| `arm-none-eabi/bin/{as,ld}` | los que busca gcc por dentro |
| `libexec/gcc/.../cc1` | el compilador de C |
| `lib/gcc/.../{include,include-fixed,libgcc.a}` | `stdint.h` y el pegamento |
| `lib/gcc/.../thumb/v7-m/nofp/libgcc.a` | el pegamento, variante M3 |
| `arm-none-eabi/include/` | los headers de newlib |
| `arm-none-eabi/lib/{ldscripts,*.specs}` | scripts base del linker y los `.specs` |
| `arm-none-eabi/lib/thumb/v7-m/nofp/` | `libc`, `libm`, `libnosys` para M3 |

En la versión usada, ese conjunto da unos 170 MB y 35 MB comprimido. Es lo que conserva
`tools/pack_toolchain.sh`, que además ejecuta una compilación de prueba antes de empaquetar. Y si querés agregar C++, sumás
`cc1plus`, `g++` y `libstdc++.a`.

El resultado está versionado en este repo, en `tools/toolchain-pkg/`, así que un `git clone` y un
`bash tools/install_toolchain.sh` alcanzan para empezar a compilar sin bajar nada más.

Para depurar hace falta GDB. En la instalación probada, el binario incluido por MCUXpresso depende
de `libncursesw.so.5` y `libtinfo.so.5`, bibliotecas ausentes en Ubuntu recientes. Si ocurre ese
error, usá `gdb-multiarch` del sistema o una distribución autocontenida como xPack
(`bash tools/install_toolchain.sh --xpack`), que trae un gdb autocontenido.

Qué hace MCUXpresso con todas estas piezas cuando apretás Build y Debug está en
[07 - MCUXpresso por dentro](../07_lpc1769/07-mcuxpresso-por-dentro.md).

---

**Toolchains:** [índice](./README.md) ·
**Anterior:** [02 - El target triplet](./02-el-triplet.md) ·
**Siguiente:** [04 - Otros toolchains](./04-otros-toolchains.md)
