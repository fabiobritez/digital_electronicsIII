# LPC-Link original: el debug probe de las LPCXpresso viejas

Es la que traen las **LPCXpresso de la primera generación** (aproximadamente 2010 a 2013,
las que tienen la línea de perforado en el medio de la placa y no dicen "CMSIS-DAP" en
ninguna parte). Es el caso incómodo: **no funciona con openocd ni con pyocd**.

Si tu placa es la rev D (OM13085), no es esta: andá a la
[guía 01](./01-cmsis-dap.md).

## Por qué no funciona con las herramientas abiertas

El probe es un **LPC3154**, y el firmware que corre no implementa CMSIS-DAP: habla un
protocolo propietario de NXP heredado de Code Red, la empresa que hacía el IDE
LPCXpresso antes de que NXP la comprara. Sin la especificación de ese protocolo, openocd y
pyocd no tienen con qué hablarle. No es una limitación que se arregle con una
configuración: sencillamente no hay soporte.

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
| La placa no dice "CMSIS-DAP" por ningún lado | es este probe |
| `pyocd list` no muestra nada, pero la placa está enchufada y encendida | es este probe |
| Línea de perforado que parte la placa en dos, sin conector Cortex de 10 pines | primera generación |

## Qué podés hacer

### Opción A: LinkServer (lo que funciona sin tocar nada)

**LinkServer** es el grabador de NXP, y sí entiende este probe. La buena noticia es que se
instala **suelto**, sin el IDE: es un programa de línea de comandos de unos cientos de MB,
no los 1.4 GB de MCUXpresso.

Se baja de [nxp.com/linkserver](https://www.nxp.com/linkserver) (hace falta crear una
cuenta gratuita en NXP).

```bash
LinkServer probes                              # ver los probes conectados
LinkServer flash LPC1769 load build/firmware.elf
LinkServer flash LPC1769 erase
LinkServer gdbserver LPC1769                   # servidor gdb en el puerto 3333
```

Con la [plantilla](../../../../plantilla/):

```bash
make flash FLASHER=linkserver
```

Y para depurar, en dos terminales:

```bash
LinkServer gdbserver LPC1769                   # terminal 1
gdb-multiarch build/firmware.elf -x debug.gdb  # terminal 2
```

Desde VSCode, usá la configuración "Debug con LinkServer (NXP)" de
[`launch.json`](../../../../plantilla/.vscode/launch.json), levantando el gdbserver antes en
una terminal.

Esto conserva casi todo lo bueno: el build sigue siendo 100% abierto y portable, y lo
único propietario es el último paso, el de mover bytes a la FLASH.

### Opción B: grabar por el puerto serie, sin usar el probe

El LPC1769 trae de fábrica un bootloader en ROM que graba por UART0. No necesita ninguna
probe: alcanza con un adaptador USB-serial de dos dólares. La contra es que **no podés
depurar**, solo grabar.

Está explicado en la [guía 06](./06-sin-probe-isp.md).

Para una materia donde el foco es entender los periféricos, es una opción perfectamente
razonable: se compensa con LEDs y `printf` por UART (módulo 12).

### Opción C: un probe externo

Cualquier probe CMSIS-DAP barato (o un J-Link, o un ST-Link reciclado de una Nucleo) se
conecta a los pines SWD del LPC1769 y te devuelve el camino abierto completo. En estas
placas viejas los pines SWD están en el conector de expansión; hay que ubicar **SWDIO**,
**SWCLK**, **GND** y, en algunos casos, **nRESET**, y deshabilitar el probe de a bordo
según indique el manual de tu placa.

Ver la [guía 05](./05-otros-probes.md).

### Lo que NO se puede

**Convertirla a CMSIS-DAP no es una opción realista.** LPCScrypt, la herramienta de NXP
que reprograma el firmware de los probes, soporta LPC-Link2 y las LPCXpresso V2/V3, pero
no el LPC-Link original. El LPC3154 recibe su firmware por DFU desde el software de NXP en
cada arranque, y no hay imagen CMSIS-DAP publicada para él.

## Resumen de decisión

| Tu situación | Hacé esto |
|--------------|-----------|
| Querés depurar y no te molesta un programa de NXP | **LinkServer** (opción A) |
| Querés cero dependencias y te alcanza con grabar | **ISP serial** (opción B) |
| Querés cero dependencias y depurar | **probe externo** (opción C) |

---

**Probes:** [índice](./README.md) ·
**Anterior:** [01 - CMSIS-DAP](./01-cmsis-dap.md) ·
**Siguiente:** [03 - LPC-Link2 y MCU-Link](./03-lpc-link2-y-mcu-link.md)
