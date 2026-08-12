# MCUXpresso por dentro: cómo compila y cómo graba

Esta página desarma un build y una sesión de programación para mostrar qué coordina el IDE. El caso
corresponde a un proyecto LPC1769 generado por MCUXpresso IDE 11.10; los comandos y rutas son una
captura de esa versión, no una descripción garantizada de todas las ediciones de MCUXpresso.

## Parte 1: el build

### El build administrado genera Makefiles

En el proyecto observado, MCUXpresso usa el *managed build* de Eclipse CDT: la configuración gráfica
se guarda como metadatos y genera Makefiles que después ejecuta `make`. Otros tipos o versiones de
proyecto pueden incorporar mecanismos diferentes. En este caso hay dos grupos de archivos:

| Archivo | Qué es | Lo edita |
|---------|--------|----------|
| `.project` | metadatos de Eclipse: nombre, naturaleza C, proyectos referenciados | Eclipse |
| `.cproject` | configuración del build: banderas, includes, defines y artefacto | el IDE, desde Properties |
| `Debug/makefile` | el Makefile de verdad | **generado**, se reescribe solo |
| `Debug/*/subdir.mk` | las reglas de compilación por carpeta | **generado** |

Los generados arrancan con este cartel:

```
################################################################################
# Automatically-generated file. Do not edit!
################################################################################
```

Es literal: si los editás, el IDE los pisa en el próximo build. La configuración vive en
`.cproject`, que es un XML grande y poco amigable de leer a mano.

### Qué genera exactamente

Para un proyecto con carpetas `src/` y `Drivers/src/`, el IDE crea dentro de `Debug/`:

| Archivo | Contenido |
|---------|-----------|
| `makefile` | el maestro: incluye a los demás y define los targets `all`, `clean`, `main-build`, `post-build` |
| `sources.mk` | la lista de subdirectorios con fuentes |
| `objects.mk` | los objetos y librerías de usuario |
| `src/subdir.mk` | por cada carpeta: la lista de `.c`, la lista de `.o`, y **la regla de compilación** |

El Makefile maestro coordina las reglas generadas. Este es el de la librería CMSIS del repositorio en
la configuración ensayada:

```makefile
-include sources.mk
-include src/subdir.mk
-include Drivers/src/subdir.mk

all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

main-build: libCMSISv2p00_LPC17xx.a

libCMSISv2p00_LPC17xx.a: $(OBJS) $(USER_OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Invoking: MCU Archiver'
	arm-none-eabi-ar -r  "libCMSISv2p00_LPC17xx.a" $(OBJS) $(USER_OBJS) $(LIBS)

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size libCMSISv2p00_LPC17xx.a ;
```

Si compilás esa librería y ves en la consola `make[1]: Nothing to be done for 'main-build'` seguido
de `Performing post-build steps` y la tabla de `arm-none-eabi-size`, ahora sabés exactamente qué
pasó: **no había nada que recompilar** (los `.o` estaban al día), así que make saltó directo al
`post-build`, que solo corre `size`.

### La regla de compilación, desarmada

En `subdir.mk` está la única línea que realmente importa. Esta es la de un proyecto de aplicación
LPC1769:

```
arm-none-eabi-gcc -DDEBUG -D__CODE_RED -DCORE_M3 -D__USE_CMSIS=CMSISv2p00_LPC17xx \
  -D__LPC17XX__ -D__REDLIB__ -I"..../test_i2c/inc" -I"..../CMSISv2p00_LPC17xx/inc" \
  -O0 -fno-common -g3 -gdwarf-4 -Wall -c -fmessage-length=0 -fno-builtin \
  -ffunction-sections -fdata-sections -fmerge-constants -fmacro-prefix-map="../src/"= \
  -mcpu=cortex-m3 -mthumb -fstack-usage -specs=redlib.specs \
  -MMD -MP -MF"src/test_i2c.d" -MT"src/test_i2c.o" -MT"src/test_i2c.d" \
  -o "src/test_i2c.o" "../src/test_i2c.c"
```

Grupo por grupo:

**Los defines generados para este proyecto**

| Define | Qué hace |
|--------|----------|
| `-DDEBUG` | build de Debug (en Release es `-DNDEBUG`, que apaga los `assert`) |
| `-D__CODE_RED` | marca "compilado con las herramientas Code Red / MCUXpresso" |
| `-DCORE_M3` | el núcleo, lo usa CMSIS para elegir headers |
| `-D__USE_CMSIS=...` | activa el uso de la librería CMSIS del workspace |
| `-D__LPC17XX__` | la familia del micro |
| `-D__REDLIB__` | vas a usar Redlib como libc |

