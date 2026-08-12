# ¿Qué es un toolchain?

Cuando apretás "Build" en MCUXpresso se ejecutan varias herramientas en cadena. Ese conjunto
se llama **toolchain**. MCUXpresso incluye uno; acá se lo separa del IDE para entender qué
hace cada programa y poder usarlo desde otro entorno.

## El nombre: `arm-none-eabi-gcc`

El toolchain estándar para Cortex-M se llama **Arm GNU Toolchain**, y sus programas empiezan con
`arm-none-eabi-`. Ese prefijo es el **target triplet**: describe **para qué máquina** genera código.

| Parte | En una línea |
|-------|-----------|
| `arm` | la arquitectura del procesador de destino: ARM de 32 bits |
| `none` | el campo de fabricante, sin uno específico |
| `eabi` | el entorno bare metal que usa la *Embedded Application Binary Interface* de Arm |

Comparalo con un GCC para Linux, cuyo prefijo puede ser `x86_64-linux-gnu-`. En una PC
x86-64, ese compilador genera código nativo; `arm-none-eabi-` genera código para un target
Arm. Eso es **compilación cruzada** (*cross-compilation*).

> Por eso no podés correr el `.elf` del micro en tu PC: son instrucciones de otra arquitectura. Tu PC
> solo **fabrica** el firmware; quien lo ejecuta es el LPC1769.

El triplet campo por campo, qué otros existen, y por qué **no alcanza** para saber para qué micro
estás compilando, está en la [página 02](./02-el-triplet.md).

## Las piezas del toolchain

Un toolchain no es un solo programa. Estas son sus piezas principales. Las herramientas que
invocás están en `tools/toolchain/bin/`; programas internos como `cc1` viven en
`libexec/`:

| Programa | Qué hace |
|----------|----------|
| `gcc` | el **director**: invoca al preprocesador, compilador, ensamblador y linker en orden |
| `cpp` (interno) | **preprocesador**: resuelve `#include`, `#define`, `#ifdef` |
| `cc1` (interno) | **compilador**: traduce C a ensamblador ARM |
| `as` | **ensamblador**: traduce ensamblador a código máquina (`.o`) |
| `ld` | **linker**: une los `.o`, aplica el linker script y produce el `.elf` |
| `objcopy` | convierte el `.elf` a `.bin` / `.hex` (lo que se graba en la Flash) |
| `objdump` | **desensamblador**: te muestra el código máquina y a qué C corresponde |
| `nm` | lista los **símbolos** (funciones y variables) de un `.o`/`.elf` |
| `size` | dice cuánto ocupan `.text`/`.data`/`.bss` ([parte 05](../05_del_codigo_al_binario/)) |
| `readelf` | inspecciona la estructura interna del `.elf` |
| `gdb` | el **depurador** (breakpoints y ver registros, [parte 06](../06_depurar_en_serio/)). No viene en el paquete del repo: se instala con `sudo apt install gdb-multiarch` |

A esto se suma una pieza que no es un programa sino una **biblioteca**:

- **newlib** (y su versión chica, **newlib-nano**): es la *libc* para embebidos. Provee `memcpy`,
  `printf`, `malloc` y otras funciones. Newlib-nano prioriza menor tamaño y omite o
  simplifica algunas funciones. En un sistema bare metal todavía tenés que definir cómo sale
  el texto, por ejemplo mediante UART o RTT.

## La cadena, en un comando

Todo lo que MCUXpresso hace con clicks, es esta cadena (lo que corrimos para
[`mygpio`](../../curso/02_arma_tu_propia_libreria/)):

```bash
# 1) compilar + ensamblar + linkear, en una sola invocación de gcc:
arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -T lpc1769.ld \
    mygpio.c main.c startup.c -o mygpio.elf

# 2) extraer el binario "pelado" para grabar:
arm-none-eabi-objcopy -O binary mygpio.elf mygpio.bin   # o -O ihex para .hex
```

Las banderas importantes:

| Bandera | Para qué |
|---------|----------|
| `-mcpu=cortex-m3` | generá código **para el Cortex-M3** (no genérico) |
| `-mthumb` | usá el set de instrucciones **Thumb** (el que usa Cortex-M) |
| `-T lpc1769.ld` | usá **este linker script** (el mapa de memoria, [parte 05](../05_del_codigo_al_binario/)) |
| `-O2` / `-Os` | nivel de optimización (`-Os` optimiza para **tamaño**, útil en micros) |
| `-Wall -Wextra` | activá los **warnings** (te avisan de bugs probables) |
| `-g` | incluí info de **debug** (para gdb) |
| `-I<dir>` | dónde buscar los `#include` (ej. los headers de CMSIS) |

> Probá `arm-none-eabi-objdump -d mygpio.elf` para ver el desensamblado, o agregá `-S`
> para intercalarlo con el código fuente cuando el ELF incluya información de debug.
> `arm-none-eabi-nm mygpio.elf` muestra los símbolos.

## ¿De dónde sale el toolchain?

GCC, binutils, GDB y newlib son proyectos abiertos. Arm publica una distribución precompilada
para sus arquitecturas y contribuye a esos proyectos. Esa distribución se entrega como un
archivo que puede descomprimirse sin permisos de administrador. En este repo se instala en
`tools/toolchain/` mediante `tools/install_toolchain.sh`. MCUXpresso incluye una distribución
basada en las mismas herramientas, con una versión y agregados elegidos por NXP. De dónde bajarlo y qué otras distribuciones hay, en la
[página 04](./04-otros-toolchains.md).

Con esto ya sabés qué es "el compilador" y qué hace cada pieza. Si querés ver **qué es cada
archivo** de esa carpeta, y no solo los programas principales, seguí por la
[página 03](./03-anatomia-de-la-carpeta.md). Y para conectarlo a un editor cómodo, la
[página 05](./05-el-editor-y-el-entorno.md).

---

**Toolchains:** [índice](./README.md) · **Siguiente:** [02 - El target triplet](./02-el-triplet.md)
