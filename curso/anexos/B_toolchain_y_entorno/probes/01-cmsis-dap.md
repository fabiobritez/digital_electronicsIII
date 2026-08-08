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

Es **CMSIS-DAP v1**. Si querés comprobarlo vos, la prueba decisiva está en los
descriptores USB:

```bash
lsusb -d 1fc9:001d -v | grep -iE "bInterfaceClass|bNumEndpoints|wMaxPacketSize|bInterval|iSerial"
```

| Lo que sale | Qué prueba |
|---|---|
| `bInterfaceClass 3` (HID) | **Definitivo.** CMSIS-DAP v2 es, por definición, USB **bulk** (clase vendor-specific). Si habla HID, es v1 |
| 2 endpoints interrupt, 64 B, `bInterval 1` | el perfil HID clásico: 64 bytes cada milisegundo |
| `iSerial` vacío | v2 **exige** número de serie; esta no lo declara |
| `bNumInterfaces 1` | solo HID: ni disco USB ni puerto serie |

Y OpenOCD lo corrobora por otro lado:

```console
$ openocd -f interface/cmsis-dap.cfg -c "transport select swd" -c init -c exit
Info : CMSIS-DAP: SWD supported
Info : CMSIS-DAP: FW Version = 1.0
```

Fijate lo que **no** dice: `SWO-UART supported`. La v1.0 no tiene los comandos de SWO.

Eso tiene tres consecuencias prácticas, y conviene tenerlas juntas:

1. **pyocd necesita el módulo `hidapi`** para hablarle; sin él no la detecta y no te avisa
   por qué.
2. **No podés capturar SWO** con esta sonda ([módulo 12, capítulo 3](../../../12_debug/03-consola-por-el-debugger-rtt.md)).
3. **El caudal está acotado por el HID.** 64 bytes cada milisegundo son 64 KB/s teóricos, y
   como cada lectura de memoria necesita ida y vuelta, en la práctica se sacan ~15.7 KB/s
   (medido con RTT). No es un límite del LPC1769 ni del cable SWD: subir el reloj del SWD de
   4 a 15 MHz no cambia el resultado ni un byte.

Y un detalle más, que se paga caro abajo: declara su número de serie USB **vacío**.

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

## ¿Se puede actualizar a CMSIS-DAP v2?

Sí, técnicamente. Pero antes de entusiasmarte conviene separar **qué ganarías**, **qué
riesgo hay** y **si vale la pena**, porque la respuesta corta es que para esta materia
probablemente no.

### Qué ganarías

- **Más caudal.** v2 usa USB bulk en vez de HID, así que el techo de ~15.7 KB/s de RTT sube.
- **Captura de SWO**, que hoy no tenés.
- **Un número de serie**, con lo cual LinkServer pasaría a funcionar (ver la sección
  anterior).

### Qué NO se puede dañar

Esto es lo primero que hay que entender, porque acota el miedo:

- **El LPC1769 no corre ningún riesgo.** Es otro chip. Estarías grabando el micro de la
  sonda, no el tuyo.
- **El LPC11U35 no se puede "brickear" de forma permanente.** Su bootloader vive en **ROM
  de máscara**: no es borrable por ningún medio, y se entra por un **pin de hardware al
  reset**, no por software. El peor caso posible no es "la sonda murió", es "la sonda no
  anda hasta que le grabe una imagen correcta".

### Cuál es el riesgo real

- **Quedarte sin sonda funcionando por un rato.** En una materia donde la placa se comparte,
  eso cuesta más de lo que parece.
- **Grabar una imagen compilada para otra placa con el mismo chip.** Enumeraría bien pero
  podría no hablar con el target, porque las asignaciones de pines (SWDIO, SWCLK, reset)
  difieren entre diseños. Recuperable, pero confuso.
- **No poder entrar al bootloader.** Este es el único escenario que te deja colgado de
  verdad, y por eso es lo primero que hay que verificar.

### El orden seguro

**Paso 0, y es el que decide todo: comprobá que podés entrar al bootloader ANTES de tocar
nada.** Se hace puenteando el pin de ISP del LPC11U35 y reseteando. Si funciona, la sonda se
re-enumera como un **disco USB** llamado `CRP DISABLD` con un `firmware.bin` adentro.

Este paso es **completamente reversible**: sacás el puente, reseteás, y vuelve a ser la
sonda de siempre. No se graba nada.

```bash
# con el puente puesto y despues del reset:
lsusb                      # ¿aparece un dispositivo distinto?
ls /dev/sd*                # ¿aparecio un disco nuevo?
```

**Si no llegás a ver el disco, no sigas.** Sin bootloader accesible no tenés red de
seguridad.

