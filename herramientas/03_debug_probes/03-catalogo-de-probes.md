# Catálogo de sondas

Esta selección reúne las familias de sondas más comunes. Primero muestra cómo identificarlas
y después resume qué software y funciones ofrece cada una.

Las instrucciones concretas para usar cada una **con el LPC1769** están en
[07 - LPC1769, guía por sonda](../07_lpc1769/probes/).

---

## Identificá la tuya

Enchufá la placa y corré:

```bash
lsusb                 # Linux
# macOS:   system_profiler SPUSBDataType
# Windows: Administrador de dispositivos, o  pyocd list
```

El **VID** (los primeros cuatro dígitos hexadecimales) identifica a la organización que
registró el dispositivo USB. Sirve como pista, pero un firmware alternativo o un clon puede
usar otro VID:

| VID | Fabricante | Qué es probable que sea |
|---|---|---|
| `1fc9` | NXP | CMSIS-DAP, LPC-Link2, MCU-Link |
| `0471` | Philips / NXP viejo | LPC-Link original |
| `1366` | SEGGER | J-Link (o una sonda con firmware de J-Link) |
| `0483` | STMicroelectronics | ST-Link |
| `03eb` | Atmel / Microchip | EDBG, Atmel-ICE |
| `2e8a` | Raspberry Pi | Debug Probe, o una Pico con `debugprobe` |
| `0d28` | ARM / Mbed | DAPLink |
| `0403` | FTDI | un módulo FT2232 usado como sonda |
| `1d50` | OpenMoko (usado por proyectos libres) | Black Magic Probe |

Si el nombre del producto incluye **`CMSIS-DAP`** o **`DAPLink`**, probá primero el backend
CMSIS-DAP de OpenOCD o pyOCD. El nombre ayuda, pero la compatibilidad final depende de que el
firmware implemente correctamente la especificación.

> Si no aparece nada al enchufar, revisá primero el cable. Los cables USB de cargador de
> celular muchas veces tienen **solo los dos hilos de alimentación** y ningún hilo de datos.
> La placa se enciende y los LED prenden, pero el host no detecta ningún dispositivo.

---

## La tabla completa

| Sonda | Interfaz hacia el host | OpenOCD | pyOCD | SWO | Dónde aparece |
|---|---|:---:|:---:|:---:|---|
| **CMSIS-DAP v1.x** | estándar público, USB HID | sí | sí | opcional | LPCXpresso rev D (**la de la cátedra**) y placas anteriores |
| **CMSIS-DAP v2.x** | estándar público, USB bulk | sí | sí | opcional | sondas y placas modernas |
| **DAPLink** | CMSIS-DAP + funciones opcionales | sí | sí | según hardware y firmware | micro:bit y varias placas de evaluación |
| **J-Link** | propietario de SEGGER | sí | mediante plugin | según modelo | sonda externa o firmware autorizado en otro hardware |
| **ST-Link V2 / V3** | propietario de ST | sí | mediante plugin | según versión y pines expuestos | Nucleo, Discovery o sonda externa |
| **LPC-Link original** | propietario de NXP/Code Red | no | no | no | LPCXpresso de primera generación |
| **LPC-Link2 / MCU-Link** | depende del firmware cargado | según firmware | según firmware | según firmware y hardware | placas NXP o sondas externas |
| **Black Magic Probe** | GDB Remote directo | no hace falta | no | según plataforma | sonda externa o firmware en otra placa |
| **FT2232 / FT232H** | MPSSE de FTDI | sí | no | no habitual | módulos USB genéricos |
| **Raspberry Pi Pico con `debugprobe`** | CMSIS-DAP v2 + CDC UART | sí | sí | no documentado | una Pico o Pico 2 reprogramada |
| **PEmicro Multilink** | propietario | no habitual | no habitual | según modelo | ecosistema NXP/Freescale |

---

## Las abiertas

### CMSIS-DAP y DAPLink

La interfaz pública de Arm. Si la sonda y el target están soportados, podés usar herramientas
independientes del fabricante del chip
([capítulo anterior](./02-cmsis-dap.md)).

Se consiguen clones genéricos por muy poco, basados en STM32F103 o en RP2040. Dos
advertencias con los más baratos: a veces vienen con firmware viejo y con bugs, y muchos **no
traen la línea de reset cableada**, lo cual se nota el día que necesitás resetear el target
por hardware.

### Black Magic Probe

Un caso distinto y elegante: **el gdbserver corre adentro de la sonda**. No necesitás
OpenOCD ni ninguna otra cosa en la PC. Te conectás con gdb directo a un puerto serie:

```bash
gdb-multiarch build/firmware.elf
(gdb) target extended-remote /dev/ttyACM0
(gdb) monitor swdp_scan
(gdb) attach 1
(gdb) load
```

Reduce una capa de software en el host y soporta LPC17xx. A cambio, su flujo y sus comandos
`monitor` difieren de los usados con OpenOCD.

### FT2232 y clones FTDI

