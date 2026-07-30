# ¿Qué es un toolchain? (la sección de compiladores)

Cuando apretás "Build" en MCUXpresso, no corre un programa: corren **varios**, en cadena. A ese
conjunto de herramientas que convierten tu código C en un firmware se le llama **toolchain** (cadena
de herramientas). MCUXpresso trae una adentro; acá vas a conocerla por separado para poder usarla sin
el IDE.

## El nombre: `arm-none-eabi-gcc`

El toolchain estándar para Cortex-M se llama **GNU Arm Embedded Toolchain**, y sus programas empiezan
con `arm-none-eabi-`. Ese prefijo es el **target triplet** (a quién apunta), y cada parte significa
algo:

| Parte | Significa |
|-------|-----------|
| `arm` | la arquitectura del procesador: ARM (el Cortex-M3) |
| `none` | el sistema operativo de destino: **ninguno** (bare-metal, tu código corre sin SO) |
| `eabi` | la convención de llamadas/binario: *Embedded Application Binary Interface* |

Compará con el `gcc` "de PC", que sería algo como `x86_64-linux-gnu-gcc` (arquitectura x86_64, SO
Linux, libc gnu). La diferencia clave: el de PC genera programas para **la misma** máquina donde
compila; el `arm-none-eabi-` genera programas para **otra** máquina (el micro). Eso se llama
**compilación cruzada** (*cross-compilation*): compilás en tu PC x86, pero el binario es para ARM.

> Por eso no podés correr el `.elf` del micro en tu PC: son instrucciones de otra arquitectura. Tu PC
> solo **fabrica** el firmware; quien lo ejecuta es el LPC1769.

## Las piezas del toolchain

Un toolchain no es un solo programa. Estas son sus partes (todas están en `tools/toolchain/bin/` del
repo, con el prefijo `arm-none-eabi-`):

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
| `size` | dice cuánto ocupan `.text`/`.data`/`.bss` (módulo 16) |
| `readelf` | inspecciona la estructura interna del `.elf` |
| `gdb` | el **depurador** (breakpoints y ver registros, página 03). No viene en el paquete del repo: se instala con `sudo apt install gdb-multiarch` |

A esto se suma una pieza que no es un programa sino una **biblioteca**:

- **newlib** (y su versión chica, **newlib-nano**): es la *libc* para embebidos. Provee `memcpy`,
  `printf`, `malloc`, etc., en una versión liviana pensada para micros. Cuando usás `printf`, el
  código viene de acá. (En embebidos se suele preferir `newlib-nano` por tamaño, y redirigir la
  salida a la UART como vimos en el módulo 9.)

## La cadena, en un comando

Todo lo que MCUXpresso hace con clicks, es esta cadena (lo que corrimos para `mygpio`):

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
| `-T lpc1769.ld` | usá **este linker script** (el mapa de memoria, módulo 16) |
| `-O2` / `-Os` | nivel de optimización (`-Os` optimiza para **tamaño**, útil en micros) |
| `-Wall -Wextra` | activá los **warnings** (te avisan de bugs probables) |
| `-g` | incluí info de **debug** (para gdb) |
| `-I<dir>` | dónde buscar los `#include` (ej. los headers de CMSIS) |

> Probá `arm-none-eabi-objdump -d mygpio.elf` para **ver el ensamblador** que generó el compilador, o
> `arm-none-eabi-nm mygpio.elf` para ver los símbolos. Son herramientas de oro para entender qué
> hace tu código por dentro.

## ¿De dónde sale el toolchain?

Es **gratis y abierto** (lo mantiene ARM sobre GCC). Se baja como un `.tar.gz` y se descomprime en
cualquier carpeta: **no requiere instalación ni permisos de administrador**. En este repo está en
`tools/toolchain/`, instalado con `tools/install_toolchain.sh`. MCUXpresso trae su propia copia
adentro; es exactamente la misma herramienta.

Con esto ya sabés qué es "el compilador" y qué hace cada pieza. En la
[próxima página](./02-setup-vscode.md) lo conectamos a VSCode para tener un entorno completo. Si
querés ver **qué es cada archivo** de esa carpeta, y no solo los programas principales, está la
[página 04](./04-adentro-del-toolchain.md).

---

**Módulo:** [Toolchain y entorno](./README.md) · **Siguiente:** [02 - Setup en VSCode](./02-setup-vscode.md)
