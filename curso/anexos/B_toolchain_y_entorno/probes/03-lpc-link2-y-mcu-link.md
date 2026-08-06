# LPC-Link2 y MCU-Link

Los dos debug probes modernos de NXP. Tienen una característica que los hace distintos de todos
las demás: **el firmware se les puede cambiar**, y según cuál les cargues hablan un
protocolo u otro. El mismo probe físico puede ser CMSIS-DAP hoy y J-Link mañana.

- **LPC-Link2**: basada en un LPC4322. Viene integrada en las LPCXpresso V2 y V3, y
  también se vende suelta (OM13054).
- **MCU-Link**: la generación siguiente, más chica y barata. Viene en las placas nuevas de
  NXP y suelta (MCU-LINK). Agrega un puerto serie virtual (VCOM) y medición de consumo.

## Identificarlas

```bash
lsusb | grep -iE "1fc9|link"
```

| ID USB | Qué es |
|--------|--------|
| `1fc9:0090` | LPC-Link2 |
| `1fc9:0143` | MCU-Link |
| `1fc9:0132` y similares | LPC-Link2 con firmware CMSIS-DAP cargado |
| `1366:xxxx` | el probe tiene firmware **J-Link** cargado (ver abajo) |

Si aparece con el ID de SEGGER (`1366`), no es que tengas un J-Link: es tu LPC-Link2 con
el firmware de J-Link puesto. Es una opción que NXP ofrece oficialmente.

## Los tres firmwares posibles

| Firmware | Protocolo | Funciona con |
|----------|-----------|--------------|
| **CMSIS-DAP** | abierto | openocd, pyocd, LinkServer, Keil, IAR... **el que querés** |
| **J-Link** | SEGGER | las herramientas de SEGGER y openocd con `interface/jlink.cfg` |
| **redlink** | propietario NXP | solo LinkServer / MCUXpresso |

Las que vienen en placas suelen traer CMSIS-DAP de fábrica, pero no siempre: si compraste
el probe suelto o la placa pasó por otras manos, puede tener cualquiera de los tres.

## Cambiarle el firmware

La herramienta se llama **LPCScrypt** (para LPC-Link2) o viene con el instalador de
**MCU-Link**. Las dos se bajan de [nxp.com/lpcutilities](https://www.nxp.com/lpcutilities).

El procedimiento es el mismo en ambas:

1. **Poné el probe en modo DFU.** Hay que puentear un jumper mientras lo enchufás:
   - LPC-Link2: el jumper **JP1** (a veces rotulado `DFU`)
   - MCU-Link: el jumper **J3** (rotulado `ISP` o `FW update`)

   Enchufá el USB con el jumper puesto, y después sacalo.

2. **Corré el script que corresponda:**

```bash
# LPC-Link2
<dir-de-LPCScrypt>/scripts/program_CMSIS       # firmware CMSIS-DAP  <- el recomendado
<dir-de-LPCScrypt>/scripts/program_JLINK       # firmware J-Link

# MCU-Link
<dir-de-MCU-Link>/scripts/program_CMSIS
```

3. **Desenchufá y volvé a enchufar** (ya sin el jumper).

Después de eso, el probe es CMSIS-DAP y todo lo de la [guía 01](./01-cmsis-dap.md) aplica
tal cual.

> El script detecta solo qué probe es y elige la imagen correcta, así que no hay que
> preocuparse por bajar el archivo justo.
>
> Hay una variante "non-bridged" del firmware CMSIS-DAP que deja solo la parte de
> depuración y quita el puerto serie virtual y la medición de consumo. Para el curso da
> igual; el VCOM incluso es cómodo, porque te da la UART sin un adaptador aparte.

## Usarla, ya con CMSIS-DAP

Exactamente igual que el probe de la cátedra:

```bash
make flash
make debug
```

O a mano:

```bash
openocd -f openocd/lpc1769.cfg -c "program build/firmware.elf verify reset exit"
pyocd flash -W -t lpc1768 build/firmware.hex
```

### Si le dejaste el firmware J-Link

Cambiá la línea de interface en
[`openocd/lpc1769.cfg`](../../../../plantilla/openocd/lpc1769.cfg):

```tcl
# source [find interface/cmsis-dap.cfg]
source [find interface/jlink.cfg]
```

El resto del archivo queda igual. Ver la [guía 04](./04-jlink.md).

### Si tiene redlink

Solo la maneja LinkServer:

```bash
make flash FLASHER=linkserver
```

Conviene cambiarle el firmware a CMSIS-DAP y olvidarse.

## El VCOM del MCU-Link (y de LPC-Link2 con firmware bridged)

Estos probes, además de depurar, exponen un **puerto serie virtual** conectado a la UART
del micro. Con un solo cable USB tenés grabado, depuración y consola serie:

```bash
ls /dev/ttyACM*                      # aparece un puerto nuevo
screen /dev/ttyACM0 115200           # o: picocom, minicom, cu
```

Es exactamente donde va a salir el `printf` redirigido a UART0 del
[módulo 0, capítulo 16](../../../00_lenguaje_c/16-redirigir-printf-a-uart.md). Hay que
verificar que la UART del LPC1769 esté cableada al probe en tu placa: en las LPCXpresso
V2/V3 lo está.

## Cablearla a un LPC1769 suelto

Si tenés el probe por separado y una placa sin probe, el conector es el **Cortex Debug de
10 pines** (paso 1.27 mm). Los que hacen falta:

| Pin del conector | Señal | Al LPC1769 |
|------------------|-------|------------|
| 1 | VTref | 3V3 (el probe lo usa para saber a qué tensión trabajar) |
| 2 | SWDIO | P1.16 / SWDIO |
| 4 | SWCLK | P1.17 / SWCLK |
| 3, 5, 9 | GND | GND |
| 10 | nRESET | RESET (opcional pero recomendado) |

**VTref no es opcional**: el probe lo usa para detectar que hay un target alimentado y
para adaptar los niveles lógicos. Sin él, muchos probes ni intentan conectarse.

---

**Probes:** [índice](./README.md) ·
**Anterior:** [02 - LPC-Link original](./02-lpc-link-original.md) ·
**Siguiente:** [04 - J-Link](./04-jlink.md)
