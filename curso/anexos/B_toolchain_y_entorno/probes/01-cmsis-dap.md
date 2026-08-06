# CMSIS-DAP: el debug probe de la placa de la cátedra

Es la que traen las **LPCXpresso LPC1769 rev D** (código de NXP: **OM13085**), y es el
mejor caso posible: no necesitás una sola línea de software de NXP para grabar ni para
depurar.

## Qué tenés exactamente

En un rincón de la placa, del lado del conector USB, hay un **segundo microcontrolador**:
un **LPC11U35**. No corre tu programa; corre el firmware **CMSIS-DAP** de ARM, cuyo único
trabajo es traducir entre el USB de tu PC y los dos pines SWD del LPC1769.

```
        ┌─────────────────── placa OM13085 ───────────────────┐
        │                          ┊                          │
 USB ───┤  LPC11U35   ── SWD ──►   ┊   LPC1769                │
        │  (el probe)  SWDIO/SWCLK ┊   (tu micro)             │
        │                          ┊                          │
        └──────────────────────────┴──────────────────────────┘
                       línea de corte / jumpers
```

Lo importante: **CMSIS-DAP es un estándar abierto de ARM**, no un invento de NXP. Por eso
la placa funciona con openocd, pyocd, Keil, IAR, Rust, Zephyr y cualquier otra cosa que
soporte el estándar. Esa es la diferencia con las LPCXpresso viejas
([guía 02](./02-lpc-link-original.md)), que traían un probe propietario.

### Cómo confirmar que es esta

```bash
lsusb | grep -i cmsis
# Bus 001 Device 034: ID 1fc9:001d NXP Semiconductors NXP CMSIS-DAP
```

Tenés que ver algo con `CMSIS-DAP` en el nombre, y un ID de vendedor `1fc9` (NXP). Si en
vez de eso ves `0471:df55`, tenés el probe viejo.

Es **CMSIS-DAP v1**, o sea que se comunica por HID (`bInterfaceClass 3`). Eso tiene una
consecuencia práctica: pyocd necesita el módulo `hidapi` para hablarle, y sin él no la
detecta. Otro detalle de esta sonda, que se paga caro más abajo: declara su número de
serie USB **vacío**.

En Windows no hace falta instalar ningún driver: CMSIS-DAP v1 se presenta como un
dispositivo HID, de la misma familia que un teclado, y Windows lo reconoce solo.

## Grabar

Cualquiera de las dos herramientas abiertas sirve. Con la
[plantilla](../../../../plantilla/):

```bash
make flash
```

Detecta cuál tenés instalada. Para forzar una:

```bash
make flash FLASHER=openocd
make flash FLASHER=pyocd
```

### Con OpenOCD

```bash
openocd -f openocd/lpc1769.cfg -c "program build/firmware.elf verify reset exit"
```

El archivo [`openocd/lpc1769.cfg`](../../../../plantilla/openocd/lpc1769.cfg) de la plantilla
ya está armado para este probe. En esencia son tres líneas:

```tcl
source [find interface/cmsis-dap.cfg]   # el probe
transport select swd                    # 2 pines, no JTAG
source [find target/lpc17xx.cfg]        # el chip
```

### Con pyOCD

```bash
pyocd flash -W -t lpc1768 build/firmware.hex
```

El target `lpc1768` viene incorporado en pyocd y sirve para el 1769: misma familia, misma
FLASH de 512 KB, misma RAM. Si querés el nombre exacto:

```bash
pyocd pack install LPC1769
pyocd flash -W -t lpc1769 build/firmware.hex
```

El `-W` importa: sin él, si la placa no está enchufada pyocd se queda esperando en
silencio para siempre en lugar de avisarte.

## Depurar

```bash
make debug
```

Levanta el servidor, graba, y te deja en gdb parado en `main`. Desde VSCode es **F5**
(configuración "Debug con OpenOCD" de
[`launch.json`](../../../../plantilla/.vscode/launch.json)).

Por debajo no hay nada más que esto:

```bash
# terminal 1
openocd -f openocd/lpc1769.cfg

# terminal 2
gdb-multiarch build/firmware.elf
(gdb) target extended-remote :3333
(gdb) load
(gdb) break main
(gdb) continue
```

## Usar un probe externo en vez del de a bordo

La placa tiene un **conector Cortex de 10 pines** para enchufar otro probe, y jumpers para
desconectar el de a bordo. Sirve si querés usar un J-Link, o si el probe integrado se
rompió. Con el probe de a bordo deshabilitado, la placa pasa a ser un LPC1769 pelado con
sus pines SWD accesibles: seguí la guía del probe que vayas a usar.

## Lo que NO funciona con esta sonda: LinkServer