**El target**

`-mcpu=cortex-m3 -mthumb` selecciona la arquitectura y el conjunto de instrucciones. No define el
mapa de memoria, los periféricos, el startup ni el ABI completo. Al cambiar de micro también pueden
cambiar `-mfpu`, `-mfloat-abi`, defines, linker script, bibliotecas y código de inicialización.

**Las banderas de tamaño**

`-ffunction-sections -fdata-sections` ponen cada función y cada variable en su **propia sección** del
`.o`. Por sí solas no hacen nada; se combinan con `--gc-sections` en el link, que después tira las
secciones que nadie usa. Es el mecanismo estándar para que una librería grande como CMSIS no te
meta 40 kB de drivers que no llamás.

`-fno-builtin` le dice a gcc que no reemplace llamadas a funciones estándar por versiones inline
propias. `-fmerge-constants` unifica constantes repetidas.

**Debug**

`-O0` reduce las transformaciones que dificultan relacionar instrucciones con el fuente.
`-g3 -gdwarf-4` agrega información de debug, incluidos macros, en formato DWARF 4. Aun con `-O0`,
una línea de C puede corresponder a varias instrucciones. En este proyecto, Release usaba
`-Os -g`.

**Las dependencias automáticas**

`-MMD -MP -MF"src/test_i2c.d" -MT...` hacen que gcc, mientras compila, escriba un archivo `.d` con
la lista de headers que incluyó el `.c`. El `makefile` los incluye con `-include $(C_DEPS)`. Gracias
a eso, si tocás un `.h`, make recompila solo los `.c` que lo usan. Es una pieza fundamental de
cualquier build serio y es puro make, sin magia del IDE.

**El resto**

`-fmessage-length=0` pone cada error en una línea (para que Eclipse los parsee y te los marque en el
editor). `-fstack-usage` genera archivos `.su` con estimaciones de uso de stack. El resultado puede quedar
marcado como dinámico o acotado y no reemplaza una medición de la profundidad total de llamadas.
`-fmacro-prefix-map` recorta las rutas en `__FILE__`.

### El link

```
arm-none-eabi-gcc -nostdlib -L"..../CMSISv2p00_LPC17xx/Debug" \
  -Xlinker -Map="test_i2c.map" -Xlinker --cref -Xlinker --gc-sections \
  -Xlinker -print-memory-usage -mcpu=cortex-m3 -mthumb \
  -T test_i2c_Debug.ld -o "test_i2c.axf" \
  ./src/cr_startup_lpc175x_6x.o ./src/crp.o ./src/test_i2c.o  -lCMSISv2p00_LPC17xx
```

| Bandera | Qué hace |
|---------|----------|
| `-nostdlib` | omite archivos de arranque y bibliotecas predeterminados; el proyecto debe aportar lo necesario |
| `-Xlinker <x>` | pasale `<x>` directamente a `ld` (gcc no lo interpreta) |
| `-Map=...map` | generá el **mapa**: muestra dónde quedó cada símbolo y es clave para analizar el tamaño |
| `--cref` | agregá al mapa una referencia cruzada de quién usa qué símbolo |
| `--gc-sections` | tirá las secciones no referenciadas (el par de `-ffunction-sections`) |
| `-print-memory-usage` | imprimí la tabla de uso de memoria al terminar |
| `-T test_i2c_Debug.ld` | el linker script, **generado** (sección siguiente) |
| `-lCMSISv2p00_LPC17xx` | linkeá `libCMSISv2p00_LPC17xx.a` |

La salida de `-print-memory-usage` es esta tabla, que sale directo del linker:

```
Memory region         Used Size  Region Size  %age Used
       MFlash512:        1012 B       512 KB      0.19%
        RamLoc32:           4 B        32 KB      0.01%
        RamAHB32:          0 GB        32 KB      0.00%
```

> `.axf` es la extensión usada para un archivo cuyo contenido es ELF. Las herramientas que
> inspeccionan el formato pueden procesarlo, aunque algunas interfaces filtren archivos por
> extensión.

## Parte 2: el linker script generado