Recién después: conseguir una imagen construida para *esta* placa, grabarla arrastrando el
archivo al disco, y verificar que quedó en v2:

```bash
lsusb -d 1fc9: -v | grep bInterfaceClass    # ya no debería decir 3 (HID)
openocd -f interface/cmsis-dap.cfg -c init  # debería mencionar CMSIS-DAPv2
```

### Lo que pasó cuando lo probamos en la placa de la cátedra

El paso 0 se hizo, y **no entró al bootloader**. Vale la pena contarlo porque el intento
fallido enseña más que la teoría, y porque muestra cómo se ve el proceso desde afuera.

Monitoreando el bus USB con un script que registra cada cambio:

```
[12:46:56] <  1fc9:001d desaparece      ← se desenchufa el cable de la placa
[12:46:57] >  1fc9:001d vuelve
[12:47:06] <  1fc9:001d desaparece      ← segundo intento, con el puente puesto
[12:47:17] >  1fc9:001d vuelve
```

Las dos veces volvió como `NXP CMSIS-DAP`, o sea con el firmware normal, y **nunca apareció
un disco**. La sonda arranca de cero bien, pero el pin de ISP no está en bajo en ese momento.

Dos aprendizajes del camino:

- **Al principio se estaba desenchufando el cable equivocado.** Los eventos del bus eran
  todos del `10c4:ea60` (el conversor USB-serie del TP) mientras el `1fc9:001d` no se movía.
  Si la sonda no pierde alimentación, el LPC11U35 no arranca de cero y no mira el pin de ISP,
  por más bien puenteado que esté. La verificación es simple: **el que tiene que desaparecer
  de `lsusb` es el `1fc9`**.
- **Y la conclusión, que es la respuesta correcta del paso 0: no seguir.** Sin haber visto el
  bootloader funcionar no hay a dónde volver, y grabar firmware en ese estado es la única
  forma de convertir un riesgo teórico en un problema real. En esta revisión de placa el pin
  de ISP del *probe* no parece estar accesible.

Después de todo el proceso la sonda quedó intacta (`Cortex-M3 r2p0 processor detected`), que
es lo esperable: **el paso 0 no escribe nada**.

Para monitorear el bus mientras probás, alcanza con esto en otra terminal:

```bash
watch -n1 'lsusb | grep -E "1fc9|10c4"; ls /dev/sd* 2>/dev/null'
```

### Dos cosas que esta guía NO puede darte

Y es honesto decirlo en vez de improvisar:

1. **El punto exacto de entrada a ISP en tu revisión de placa.** Sale del esquemático o de
   la serigrafía. No te fíes de un número de jumper sacado de otra placa.
2. **Una imagen v2 verificada para la OM13085.** El proyecto **DAPLink** (el firmware de
   sonda abierto de ARM/Mbed) soporta el LPC11U35 como plataforma y sus versiones modernas
   hacen CMSIS-DAP v2 con SWO, pero *no está comprobado* que exista un build específico para
   esta placa. Usar el de otra placa es exactamente el escenario confuso de más arriba.

### La alternativa con riesgo cero, que es la que se recomienda

**No toques la sonda que funciona: sumá una segunda.**

- Una **Raspberry Pi Pico** con el firmware oficial `debugprobe` es CMSIS-DAP **v2**, sale
  muy poco, y grabarla es apretar BOOTSEL y arrastrar un archivo — también con bootloader en
  ROM, también imposible de brickear.
- Un **MCU-Link** de NXP (~US$10) es v2, tiene SWO y está oficialmente soportado, incluso
  por LinkServer ([guía 03](./03-lpc-link2-y-mcu-link.md)).

Cualquiera de las dos se enchufa en el conector Cortex de 10 pines (ver la sección
anterior) sin tocar una sola línea del firmware de la placa.

### Y la pregunta que casi nadie hace: ¿lo necesitás?

Para el curso, **no**. Lo que se gana con v2 ya está cubierto:

| Lo que daría v2 | Lo que ya tenés |
|---|---|
| Captura de SWO | **RTT**, que es más rápido y no gasta pines ([módulo 12 cap. 3](../../../12_debug/03-consola-por-el-debugger-rtt.md)) |
| Más caudal por el debugger | **UART a 921600 = 92 KB/s**, hoy, sin tocar nada ([módulo 9](../../../09_uart/)) |
| LinkServer andando | **OpenOCD**, que anda perfecto y es lo que usa todo el repo |

Convertir la única sonda que tenés para ganar algo que ya tenés por otro lado no es un buen
negocio. Si igual querés hacerlo, hacelo cuando tengas una segunda sonda como respaldo.

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
