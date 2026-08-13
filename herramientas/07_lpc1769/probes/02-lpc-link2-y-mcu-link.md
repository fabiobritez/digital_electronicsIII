# LPC-Link2 y MCU-Link

LPC-Link2 y MCU-Link son familias de sondas NXP reprogramables. No comparten necesariamente los
mismos firmwares ni funciones: antes de actualizar hay que identificar el modelo y consultar su
manual.

- **LPC-Link2**: basada en un LPC4322. Viene integrada en las LPCXpresso V2 y V3, y
  también se vende suelta (OM13054).
- **MCU-Link**: familia basada en LPC55S69, disponible como sonda separada y como circuito integrado
  en placas. El modelo base ofrece SWD, SWO y VCOM; medición de energía y puentes adicionales
  dependen de las variantes Pro u on-board.

## Identificarlas

```bash
lsusb | grep -iE "1fc9|link"
```

| ID USB | Qué es |
|--------|--------|
| `1fc9:0090` | observado en LPC-Link2 |
| `1fc9:0143` | observado en MCU-Link |
| `1fc9:0132` y similares | puede corresponder a LPC-Link2 con otro firmware o modo |
| `1366:xxxx` | identificador de vendedor SEGGER; puede ser una sonda física o un firmware J-Link autorizado |

Si aparece con el ID de SEGGER (`1366`), no es que tengas un J-Link: es tu LPC-Link2 con
el firmware de J-Link puesto. Es una opción que NXP ofrece oficialmente.

## Opciones de firmware

- **LPC-Link2** admite imágenes CMSIS-DAP y J-Link distribuidas para ese hardware. En modo DFU,
  MCUXpresso también puede cargar temporalmente el firmware que necesita.
- **MCU-Link base** viene con CMSIS-DAP y su manual indica que no ejecuta la imagen J-Link disponible
  para otras implementaciones.
- **MCU-Link Pro** y algunas variantes on-board sí ofrecen una opción J-Link. Las funciones de
  puente y medición requieren el firmware CMSIS-DAP y el hardware correspondiente.

La compatibilidad se determina por el par modelo/firmware, no solamente por el nombre MCU-Link.

## Cambiarle el firmware

LPC-Link2 se actualiza con **LPCScrypt**; MCU-Link usa el paquete de firmware y las utilidades que
NXP distribuye para su modelo. Se obtienen desde
[nxp.com/lpcutilities](https://www.nxp.com/lpcutilities). El esquema general es:

1. **Entrá al modo de actualización.** El número de jumper cambia entre la sonda separada y cada
   placa. Buscá `DFU`, `ISP` o `FW update` en el manual y comprobá que el dispositivo se
   enumere en ese modo antes de escribir.

2. **Corré el script que corresponda:**

```bash
# LPC-Link2
<dir-de-LPCScrypt>/scripts/program_CMSIS       # firmware CMSIS-DAP  <- el recomendado
<dir-de-LPCScrypt>/scripts/program_JLINK       # firmware J-Link

# MCU-Link (el nombre exacto depende del paquete)
<dir-de-MCU-Link>/scripts/program_CMSIS
```

3. **Desenchufá y volvé a enchufar** (ya sin el jumper).

Después, verificá con la herramienta de actualización y con `pyocd list` u OpenOCD qué firmware y
capacidades quedaron activos. No uses un script de otro modelo aunque el micro de la sonda sea
parecido. Algunas imágenes CMSIS-DAP ofrecen variantes con o sin puentes USB; elegí según las
funciones cableadas en tu hardware.

## Usarla, ya con CMSIS-DAP

Con la configuración de interfaz CMSIS-DAP, el flujo general es el mismo que con la sonda de la
cátedra:

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
[`openocd/lpc1769.cfg`](../../../plantilla/openocd/lpc1769.cfg):

```tcl
# source [find interface/cmsis-dap.cfg]
source [find interface/jlink.cfg]
```

El target sigue siendo el LPC1769, pero verificá velocidad, reset y selección de sonda. Ver la
[guía 03](./03-jlink.md).

### Si tiene redlink

Solo la maneja LinkServer:

```bash
make flash FLASHER=linkserver
```

Podés conservarlo si tu flujo usa LinkServer o cargar un firmware compatible siguiendo el
procedimiento del modelo.

## El VCOM del MCU-Link (y de LPC-Link2 con firmware bridged)

Estos probes, además de depurar, exponen un **puerto serie virtual** conectado a la UART
del micro. Con un solo cable USB tenés grabado, depuración y consola serie:

```bash
ls /dev/ttyACM*                      # aparece un puerto nuevo
screen /dev/ttyACM0 115200           # o: picocom, minicom, cu
```

El texto aparece ahí solamente si la UART elegida en el firmware está conectada al puente VCOM de la
placa. Confirmá TX, RX y la instancia UART en el esquemático antes de asumir que corresponde a
UART0.

## Cablearla a un LPC1769 suelto

Si tenés el probe por separado y una placa sin probe, el conector es el **Cortex Debug de
10 pines** (paso 1.27 mm). Los que hacen falta:

| Pin del conector | Señal | Al LPC1769 |
|------------------|-------|------------|
| 1 | VTref | 3V3 (el probe lo usa para saber a qué tensión trabajar) |
| 2 | SWDIO | pin **TMS/SWDIO** del LPC1769 |
| 4 | SWCLK | pin **TCK/SWDCLK** del LPC1769 |
| 3, 5, 9 | GND | GND |
| 10 | nRESET | RESET (opcional pero recomendado) |

> Ojo: en el LPC176x los pines de depuración son **pines dedicados** del encapsulado (1 a 5 en el
> LQFP100: TDO/SWO, TDI, TMS/SWDIO, TRST, TCK/SWDCLK), **no** pines de propósito general. No los
> busques entre los `P0.x` / `P1.x`.

**VTref es necesario en estas sondas** para detectar la alimentación del target y establecer los
niveles de la interfaz. No lo confundas con una salida de alimentación: consultá el manual antes de
unirlo a 3,3 V.

---

**Guía por sonda:** [índice](./README.md) ·
**Anterior:** [01 - LPC-Link original](./01-lpc-link-original.md) ·
**Siguiente:** [03 - J-Link](./03-jlink.md)