El linker script concentra el mapa de memoria y las reglas de ubicación, por lo que entender su
origen ayuda a diagnosticar diferencias entre el proyecto del IDE y un build externo.

### Tres archivos, no uno

Por cada configuración de build, MCUXpresso genera **tres** linker scripts en `Debug/`:

| Archivo | Qué define |
|---------|-----------|
| `<proyecto>_<config>_memory.ld` | el **mapa de memoria**: bloque `MEMORY` con Flash y RAM |
| `<proyecto>_<config>_library.ld` | qué **librerías** se linkean (bloque `GROUP`) |
| `<proyecto>_<config>.ld` | el script principal: el bloque `SECTIONS`, e incluye a los otros dos |

Todos arrancan con `GENERATED FILE - DO NOT EDIT`.

### De dónde sale el mapa de memoria

Del **part support database** del IDE. Para el LPC1769 la entrada está en
`ide/binaries/nxp_lpc17xx.xml`:

```xml
<info name="LPC1769" chip="LPC1769" match_ID="0x26113F37"
      stub='crt_emu_cm3_nxp' flash_driver='LPC175x_6x_512.cfx'>
  <memoryInstance id="MFlash512" location="0x00000000" size="0x80000"/>
  <memoryInstance id="RamLoc32"  location="0x10000000" size="0x8000"/>
  <memoryInstance id="RamAHB32"  location="0x2007c000" size="0x8000"/>
  <prog_flash location="0"       size="0x10000" blocksz="0x1000" maxPrgBuff="0x1000"/>
  <prog_flash location="0x10000" size="0x70000" blocksz="0x8000" maxPrgBuff="0x1000"/>
</info>
```

Ese fragmento contiene la información que LinkServer necesita para esta variante: 512 KiB de FLASH
en `0x00000000`, 32 KiB de RAM local en `0x10000000` y 32 KiB de RAM AHB en
`0x2007C000`. Y los `prog_flash` describen los sectores: 16 de 4 kB
(`0x10000 / 0x1000`) y después 14 de 32 kB (`0x70000 / 0x8000`), que es exactamente lo que dice el
capítulo 32 del manual. También aparece cuál es el driver de Flash y el stub de debug, que usamos en
la parte 3.

Ese XML se convierte en el `_memory.ld`:

```
MEMORY
{
  MFlash512 (rx) : ORIGIN = 0x0,        LENGTH = 0x80000 /* 512K bytes (alias Flash) */
  RamLoc32 (rwx) : ORIGIN = 0x10000000, LENGTH = 0x8000  /* 32K bytes (alias RAM) */
  RamAHB32 (rwx) : ORIGIN = 0x2007c000, LENGTH = 0x8000  /* 32K bytes (alias RAM2) */
}

  __base_MFlash512 = 0x0 ;
  __base_Flash     = 0x0 ;
  __top_MFlash512  = 0x0 + 0x80000 ;
  ...
```

Notá los **alias**: `Flash`, `RAM`, `RAM2`. Son los nombres genéricos que usan las macros de
`cr_section_macros.h` (`__DATA(RAM2)` pone una variable en la RAM AHB). Y los símbolos
`__base_X` / `__top_X` los usa el resto del script, por ejemplo para poner el stack.

### El motor: plantillas FreeMarker

El IDE no arma el `.ld` concatenando strings: usa **FreeMarker**, un motor de plantillas de Java.
Las plantillas tienen extensión `.ldt` y están, en esta instalación, en:

```
/usr/local/LinkServer_<version>/Wizards/linker/     (93 archivos .ldt)
```

Son texto plano y se pueden leer. La de memoria, `memory.ldt`, es esta:

```
MEMORY
{
<#list configMemory as memory>
  ${memory.name} (${memory.linkerMemoryAttributes}) : ORIGIN = ${memory.location}, LENGTH = ${memory.size}
</#list>
}
```

Un `for` sobre los bloques de memoria del micro. Nada más que eso.

Las plantillas se incluyen unas a otras en una jerarquía que arranca en `linkscript.ldt`:

```
linkscript.ldt
├── user.ldt                  (vacía, para que la sobrescribas)
└── linkscript_common.ldt
    ├── header.ldt            el encabezado GENERATED FILE
    ├── includes.ldt          los INCLUDE de _memory.ld y _library.ld
    ├── main_text_section.ldt la seccion .text
    │   ├── global_section_table.ldt
    │   ├── crp.ldt
    │   └── main_rodata.ldt
    ├── main_data_section.ldt
    ├── main_bss_section.ldt
    ├── noinit_section.ldt
    ├── stack_heap.ldt
    ├── checksum.ldt
    └── symbols.ldt
```

