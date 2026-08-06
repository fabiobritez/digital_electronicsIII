# J-Link

El debug probe de **SEGGER**. Es el más rápido y el más sólido del mercado, y lo vas a encontrar
en cualquier laboratorio. El protocolo es propietario, pero SEGGER publica las
herramientas para todos los sistemas operativos y openocd la soporta bien.

Puede llegar a tus manos de tres formas: comprada suelta (J-Link EDU cuesta bastante menos
que la versión comercial), integrada en placas de otros fabricantes, o como firmware
cargado en un [LPC-Link2 o MCU-Link](./03-lpc-link2-y-mcu-link.md).

## Identificarla

```bash
lsusb | grep -i segger
# Bus 001 Device 005: ID 1366:0101 SEGGER J-Link
```

Cualquier ID que empiece con `1366` es SEGGER.

## Camino A: con OpenOCD (recomendado, sigue siendo abierto)

Es un cambio de una línea en
[`openocd/lpc1769.cfg`](../../../../plantilla/openocd/lpc1769.cfg):

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
[`99-lpc-probes.rules`](../../../../plantilla/tools/99-lpc-probes.rules).

En Windows, OpenOCD necesita que el J-Link tenga el driver **WinUSB** en lugar del de
SEGGER. Se cambia con [Zadig](https://zadig.akeo.ie/). El costo es que después las
herramientas de SEGGER dejan de verlo hasta que revertís el driver, así que conviene
elegir un camino y quedarse.

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

El `-device LPC1769` es importante: con ese nombre, SEGGER sabe cómo es la FLASH del chip
y **calcula solo el checksum del vector 7**, así que por este camino no hace falta
inyectarlo (aunque no molesta que esté).

### Depurar

```bash
# terminal 1
JLinkGDBServer -device LPC1769 -if SWD -speed 4000

# terminal 2
gdb-multiarch build/firmware.elf -x debug.gdb
```

`JLinkGDBServer` escucha en el puerto **2331** por defecto, no en el 3333. O lo cambiás
con `-port 3333`, o editás `debug.gdb`.

Desde VSCode, Cortex-Debug soporta J-Link de forma nativa. Agregá esta configuración a
[`launch.json`](../../../../plantilla/.vscode/launch.json):

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
| SWDIO | P1.16 / SWDIO | sí |
| SWCLK | P1.17 / SWCLK | sí |
| GND | GND | sí |
| nRESET | RESET | recomendado |

El error `Cannot connect to target` casi siempre es VTref sin conectar o una masa que no
es común entre el probe y la placa.

## Sobre la licencia

Las J-Link EDU y EDU Mini tienen una licencia que las limita a uso educativo y de
aficionado. Para una materia estás dentro de esos términos; para un desarrollo comercial,
no. Las herramientas te lo recuerdan al arrancar.

---

**Probes:** [índice](./README.md) ·
**Anterior:** [03 - LPC-Link2 y MCU-Link](./03-lpc-link2-y-mcu-link.md) ·
**Siguiente:** [05 - Otros probes](./05-otros-probes.md)
