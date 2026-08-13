# Otros debug probes: ST-Link, clones y hacerte uno

Una sonda capaz de transportar SWD puede acceder al puerto de debug del LPC1769, pero el flujo
completo también requiere niveles eléctricos compatibles, un servidor que admita la sonda y un
algoritmo de FLASH para el target.

## ST-Link (reciclado de una Nucleo o Discovery)

Muchas Nucleo y Discovery incorporan ST-Link. Algunos modelos permiten aislarlo del STM32 de la
placa y exponen señales para un target externo; confirmalo en el manual de esa revisión.

### Con OpenOCD

```tcl
# en openocd/lpc1769.cfg
source [find interface/stlink.cfg]
transport select swd
set WORKAREASIZE 0x2000
set CCLK 4000
source [find target/lpc17xx.cfg]
```

OpenOCD actual puede usar su API DAP directa para ST-Link; documentación antigua lo muestra como
HLA. La capacidad concreta depende de la generación y del firmware, así que comprobá el transporte,
reset, velocidad y comandos disponibles en tu combinación.

### Con pyOCD

pyOCD puede usar ST-Link mediante su plugin. Instalalo, comprobá que `pyocd list` muestre la sonda
y recién después ejecutá el comando de FLASH para el target elegido. No supongas que
`--probe stlink` selecciona o instala el backend por sí solo.

### Cableado

Los jumpers y conectores cambian entre Nucleo y Discovery. Buscá en el manual cómo separar
ST-LINK del STM32 y ubicá VTref/VDD_TARGET, SWCLK, GND, SWDIO y NRST. Verificá dos veces el pinout:
un nombre como `CN2` o `CN4` no es universal.

## Probes CMSIS-DAP genéricos

Hay sondas CMSIS-DAP basadas en distintos micros. El nombre indica el protocolo, no garantiza
calidad eléctrica, transporte USB, reset, SWO ni firmware actualizado. OpenOCD o pyOCD todavía
necesitan reconocer el dispositivo y el LPC1769. Revisá pinout, VTref y capacidades declaradas.

## Hacerte una con lo que tengas

Vale la pena saber que existe, aunque no lo uses:

- **Raspberry Pi Pico como probe**: la fundación publica
  [`debugprobe`](https://github.com/raspberrypi/debugprobe), un firmware oficial que
  convierte una Pico en una sonda CMSIS-DAP v2 con SWD y UART. No documenta SWO ni adaptación de
  niveles, por lo que se usa con targets de 3,3 V y cableado controlado.
- **Raspberry Pi con Linux**: OpenOCD puede generar SWD mediante GPIO con configuraciones como
  `raspberrypi-native.cfg`. Requiere niveles compatibles, masa común, acceso privilegiado al
  periférico y cables cortos; no es eléctricamente equivalente a una sonda con buffers.
- **FT2232 y módulos FTDI**: OpenOCD los usa habitualmente para JTAG mediante MPSSE. SWD no está
  garantizado por cualquier módulo o cableado. Hace falta un esquema y un archivo de interfaz
  específicos.

## Black Magic Probe

Un caso distinto y elegante: el probe **corre el gdbserver adentro**. No necesitás openocd
ni ninguna otra cosa; te conectás con gdb directo a un puerto serie:

```bash
gdb-multiarch build/firmware.elf
(gdb) target extended-remote /dev/ttyACM0
(gdb) monitor swdp_scan
(gdb) attach 1
(gdb) load
```

El soporte de LPC17xx depende del firmware instalado en la sonda. Su ventaja arquitectónica es que
GDB se conecta directamente al servidor del probe; la programación de FLASH todavía debe estar
implementada para ese target.

> En el LPC176x los pines de depuración son **dedicados** (pines 1 a 5 del LQFP100), no `P0.x` ni
> `P1.x`. En la placa de la cátedra salen al conector Cortex de 10 pines.

## Lo que hay que respetar, sea cual sea el probe

1. **VTref / VDD_TARGET según el manual.** Muchas sondas lo usan para detectar alimentación y fijar
   niveles. Averiguá si es una entrada de referencia o también puede alimentar.
2. **Masa común.** La sonda y la placa deben compartir GND.
3. **No alimentes la placa por dos lados a la vez** sin saber qué estás haciendo (USB de
   la placa más 3V3 del probe).
4. **Integridad de señal.** Mantené cortas las conexiones y una buena referencia de masa. Ante
   errores intermitentes, inspeccioná las señales y bajá `adapter speed`.

## Cómo saber si tu probe está soportado por OpenOCD

```bash
openocd -c "adapter list; shutdown"
ls /usr/share/openocd/scripts/interface/
```

La lista de adaptadores muestra drivers compilados; los scripts aportan configuraciones para
hardware concreto. La presencia de un `.cfg` no garantiza que tu variante, firmware o target hayan
sido probados juntos.

---

**Guía por sonda:** [índice](./README.md) ·
**Anterior:** [03 - J-Link](./03-jlink.md) ·
**Siguiente:** [05 - Sin sonda: ISP serial](./05-sin-probe-isp.md)