En la versión ensayada se puede sustituir una plantilla mediante una carpeta `linkscripts/` en el
proyecto y un archivo `.ldt` con el mismo nombre. El orden observado es:

1. `<proyecto>/linkscripts/`
2. la variable global `searchPath`
3. `<instalacion>/ide/Data/Linkscripts/`
4. las plantillas internas del IDE

Esto permite conservar el generador y cambiar una parte. La otra opción es desactivar la gestión del
script desde las propiedades del linker y mantener un `.ld` propio, como en la
[parte 05](../05_del_codigo_al_binario/). Antes de elegir, verificá qué archivos vuelve a generar tu
versión del IDE.

### Tres cosas del script generado que valen la pena

**La tabla global de secciones.** Al principio de `.text`, justo después de la tabla de vectores, el
linker escribe una tabla con las direcciones y tamaños de todo lo que hay que inicializar:

```
__section_table_start = .;
__data_section_table = .;
LONG(LOADADDR(.data));  LONG(ADDR(.data));  LONG(SIZEOF(.data));
LONG(LOADADDR(.data_RAM2)); LONG(ADDR(.data_RAM2)); LONG(SIZEOF(.data_RAM2));
__data_section_table_end = .;
__bss_section_table = .;
LONG(ADDR(.bss));  LONG(SIZEOF(.bss));
LONG(ADDR(.bss_RAM2)); LONG(SIZEOF(.bss_RAM2));
__bss_section_table_end = .;
```

El `ResetISR` de `cr_startup_lpc175x_6x.c` recorre esa tabla en un `while` y hace las copias y los
ceros. El startup recorre la tabla y puede inicializar varias secciones sin codificar cada dirección en C.
El startup mínimo de la parte 05 usa símbolos fijos para una sola `.data` y una sola `.bss`;
ambos enfoques responden a alcances distintos.

**El checksum del vector table.** Al final del script:

```
PROVIDE(__valid_user_code_checksum = 0 -
        (_vStackTop + (ResetISR + 1) + (NMI_Handler + 1) + (HardFault_Handler + 1)
         + (MemManage_Handler + 1) + (BusFault_Handler + 1) + (UsageFault_Handler + 1)));
```

Esto es **específico de los LPC**. La boot ROM del LPC1769 suma las primeras 7 palabras de la tabla
de vectores y exige que el total dé cero; la palabra 8 (offset `0x1C`) es el complemento que hace
cerrar la cuenta. Si no está bien, el bootloader considera que no hay imagen válida y se queda en
modo ISP en vez de arrancar tu código. El linker calcula el valor y el startup lo pone en la tabla.

> En un LPC1769 con una imagen que no arranca, verificá este vector junto con el pin ISP y el reset.
> La instalación observada incluye `ide/binaries/checksum`, capaz de modificar un `.bin` con
> `checksum -p LPC1769 firmware.bin`. No generalices este esquema a toda la familia LPC ni a otros
> fabricantes: consultá la condición de arranque del dispositivo concreto.

**El CRP.** El *Code Read Protect* del LPC vive en una dirección fija:

```
. = 0x000002FC ;
PROVIDE(__CRP_WORD_START__ = .) ;
KEEP(*(.crp))
ASSERT(!(__CRP_WORD_START__ == __CRP_WORD_END__), "Linker CRP Enabled, but no CRP_WORD provided...");
```

El script ubica la sección CRP en `0x2FC` y comprueba que exista cuando la opción está habilitada.
Los proyectos generados incluyen un `crp.c` para definirla. Como los patrones restringen debug e
ISP, revisá la [sección específica](./02-la-boot-rom-el-isp-y-el-checksum.md) antes de cambiarlos.

### Qué librerías se linkean

El `_library.ld` de un proyecto con Redlib queda:

```
GROUP (
  "libcr_c.a"
  "libcr_eabihelpers.a"
  "libgcc.a"
)
```

Si cambiás la libc desde Quick Settings, el IDE ajusta este grupo y también puede cambiar includes,
defines y specs de compilación. El archivo `_library.ld` muestra qué bibliotecas terminaron
seleccionadas para el link.