Si tenés MCUXpresso instalado, LinkServer parece la opción obvia. **No lo es.** Detecta la
sonda, pero fijate en la columna del serial:

```
$ LinkServer probes
  #  Description    Serial
---  -------------  --------
  1  NXP CMSIS-DAP
```

Vacía. Este probe declara el descriptor USB `iSerial` como cadena vacía, y LinkServer le
pasa ese serial vacío a su motor de grabado:

```
Nc: Connecting to probe serial '' core 0 - Ee(E1). Probe serial number not found
Et:31: No connection to chip's debug port
```

Pasarle el índice en vez del serial (`--probe '#1'`) tampoco sirve: por debajo sigue
mandando `--probeserial ''`. No hay manera de darle la vuelta desde la línea de comandos.

Con **OpenOCD anda a la primera**, así que usá eso y listo. Está probado de punta a punta
en la [página 08](../08-primer-grabado-verificado.md).

## Problemas típicos

**`unable to find a matching CMSIS-DAP device` (Linux)**

Permisos de USB. Instalá las reglas de udev:

```bash
sudo cp plantilla/tools/99-lpc-probes.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
```

Y desenchufá y volvé a enchufar la placa. No uses `sudo openocd`: te va a funcionar en la
terminal y te va a fallar en VSCode.

Para verificar que las reglas hicieron efecto, mirá el dueño del nodo USB: tiene que pasar
de `root root` a `root plugdev` (o mostrar el `+` de la ACL de `uaccess`).

```bash
ls -l /dev/bus/usb/001/034      # el número sale de lsusb
```

**`pyocd list` dice `No available debug probes are connected`, con la placa enchufada**

Dos causas, en este orden. Primero, falta el backend HID: `pip install hidapi`, porque esta
sonda es CMSIS-DAP v1. Segundo, los permisos de udev: sin acceso de escritura al nodo USB,
la biblioteca no puede leer el nombre del producto, pyocd busca la cadena `CMSIS-DAP` para
filtrar, no la encuentra y descarta la sonda **en silencio**.

**Funcionaba y de golpe dejó de aparecer**

La sonda se traba después de un intento de conexión fallido (por ejemplo, LinkServer
peleándose con el serial vacío). El firmware del LPC11U35 se queda esperando el final de
una transacción que nunca se completó. El síntoma típico de OpenOCD es:

```
Warn : could not read product string for device 0x1fc9:0x001d: Operation timed out
```

**Desenchufá y volvé a enchufar el cable USB.** Es lo único que la saca de ese estado. Al
reconectar se re-enumera con otro número de dispositivo, lo cual es normal.

**Graba bien pero la placa no hace nada**

Casi seguro es el **checksum del vector 7**: la boot ROM verifica que la suma de las
primeras 8 palabras de la tabla de vectores dé cero, y si no, se queda en modo ISP sin
correr tu programa. `openocd` lo parchea solo, pero **pyocd no**. La plantilla lo inyecta
en tiempo de compilación para que los dos caminos funcionen igual. Verificalo:

```bash
make preflight     # el checksum y todo lo demás que impide arrancar
make vectores      # la tabla de vectores en crudo, si querés verla
```

Para saber si el chip está corriendo tu programa o quedó en el bootloader, mirá dónde está
el PC:

```bash
openocd -f openocd/lpc1769.cfg -c "init; halt; exit"
```

Si el PC cae en `0x1fff0xxx` está en la **boot ROM**, o sea que no encontró código de
usuario válido. Si cae en una dirección chica (`0x000001xx` para un programa de este
tamaño), está corriendo lo tuyo.

El detalle completo está en
[`tools/lpc_checksum.py`](../../../../plantilla/tools/lpc_checksum.py) y en el
[anexo A](../../A_build_linker_startup/02-linker-y-startup.md).

**`Warning: checksum mismatch` al hacer verify con openocd**

Lo mismo al revés: openocd parchea el checksum al escribir, así que lo grabado queda
distinto del archivo en disco y la verificación se queja. Inyectándolo en el build (lo que
hace la plantilla) el aviso desaparece.

**`Error: Debug adapter doesn't support any transports`**

Estás usando una versión de openocd anterior a la 0.10, sin soporte de CMSIS-DAP.
Actualizá: `sudo apt install openocd` en Ubuntu 22.04 o posterior ya trae una versión
suficiente.

**Se desconecta en medio de la grabación**

Bajá la velocidad en `openocd/lpc1769.cfg`: cambiá `adapter speed 1000` por `500` o `100`.

---

**Probes:** [índice](./README.md) ·
**Siguiente:** [02 - LPC-Link original](./02-lpc-link-original.md)