Los chips FTDI de doble canal tienen un modo (**MPSSE**) que permite hacer JTAG y SWD por
software. Se consiguen módulos genéricos muy baratos y OpenOCD los soporta con
`interface/ftdi/*.cfg`. La contra es que hay que armarse el archivo de configuración con la
asignación de pines correcta, y eso ya es trabajo.

### Raspberry Pi Pico

Vale su propia página: [05 - Armarte tu propia sonda](./05-armarte-tu-propia-sonda.md).

---

## Las propietarias

### J-Link (SEGGER)

J-Link es una familia amplia de sondas comerciales. El protocolo es propietario; SEGGER
publica herramientas para Linux, macOS y Windows, y OpenOCD ofrece un driver para los modelos
compatibles mediante `interface/jlink.cfg`.

Dos cosas para saber:

- Las **J-Link EDU** y **EDU Mini** cuestan mucho menos, con una licencia limitada a uso
  educativo y de aficionado. Para una materia estás dentro de esos términos.
- En Windows, para usarla con OpenOCD hay que cambiarle el driver a **WinUSB** con
  [Zadig](https://zadig.akeo.ie/), y entonces las herramientas de SEGGER dejan de verla.
  Conviene elegir un camino y quedarse.

### ST-Link

Las placas Nucleo y Discovery de ST traen una a bordo, y **se puede usar con micros de otros
fabricantes** aunque ST no lo publicite. Si alguien tiene una Nucleo dando vueltas, ahí hay
una sonda gratis.

Las versiones actuales de OpenOCD pueden usar la API directa al DAP de ST-Link V2/V3 con
firmware reciente. También conservan el backend HLA anterior, que expone menos operaciones.
La compatibilidad con un target que no sea de ST debe verificarse con la versión concreta de
sonda, firmware y OpenOCD.

### Las de NXP

Tres generaciones, y la diferencia entre ellas es enorme:

| Generación | Chip | Protocolo | ¿Sirve con herramientas libres? |
|---|---|---|---|
| **LPC-Link** (2010-2013) | LPC3154 | propietario, heredado de Code Red | no con OpenOCD o pyOCD; requiere herramientas compatibles de NXP |
| **LPC-Link2** | LPC4322 | el que le cargues | sí, con firmware CMSIS-DAP |
| **MCU-Link** | LPC55S69 | CMSIS-DAP v2; otras opciones dependen del modelo | sí, con CMSIS-DAP |

LPC-Link2 y MCU-Link admiten firmware actualizable, pero las opciones no son iguales en
todos los modelos. LPC-Link2 tuvo imágenes CMSIS-DAP, J-Link y propietarias de NXP; en la
familia MCU-Link, las funciones disponibles dependen de la variante. El modelo Base ya ofrece
SWD, SWO y VCOM con CMSIS-DAP; otras variantes pueden agregar medición de energía u opciones de
firmware. Antes de elegir una, revisá el modelo, el firmware y el conector concretos.

### PEmicro

Son sondas del ecosistema NXP/Freescale con soporte en MCUXpresso y otras herramientas
comerciales. No se usan en el flujo preparado para esta materia.

---

## Y el caso "no tengo ninguna"

No es el fin del mundo. El LPC1769 se graba con un **adaptador USB-serie compatible** por
su bootloader de fábrica ([ver 01-02](../01_panorama/02-como-se-graba-un-micro.md) y
[07/probes/05](../07_lpc1769/probes/05-sin-probe-isp.md)). Lo que perdés es la depuración: por
ese camino solo grabás.

Sin depuración por hardware todavía podés diagnosticar con LEDs, UART e instrumentación,
aunque perdés breakpoints, watchpoints y lectura directa de registros.

---

## Lo que hay que respetar, sea cual sea la sonda

Cuatro reglas de cableado que resuelven la mayoría de los "no conecta":

1. **VTref / VDD_TARGET conectado.** Casi todas las sondas lo usan para detectar que hay un
   target alimentado y para adaptar los niveles lógicos. Sin él no se conectan, y el mensaje
   de error no lo dice.
2. **Masa común.** La sonda y la placa tienen que compartir GND. Es el error de cableado
   número uno.
3. **No alimentes la placa por dos lados a la vez** sin saber lo que estás haciendo (el USB
   de la placa más los 3V3 de la sonda).
4. **Cables cortos.** A varios MHz, la longitud, la masa y el tendido afectan la integridad
   de señal. Si aparecen desconexiones, acortá los cables y bajá `adapter speed`.

## Cómo saber si tu sonda está soportada por OpenOCD

```bash
ls /usr/share/openocd/scripts/interface/
ls /usr/share/openocd/scripts/interface/ftdi/
```

Esos archivos muestran los adaptadores para los que esa instalación trae configuración. La
lista efectiva también depende de los drivers habilitados al compilar OpenOCD; podés verla
con `openocd -c "adapter list; shutdown"`.

---

**Sondas:** [índice](./README.md) ·
**Anterior:** [02 - CMSIS-DAP](./02-cmsis-dap.md) ·
**Siguiente:** [04 - El software del lado de la PC](./04-el-software-del-host.md)