### El post-build

```
arm-none-eabi-size "test_i2c.axf";
# arm-none-eabi-objcopy -v -O binary "test_i2c.axf" "test_i2c.bin" ;
# checksum -p LPC1769 -d "test_i2c.bin";
```

Por defecto solo corre `size`. Las otras dos líneas vienen **comentadas** en la plantilla del
proyecto: se descomentan desde Properties, C/C++ Build, Settings, Build steps si querés un `.bin`
listo para grabar por ISP. Para el flujo normal de debug no hace falta, porque el debugger graba
directo desde el `.axf`.

## Parte 3: cómo lo sube a la placa

### Las piezas

La instalación de MCUXpresso puede integrar varias soluciones de debug; su disponibilidad depende de
los componentes instalados:

| Solución | Debug probes | Dónde vive |
|----------|--------|-----------|
| **LinkServer** (NXP) | sondas NXP compatibles y determinados firmwares CMSIS-DAP/OpenSDA | paquete `LinkServer`, enlazado desde `ide/LinkServer` |
| **SEGGER J-Link** | J-Link y OpenSDA con firmware J-Link | `/opt/SEGGER/JLink` |
| **PEmicro** | Multilink, Cyclone, OpenSDA con firmware PEmicro | plugin `com.pemicro.*` |

La configuración del build **no cambia** según cuál uses. Se elige al crear la Launch Configuration.

A continuación se describe LinkServer porque fue el backend inspeccionado. No todas las placas
LPCXpresso ni todos sus firmwares son compatibles con él.

### Componentes observados de LinkServer

```
/usr/local/LinkServer_<version>/
├── LinkServer                    lanzador de línea de comandos
├── binaries/
│   ├── redlinkserv               el servidor que habla con el probe por USB
│   ├── crt_emu_cm_redlink        el "debug stub": traduce gdb a operaciones de debug ARM
│   └── Flash/*.cfx               176 drivers de Flash, uno por familia
```

La cadena completa cuando apretás Debug es:

```
Eclipse  ->  arm-none-eabi-gdb  ->  crt_emu_cm_redlink  ->  redlinkserv  ->  USB  ->  probe  ->  SWD  ->  LPC1769
```

GDB no controla USB ni SWD: habla el **GDB Remote Serial Protocol** con un servidor. En esta cadena,
los componentes de LinkServer terminan traduciendo esas operaciones a accesos al puerto de debug
del Cortex-M3. Es la misma
arquitectura que OpenOCD o pyOCD ([ver 03-04](../03_debug_probes/04-el-software-del-host.md)); cambia la
implementación, no el concepto.

### Los drivers de Flash `.cfx`

Esta es la parte más interesante y la menos conocida.

Un driver `.cfx` contiene código de programación para el target. En la sesión observada, la
secuencia fue:

1. El probe halla el core y **copia el driver `.cfx` a la RAM** del LPC1769.
2. Manda un `VECTRESET` para arrancarlo.
3. El driver, ya corriendo en el micro, borra sectores y escribe páginas usando las rutinas **IAP**
   de la boot ROM.
4. Los datos van llegando por SWD a un buffer en RAM (`maxPrgBuff="0x1000"`, o sea 4 kB por vez).

La sonda transporta el driver y los datos, mientras el target ejecuta las operaciones necesarias
sobre su controlador de FLASH. El log mostró:

```
Opening flash driver LPC175x_6x_512.cfx
Sending VECTRESET to run flash driver
Writing 26880 bytes to address 0x00000000 in Flash
Sectors written: 0, unchanged: 7, total: 7
Closing flash driver
```

Cuál `.cfx` se usa lo dice el part support: para el LPC1769, `flash_driver='LPC175x_6x_512.cfx'`.
Hay uno por tamaño de Flash porque el mapa de sectores cambia.

La línea `unchanged: 7` indica que LinkServer evitó reescribir sectores cuyo contenido consideró
igual. El algoritmo exacto y desde qué versión se aplica son detalles de implementación: medí el
comportamiento de la versión que uses si el tiempo de programación importa.

### Todo eso, desde la terminal

Lo bueno de que LinkServer sea un paquete aparte es que **no necesitás el IDE para usarlo**:

