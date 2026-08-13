# J-Link

J-Link es una familia de sondas de SEGGER con herramientas propias y soporte de GDB. OpenOCD también
incluye un driver J-Link, aunque no aprovecha todas las funciones del software nativo.

Puede ser una sonda separada, una implementación on-board autorizada o un firmware J-Link ofrecido
para determinados [LPC-Link2 o MCU-Link](./02-lpc-link2-y-mcu-link.md).

## Identificarla

```bash
lsusb | grep -i segger
# Bus 001 Device 005: ID 1366:0101 SEGGER J-Link
```

El VID USB `1366` identifica a SEGGER, pero no distingue por sí solo el modelo físico ni una
implementación on-board.

## Camino A: con OpenOCD

Es un cambio de una línea en
[`openocd/lpc1769.cfg`](../../../plantilla/openocd/lpc1769.cfg):

```tcl
# source [find interface/cmsis-dap.cfg]
source [find interface/jlink.cfg]

transport select swd
set WORKAREASIZE 0x2000
set CCLK 4000
source [find target/lpc17xx.cfg]
adapter speed 1000
```

Y de ahí en más todo igual:

```bash
make flash
make debug
```

Para usar OpenOCD con un J-Link en Linux hace falta que el usuario tenga acceso al
dispositivo USB. La regla ya está en
[`99-lpc-probes.rules`](../../../plantilla/tools/99-lpc-probes.rules).

En Windows, OpenOCD usa WinUSB. Los modelos actuales suelen poder configurarlo desde **J-Link
Configurator** y muchos ya vienen en ese modo; unidades antiguas pueden usar el driver legado de
SEGGER. Verificá la configuración antes de cambiar nada. Si reemplazás el driver por otro mecanismo,
las herramientas SEGGER pueden dejar de reconocer la sonda hasta restaurarlo.

## Camino B: con las herramientas de SEGGER

Se bajan de
[segger.com/downloads/jlink](https://www.segger.com/downloads/jlink/) (paquete "J-Link
Software and Documentation Pack", hay `.deb` para Ubuntu).

### Grabar

```bash
JLinkExe -device LPC1769 -if SWD -speed 4000 -autoconnect 1
J-Link> loadfile build/firmware.hex
J-Link> r        # reset
J-Link> g        # go (arrancar)
J-Link> q        # salir
```

O sin interacción, con un guión:

```bash
cat > flash.jlink <<'EOF'
loadfile build/firmware.hex
r
g
q
EOF
JLinkExe -device LPC1769 -if SWD -speed 4000 -CommanderScript flash.jlink
```

El `-device LPC1769` selecciona el algoritmo de FLASH y la configuración del target. La imagen del
curso ya lleva el checksum del vector 7; mantenelo correcto en el build en lugar de depender de un
parche implícito de la herramienta.

### Depurar

```bash
# terminal 1
JLinkGDBServerCLExe -device LPC1769 -if SWD -speed 4000

# terminal 2
gdb-multiarch build/firmware.elf -x debug.gdb
```

El ejecutable puede llamarse `JLinkGDBServerCLExe` o `JLinkGDBServer` según la instalación.
Consultá su ayuda para confirmar el puerto, que normalmente es 2331, y hacelo coincidir con el
archivo de comandos de GDB.

Desde VSCode, Cortex-Debug soporta J-Link de forma nativa. Agregá esta configuración a
[`launch.json`](../../../plantilla/.vscode/launch.json):

```jsonc
{
  "name": "Debug con J-Link",
  "type": "cortex-debug",
  "request": "launch",
  "servertype": "jlink",
  "cwd": "${workspaceFolder}",
  "executable": "${workspaceFolder}/build/firmware.elf",
  "device": "LPC1769",
  "interface": "swd",
  "runToEntryPoint": "main",
  "preLaunchTask": "build",
  "gdbPath": "gdb-multiarch"
}
```

## Cablearla a la placa

Conector estándar Cortex de 10 pines, o los 20 pines clásicos de J-Link. Lo mínimo:

| Señal | Al LPC1769 | ¿Obligatorio? |
|-------|-----------|---------------|
| VTref | 3V3 | **sí**: sin esto el J-Link no detecta el target |
| SWDIO | pin **TMS/SWDIO** | sí |
| SWCLK | pin **TCK/SWDCLK** | sí |
| GND | GND | sí |
| nRESET | RESET | recomendado |

Ante `Cannot connect to target`, comprobá primero VTref, GND, orientación, alimentación, reset,
cableado y velocidad. Después revisá protección y configuración del servidor.

## Sobre la licencia

Las J-Link EDU y EDU Mini están limitadas a uso educativo sin fines de lucro y no comercial. Una
actividad de una materia normalmente encaja si no ocurre dentro de un entorno comercial, pero hay
que leer los términos del modelo concreto; un desarrollo para una empresa no queda cubierto.

---

**Guía por sonda:** [índice](./README.md) ·
**Anterior:** [02 - LPC-Link2 y MCU-Link](./02-lpc-link2-y-mcu-link.md) ·
**Siguiente:** [04 - Otras sondas](./04-otros-probes.md)
