# Otros debug probes: ST-Link, clones y hacerte uno

Cualquier debug probe que hable **SWD** puede grabar y depurar un LPC1769. El chip no sabe ni le
importa quién está del otro lado de los dos pines: SWD es un estándar de ARM, no de NXP.
Esto es útil de saber, porque abre opciones muy baratas.

## ST-Link (reciclado de una Nucleo o Discovery)

Las placas de STMicroelectronics traen un ST-Link a bordo que se puede usar con **otros
fabricantes**, aunque ST no lo publicite. Si alguien en la cátedra tiene una Nucleo dando
vueltas, ahí hay un probe.

### Con OpenOCD

```tcl
# en openocd/lpc1769.cfg
source [find interface/stlink.cfg]
transport select swd
set WORKAREASIZE 0x2000
set CCLK 4000
source [find target/lpc17xx.cfg]
```

Una advertencia honesta: el ST-Link es un probe **HLA** (*high level adapter*). En vez de
darle a openocd acceso crudo al bus de depuración, expone operaciones de alto nivel. Anda
bien para grabar y para depurar normal, pero algunas cosas no funcionan (`adapter speed`
se ignora en parte, ciertos comandos de bajo nivel no están). Para el curso alcanza de
sobra.

### Con pyOCD

```bash
pyocd flash -W -t lpc1768 --probe stlink build/firmware.hex
```

### Cableado

En una Nucleo hay que **sacar los dos jumpers de `ST-LINK`** (los que dicen `CN2` en la
mayoría) para desconectar el probe del micro de la propia placa, y usar el conector
`CN4` / `SWD`:

| Pin de CN4 | Señal | Al LPC1769 |
|------------|-------|-----------|
| 1 | VDD_TARGET | 3V3 |
| 2 | SWCLK | P1.17 / SWCLK |
| 3 | GND | GND |
| 4 | SWDIO | P1.16 / SWDIO |
| 5 | NRST | RESET |

## Probes CMSIS-DAP genéricos

Se consiguen por muy poco: hay clones basados en STM32F103 o en RP2040. Si dicen
"CMSIS-DAP" o "DAPLink", funcionan con openocd y pyocd sin ninguna configuración especial,
igual que el [probe de la cátedra](./01-cmsis-dap.md).

Cuidado con dos cosas: los clones más baratos a veces vienen con firmware viejo y buggy, y
muchos no traen la línea de reset cableada.

## Hacerte una con lo que tengas

Vale la pena saber que existe, aunque no lo uses:

- **Raspberry Pi Pico como probe**: la fundación publica
  [`debugprobe`](https://github.com/raspberrypi/debugprobe), un firmware oficial que
  convierte una Pico en un probe CMSIS-DAP completo. Es la opción más barata que hay y
  funciona muy bien.
- **Raspberry Pi (la de verdad)**: openocd puede hacer *bit-banging* de SWD directamente
  sobre los pines GPIO, sin ningún hardware extra
  (`interface/raspberrypi-native.cfg`). Es lento pero funciona.
- **FT2232 y clones**: los módulos genéricos basados en FTDI se pueden usar con
  `interface/ftdi/*.cfg`. Hay que armarse el archivo de configuración con los pines
  correctos.

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

Soporta LPC17xx. Es un probe menos común, pero si te cruzás con uno, es el de menos
piezas móviles.

## Lo que hay que respetar, sea cual sea el probe

1. **VTref / VDD_TARGET conectado.** Casi todos los probes lo usan para detectar que hay
   un target alimentado. Sin él, no se conectan y el mensaje de error no lo dice.
2. **Masa común.** El probe y la placa tienen que compartir GND. Es el error de cableado
   número uno.
3. **No alimentes la placa por dos lados a la vez** sin saber qué estás haciendo (USB de
   la placa más 3V3 del probe).
4. **Cables cortos.** SWD anda a varios MHz: 20 cm de cable de protoboard ya empiezan a
   dar problemas. Si tenés desconexiones raras, bajá `adapter speed`.

## Cómo saber si tu probe está soportado por OpenOCD

```bash
ls /usr/share/openocd/scripts/interface/
ls /usr/share/openocd/scripts/interface/ftdi/
```

Cada archivo de ahí es un probe soportado. Son más de cien.

---

**Probes:** [índice](./README.md) ·
**Anterior:** [04 - J-Link](./04-jlink.md) ·
**Siguiente:** [06 - Sin probe: ISP serial](./06-sin-probe-isp.md)