```bash
LS=/usr/local/LinkServer_1.6.133

# ver los probes conectados
$LS/LinkServer probes

# confirmar que el chip esta soportado
$LS/LinkServer devices | grep 1769

# grabar (acepta .axf, .elf, .hex, .s19, o .bin con --addr)
$LS/LinkServer flash LPC1769 load Debug/test_i2c.axf

# borrar toda la Flash
$LS/LinkServer flash LPC1769 erase

# verificar que lo grabado coincide
$LS/LinkServer flash LPC1769 verify Debug/test_i2c.axf

# levantar un gdbserver para depurar con cualquier gdb
$LS/LinkServer gdbserver LPC1769
```

Y desde otra terminal:

```bash
gdb-multiarch Debug/test_i2c.axf
(gdb) target remote :3333
(gdb) load
(gdb) break main
(gdb) continue
```

Estos comandos reproducen las operaciones principales del backend inspeccionado sin abrir Eclipse.
La sintaxis y compatibilidad dependen de la versión de LinkServer y de la sonda. En particular, la
sonda sin número de serie de la OM13085 falló con la versión probada.

> Con J-Link, `JLinkExe -device LPC1769 -if SWD -speed 4000` abre una sesión equivalente para este
> target. Al cambiar de placa también pueden cambiar interfaz, velocidad, reset, alimentación y
> archivo de comandos.

## El resumen: la equivalencia completa

| MCUXpresso hace | Vos escribís |
|-----------------|--------------|
| lee `.cproject` y genera Makefiles | mantenés un Makefile u otro sistema de build |
| compila con banderas del IDE | declarás CPU, ABI, optimización, includes y defines |
| genera 3 linker scripts desde plantillas | mantenés uno o más `.ld` propios ([parte 05](../05_del_codigo_al_binario/)) |
| toma el mapa de memoria del part support XML | ponés el `MEMORY {}` a mano, del manual |
| linkea con Redlib | linkeás con `-specs=nano.specs -specs=nosys.specs` |
| calcula el checksum de los vectores | lo calculás en el linker/build o corregís la imagen con una herramienta controlada |
| corre `size` en el post-build | agregás una línea al Makefile |
| lanza `redlinkserv` + `crt_emu_cm_redlink` + gdb | `LinkServer gdbserver LPC1769` + `gdb-multiarch` |
| graba con un `.cfx` | `LinkServer flash LPC1769 load app.elf`, o `pyocd flash`, o `openocd -c program` |

El IDE genera y coordina estas piezas; un build mantenido a mano las vuelve explícitas. En ambos
casos hay que contrastar el mapa de memoria y la inicialización con el manual, y conservar versiones
reproducibles.

## Para otras placas

Casi todo lo de arriba es genérico. Lo que cambia al pasar del LPC1769 a otro micro:

| Pieza | LPC1769 | Qué cambia |
|-------|---------|-----------|
| Banderas de CPU | `-mcpu=cortex-m3 -mthumb` | el `-mcpu`, y se suman `-mfpu`/`-mfloat-abi` si hay FPU |
| Multilib | `thumb/v7-m/nofp` | lo elige gcc solo, no lo tocás |
| Mapa de memoria | Flash `0x0`, RAM `0x10000000` | del manual del micro, o del CMSIS pack del fabricante |
| Startup y vectores | `cr_startup_lpc175x_6x.c` | uno por familia, lo provee CMSIS o el SDK |
| Validación de imagen por la boot ROM | checksum de vectores en este LPC1769 | cada familia define sus propias condiciones o metadatos |
| CRP en `0x2FC` | específico de LPC | otros tienen su propio mecanismo (option bytes en STM32) |
| Driver de FLASH | `LPC175x_6x_512.cfx` con LinkServer | algoritmo o driver admitido por el servidor elegido; pyOCD puede obtener `.FLM` de un CMSIS-Pack |
| Grabador | LinkServer | pyOCD y OpenOCD soportan cientos de micros con el mismo flujo |

El cambio de CPU es solo una parte del port. También hay que validar memoria, startup, clocks,
periféricos, algoritmo de FLASH, reset y seguridad. Un CMSIS-Pack o SDK aporta descripciones y
código, pero no elimina esa verificación.

---

**LPC1769:** [índice](./README.md) ·
**Anterior:** [06 - El primer grabado, verificado](./06-primer-grabado-verificado.md) ·
**Ver también:** [04 - Toolchains](../04_toolchains/) · [05 - Del código al binario](../05_del_codigo_al_binario/)
