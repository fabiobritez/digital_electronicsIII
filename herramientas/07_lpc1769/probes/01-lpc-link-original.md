# LPC-Link original: el debug probe de las LPCXpresso viejas

Algunas LPCXpresso de primera generación incorporan una sonda basada en LPC3154, anterior a
CMSIS-DAP. OpenOCD y pyOCD no implementan su protocolo propietario, por lo que hay que usar un
backend NXP compatible, el ISP del target o una sonda externa.

Si tu placa es la rev D (OM13085), no es esta: andá a la
[la página de la sonda de a bordo](../01-la-placa-y-su-sonda.md).

## Por qué no funciona con las herramientas abiertas

El probe es un **LPC3154**, y el firmware que corre no implementa CMSIS-DAP: habla un
protocolo propietario de NXP heredado de Code Red, la empresa que hacía el IDE
LPCXpresso antes de que NXP la comprara. OpenOCD y pyOCD no ofrecen un driver para ese protocolo. Cambiar el archivo de interfaz no agrega
soporte que el servidor no implementa.

Hay un detalle más que confunde: el probe **arranca sin firmware**. Al enchufar la placa,
el LPC3154 aparece como un dispositivo DFU vacío, y es el software de NXP el que le carga
el firmware por USB cada vez. Por eso el ID que ves con `lsusb` cambia según si ya se
conectó el IDE o no.

```bash
lsusb | grep -iE "0471|nxp|lpc"
# ID 0471:df55  ->  el LPC3154 en modo DFU, todavia sin firmware
```

## Cómo identificarla

| Señal | Qué indica |
|-------|-----------|
| ID USB `0471:df55` | LPC3154 en DFU: es este probe |
| La placa no anuncia CMSIS-DAP | indicio, no identificación definitiva |
| `pyocd list` no muestra nada | puede ser esta sonda, permisos o backend faltante |
| Diseño y línea de corte de primera generación | comparalo con el código y el esquemático de la placa |

## Qué podés hacer

### Opción A: software NXP compatible

Las versiones históricas de LPCXpresso/Redlink admitían el LPC-Link original. LinkServer actual se
distribuye también por separado, pero su lista de sondas compatibles puede cambiar. Instalá una
versión que documente este LPC3154 y comprobalo con `LinkServer probes` antes de preparar el resto
del flujo.

Se baja de [nxp.com/linkserver](https://www.nxp.com/linkserver) (hace falta crear una
cuenta gratuita en NXP).

```bash
LinkServer probes                              # ver los probes conectados
LinkServer flash LPC1769 load build/firmware.elf
LinkServer flash LPC1769 erase
LinkServer gdbserver LPC1769                   # servidor gdb en el puerto 3333
```

Con la [plantilla](../../../plantilla/):

```bash
make flash FLASHER=linkserver
```

Y para depurar, en dos terminales:

```bash
LinkServer gdbserver LPC1769                   # terminal 1
gdb-multiarch build/firmware.elf -x debug.gdb  # terminal 2
```

Desde VSCode, usá la configuración "Debug con LinkServer (NXP)" de
[`launch.json`](../../../plantilla/.vscode/launch.json), levantando el gdbserver antes en
una terminal.

El build puede seguir usando GCC y Make; el backend que controla esta sonda permanece propietario.

### Opción B: grabar por el puerto serie, sin usar el probe

El LPC1769 trae de fábrica un bootloader en ROM que graba por UART0. No necesita una sonda: requiere un adaptador USB-serie con niveles compatibles. Permite programar,
pero no ofrece breakpoints ni acceso de debug.

Está explicado en la [guía 05](./05-sin-probe-isp.md).

Para una materia donde el foco es entender los periféricos, es una opción perfectamente
razonable: se compensa con LEDs y `printf` por UART ([parte 06](../../06_depurar_en_serio/)).

### Opción C: un probe externo

Una sonda externa compatible con Cortex-M3 puede conectarse al puerto SWD del LPC1769. Ubicá
**SWDIO**, **SWCLK**, **GND**, **VTref** y, de ser posible, **nRESET** en el esquemático. También
hay que aislar el LPC-Link original para evitar que dos salidas manejen las mismas señales.

Ver la [guía 04](./04-otros-probes.md).

### Lo que NO se puede

**Convertirla a CMSIS-DAP no es una opción realista.** LPCScrypt, la herramienta de NXP
que reprograma el firmware de los probes, soporta LPC-Link2 y las LPCXpresso V2/V3, pero
no el LPC-Link original. El LPC3154 recibe su firmware por DFU desde el software de NXP en
cada arranque, y no hay imagen CMSIS-DAP publicada para él.

## Resumen de decisión

| Tu situación | Hacé esto |
|--------------|-----------|
| Tu versión del software NXP reconoce la sonda | usá su servidor y verificá grabación/debug (opción A) |
| Solo necesitás grabar y tenés acceso a los pines de ISP | **ISP serial** (opción B) |
| Necesitás depurar con herramientas actuales | **sonda externa** (opción C) |

---

**Guía por sonda:** [índice](./README.md) ·
**Siguiente:** [02 - LPC-Link2 y MCU-Link](./02-lpc-link2-y-mcu-link.md) ·
**La sonda de a bordo:** [07-01](../01-la-placa-y-su-sonda.md)
