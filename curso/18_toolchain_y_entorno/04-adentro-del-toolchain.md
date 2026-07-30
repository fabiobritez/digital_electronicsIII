# Adentro de la carpeta del toolchain

En la [página 01](./01-que-es-un-toolchain.md) vimos qué programas forman un toolchain. Acá abrimos
la carpeta real y vemos **qué es cada archivo, quién lo llama y cuándo**. Usamos como ejemplo la
copia que trae MCUXpresso IDE, porque es la que probablemente ya tenés instalada, pero la estructura
es la misma en cualquier toolchain GCC: la que baja `tools/install_toolchain.sh`, la de xPack, la de
`apt`, o la que uses para un STM32 o un RP2040.

## Dónde está

En Linux, MCUXpresso instala el toolchain en:

```
/usr/local/mcuxpressoide-<version>/ide/tools/
```

Ese `ide/tools` es en realidad un **enlace simbólico** a un plugin de Eclipse:

```
ide/tools -> plugins/com.nxp.mcuxpresso.tools.linux_11.10.0.202311280810/tools
```

No es un detalle menor: MCUXpresso es Eclipse, y **todo** lo que no es Java lo empaqueta como
plugins. El compilador es un plugin, los binarios de debug son otro. Por eso al actualizar el IDE
puede cambiarte la versión del compilador sin que te enteres.

Para saber cuál es la tuya y qué versión es:

```bash
ls -d /usr/local/mcuxpressoide-*/ide/tools
/usr/local/mcuxpressoide-*/ide/tools/bin/arm-none-eabi-gcc --version
```

En esta instalación devuelve `Arm GNU Toolchain 13.2.rel1 (Build arm-13.7)`, gcc 13.2.1. Eso confirma
algo importante: **no es un compilador de NXP**. Es la *Arm GNU Toolchain* que publica Arm
gratuitamente, la misma que podés bajar de la web de Arm, con unos agregados de NXP encima (los
vemos más abajo). El archivo `13.2.Rel1-x86_64-arm-none-eabi-manifest.txt` en esa carpeta lo prueba:
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

Total: **1.4 GB**. Vale la pena ver por qué es tan grande, porque explica qué se puede tirar.

## `bin/`: lo que vos escribís en la terminal

Son los ejecutables **de tu PC** (x86-64) que producen código **para ARM**. Ese es el sentido de
"compilación cruzada". Todos llevan el prefijo `arm-none-eabi-`:

| Archivo | Qué es | Quién lo llama |
|---------|--------|----------------|
| `arm-none-eabi-gcc` | el **driver**: no compila nada, decide qué programas correr y en qué orden | vos, o el `make` |
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

Hay dos grupos acá que conviene distinguir: **GCC** (el compilador propiamente dicho: `gcc`, `g++`,
`cpp`, `gcov`) y **binutils** (todo el resto: `as`, `ld`, `ar`, `objcopy`, `objdump`, `nm`, `size`,
`readelf`, `strip`). Son dos proyectos separados del mundo GNU que se distribuyen juntos.

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
interna. Es una convención vieja de GCC y no hay que tocarlo: si borrás `arm-none-eabi/bin/`, gcc
deja de compilar.

### El multilib: la razón de los 670 MB

`lib/` no tiene *una* copia de la librería estándar: tiene **39**. Una por cada combinación de
arquitectura, set de instrucciones y unidad de punto flotante que soporta ARM:

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

Esa sola carpeta pesa 18 MB. Las otras 38 no se usan nunca en este curso. Es exactamente lo que
recorta `tools/pack_toolchain.sh` para bajar de 1.4 GB a 170 MB.

> Si trabajás con otra placa el directorio cambia solo: un STM32F4 (Cortex-M4F) da
> `thumb/v7e-m+fp/hard`, un RP2040 (Cortex-M0+) da `thumb/v6-m/nofp`. El mecanismo es idéntico.

### Qué hay adentro del multilib del Cortex-M3

```bash
ls arm-none-eabi/lib/thumb/v7-m/nofp/
```

| Archivo | Qué es |
|---------|--------|
| `libc.a` | **newlib**: la libc completa (`printf`, `malloc`, `strcpy`, `memcpy`...) |
| `libc_nano.a` | **newlib-nano**: la misma libc, recortada para micros (printf sin float, malloc simple) |
| `libm.a` | funciones matemáticas (`sin`, `sqrt`, `pow`) |
| `libg.a` / `libg_nano.a` | variantes de `libc` con más info de debug |
| `libnosys.a` | **stubs vacíos** de las syscalls (`_write`, `_read`, `_sbrk`...) que newlib espera de un SO |
| `librdimon.a` | igual que `libnosys` pero las syscalls van por **semihosting** al depurador |
| `libstdc++.a` / `libsupc++.a` | la librería estándar de C++ |
| `crt0.o` | el arranque genérico de C (en bare-metal casi siempre lo reemplaza tu `startup.c`) |
| `*.specs` | archivos de configuración del linker (los vemos ahora) |

El detalle importante de bare-metal: **newlib no sabe qué es un `printf`**. Sabe formatear el texto,
pero para sacarlo llama a `_write()`, que en un sistema operativo sería una syscall. En un micro no
hay SO, así que alguien tiene que proveer ese `_write`: o `libnosys.a` (no hace nada), o
`librdimon.a` (lo manda al depurador por semihosting), o vos (lo mandás a la UART, como en el
módulo 9).

### Los archivos `.specs`

Un `.specs` es un archivito de texto que le cambia a `gcc` los comandos que arma. Es cómo se
seleccionan las variantes de librería:

```bash
arm-none-eabi-gcc ... -specs=nano.specs -specs=nosys.specs -o app.elf
```

- `nano.specs`: usá `libc_nano.a` en vez de `libc.a`. Ahorra varios kB.
- `nosys.specs`: linkeá `libnosys.a`, o sea, las syscalls vacías. Sin esto el linker te tira
  `undefined reference to _write`.
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

Dos cosas que sorprenden:

**`stdint.h` no viene de newlib, viene de gcc.** Los tipos `uint32_t`, `int8_t` dependen de cómo el
compilador representa los enteros, así que el compilador es quien los define. Por eso están acá y no
en el sysroot.

**`libgcc.a` no es la libc.** Es el "pegamento" del compilador: rutinas que gcc necesita cuando el
procesador no tiene una instrucción para algo. El Cortex-M3 no tiene instrucción de división de
enteros de 64 bits ni de punto flotante, así que si escribís `a / b` con `int64_t`, gcc emite una
llamada a `__aeabi_ldivmod`, que vive en `libgcc.a`. Se linkea **siempre**, aunque uses
`-nostdlib`.

## `redlib/` y `features/`: lo que sí puso NXP

Acá está lo único que no es de Arm.

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

> Estos tres archivos, más Redlib, son la razón por la que un proyecto hecho en MCUXpresso **no
> compila tal cual** en otro toolchain. Es la única atadura real al IDE, y se resuelve reemplazando
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
enorme: **el toolchain es reubicable**. Podés moverlo, copiarlo a otra máquina o descomprimirlo en
tu `$HOME` y funciona igual, siempre que la estructura interna `bin/`, `lib/`, `libexec/`,
`arm-none-eabi/` se mantenga. No hay rutas absolutas ni variables de entorno obligatorias.

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

Eso da unos 170 MB, 35 MB comprimido. Es literalmente lo que hace `tools/pack_toolchain.sh`, que
además corre una prueba de compilación real antes de empaquetar. Y si querés agregar C++, sumás
`cc1plus`, `g++` y `libstdc++.a`.

Para depurar hace falta `gdb`, pero **no** conviene sacarlo de MCUXpresso: la build de NXP está
linkeada contra `libncursesw.so.5` y `libtinfo.so.5`, que ya no existen en Ubuntu moderno, y no
arranca. Usá `sudo apt install gdb-multiarch`, o el toolchain de xPack
(`bash tools/install_toolchain.sh --xpack`), que trae un gdb autocontenido.

En la [próxima página](./05-como-compila-y-graba-mcuxpresso.md) vemos qué hace MCUXpresso con todas
estas piezas cuando apretás Build y Debug.

---

**Anterior:** [03 - Quemar la placa](./03-quemar-la-placa.md) ·
**Siguiente:** [05 - Cómo compila y graba MCUXpresso](./05-como-compila-y-graba-mcuxpresso.md) ·
**Módulo:** [Toolchain y entorno](./README.md)
