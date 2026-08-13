# La placa y su sonda

La placa usada por la cátedra es una **LPCXpresso LPC1769 rev. D** (código **OM13085**). Su
sonda integrada puede utilizarse con herramientas abiertas como OpenOCD y pyOCD, sin depender de
MCUXpresso para grabar o depurar.

Esta página reúne lo necesario para trabajar con la sonda integrada: qué es, cómo
confirmar qué versión de firmware corre, qué implica esa versión y por qué **no** conviene
intentar actualizarla sin un procedimiento específico para este modelo.

> La teoría de fondo está en [03 - Debug probes](../03_debug_probes/):
> [qué es una sonda](../03_debug_probes/01-un-probe-es-otro-micro.md) y
> [qué es CMSIS-DAP](../03_debug_probes/02-cmsis-dap.md). Acá bajamos a esta placa concreta.

## Qué tenés exactamente

En un rincón de la placa, del lado del conector USB, hay un **segundo microcontrolador**:
un **LPC11U35**. No ejecuta tu aplicación: corre un firmware CMSIS-DAP que traduce las
operaciones de la PC a accesos de debug sobre el LPC1769.

```
        ┌─────────────────── placa OM13085 ───────────────────┐
        │                          ┊                          │
 USB ───┤  LPC11U35   ── SWD ──►   ┊   LPC1769                │
        │  (el probe)  SWDIO/SWCLK ┊   (tu micro)             │
        │                          ┊                          │
        └──────────────────────────┴──────────────────────────┘
                       línea de corte / jumpers
```

CMSIS-DAP es una especificación pública de Arm. La placa puede funcionar con las herramientas que
admitan su transporte USB HID y el LPC1769. Esto la diferencia de algunas LPCXpresso anteriores
([guía de la LPC-Link vieja](./probes/01-lpc-link-original.md)), que traían un probe propietario.

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
| `bInterfaceClass 3` (HID) | usa el transporte USB HID conocido como CMSIS-DAP v1 |
| 2 endpoints interrupt, 64 B, `bInterval 1` | tamaño e intervalo declarados por esta interfaz |
| `iSerial` vacío | el dispositivo no publica número de serie |
| `bNumInterfaces 1` | esta enumeración expone solamente la interfaz HID |

Y OpenOCD lo corrobora por otro lado:

```console
$ openocd -f interface/cmsis-dap.cfg -c "transport select swd" -c init -c exit
Info : CMSIS-DAP: SWD supported
Info : CMSIS-DAP: FW Version = 1.0
```

La salida tampoco anuncia `SWO-UART supported` y, al intentar iniciar la traza, OpenOCD informa
que la sonda no la admite. Es una limitación de este firmware, no de todo CMSIS-DAP v1: los comandos
SWO existen desde la revisión 1.1 y su implementación es opcional.

Consecuencias prácticas de esta unidad concreta:

1. pyOCD necesita un backend HID compatible y permisos sobre el dispositivo;
2. para una consola sin UART se puede usar
   [RTT](../06_depurar_en_serio/04-consola-por-el-debugger-rtt.md), que lee RAM por SWD;
3. con RTT se midió un techo cercano a 15,7 kB/s. El resultado corresponde a esta sonda y al host
   probados; no es un límite del LPC1769 ni de RTT;
4. el descriptor de número de serie vacío causa problemas con la versión de LinkServer ensayada
   más abajo.

En Windows, la interfaz HID suele usar el controlador incluido con el sistema. La herramienta de
debug todavía debe reconocer CMSIS-DAP y tener acceso al dispositivo.

## Grabar

Cualquiera de las dos herramientas abiertas sirve. Con la
[plantilla](../../plantilla/):

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

El archivo [`openocd/lpc1769.cfg`](../../plantilla/openocd/lpc1769.cfg) de la plantilla
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

En la versión probada de pyOCD, el target integrado `lpc1768` puede programar esta variante porque
comparte la geometría de FLASH necesaria para el algoritmo. Es preferible instalar la descripción
específica cuando esté disponible:

```bash
pyocd pack install LPC1769
pyocd flash -W -t lpc1769 build/firmware.hex
```

La opción `-W` evita que pyOCD espere indefinidamente a que aparezca una sonda.

## Depurar

```bash
make debug
```

La receta de la plantilla levanta el servidor, graba y abre GDB detenido en `main`. Desde VSCode
se inicia con **F5**
(configuración "Debug con OpenOCD" de
[`launch.json`](../../plantilla/.vscode/launch.json)).

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

La placa tiene un conector Cortex de 10 pines para una sonda externa y enlaces que permiten aislar
la integrada. Antes de conectar otra, verificá en el esquemático de tu revisión cómo se desconectan
SWDIO, SWCLK y RESET, y comprobá la orientación, GND y VTref. Después seguí la guía de la sonda
elegida.

## ¿Conviene actualizar la sonda integrada?

Para usar la placa en el curso, no hace falta. La sonda actual permite grabar, detener el núcleo,
usar breakpoints y leer memoria con OpenOCD. Cambiarle el firmware agrega un problema de
recuperación a una pieza que ya funciona.

Además, conviene separar tres conceptos:

- una versión nueva del protocolo CMSIS-DAP puede agregar comandos;
- el transporte USB bulk de CMSIS-DAP v2 puede mejorar el intercambio con el host;
- SWO, reset, VCOM y otras funciones siguen siendo capacidades opcionales del firmware y del
  hardware.

Por eso no se puede concluir que una imagen “v2” vaya a dar más caudal, SWO o compatibilidad con
LinkServer en esta placa. Hace falta una imagen construida para su LPC11U35, sus pines y sus
interfaces USB.

### La condición de recuperación

El LPC11U35 tiene un bootloader en ROM, pero para aprovecharlo hay que poder forzar el modo ISP desde
la placa. Antes de escribir cualquier imagen deberían estar resueltos estos puntos:

1. esquemático correspondiente a la revisión exacta;
2. punto o jumper que lleva el pin ISP del **LPC11U35** al nivel requerido durante reset;
3. aparición comprobada del dispositivo de bootloader;
4. imagen específica y una copia verificable del firmware original;
5. una segunda sonda o método de recuperación.

Probar solamente la entrada al bootloader es reversible: no modifica la FLASH. En la OM13085
ensayada no apareció el dispositivo de almacenamiento esperado y la sonda volvió a enumerar con su
firmware normal. Eso significa que **no se verificó una ruta de recuperación accesible**, así que no
se continuó con la actualización. La conclusión vale para esa unidad y ese punto de acceso; no
reemplaza el esquemático de otra revisión.

### Alternativa: una sonda externa

Si necesitás otra interfaz USB, más rendimiento o una capacidad de traza concreta, es más seguro
conectar una sonda externa y conservar la integrada como respaldo. La Raspberry Pi Debug Probe
oficial, por ejemplo, brinda CMSIS-DAP v2, SWD y UART, pero no documenta captura SWO. Para SWO elegí
un modelo que lo indique de forma explícita.

Antes de conectarla, aislá la sonda de a bordo como indique el esquemático y verificá VTref, GND,
SWDIO, SWCLK y RESET. No alimentes el target desde dos fuentes sin entender cómo están unidas.

## Lo que NO funciona con esta sonda: LinkServer

En la versión ensayada de LinkServer, la sonda aparece en la lista pero no puede abrirse porque el
descriptor de número de serie está vacío:

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

En esa versión, seleccionar `--probe '#1'` produjo el mismo resultado porque el motor recibió un
serial vacío. Es un resultado del software y el firmware probados, no una regla general de
LinkServer. Para esta placa, la ruta verificada es OpenOCD; está documentada de punta a punta en la
[página 06](./06-primer-grabado-verificado.md).

## Problemas típicos

**`unable to find a matching CMSIS-DAP device` (Linux)**

Permisos de USB. Instalá las reglas de udev:

```bash
sudo cp plantilla/tools/99-lpc-probes.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
```

Después desenchufá y volvé a enchufar la placa. Usar `sudo openocd` puede confirmar que el
problema es de permisos, pero no es la solución permanente.

Para verificar la regla, inspeccioná el nodo USB. Según la distribución puede aparecer un grupo como
`plugdev` o una ACL de `uaccess`:

```bash
ls -l /dev/bus/usb/001/034      # el número sale de lsusb
```

**`pyocd list` dice `No available debug probes are connected`, con la placa enchufada**

Comprobá que el entorno de Python tenga instalado el backend HID requerido por tu versión de pyOCD
y revisá los permisos udev. Ejecutá `python -m pip show hidapi pyocd` con el mismo intérprete desde
el que invocás `pyocd`.

**Funcionaba y de golpe dejó de aparecer**

Después de algunos intentos fallidos se observó que la sonda dejó de responder. Un síntoma posible
en OpenOCD es:

```
Warn : could not read product string for device 0x1fc9:0x001d: Operation timed out
```

Cerrá las herramientas que estén usando la sonda y desconectá y reconectá su cable USB. Así se
reinician el LPC11U35 y la enumeración; es normal que cambie el número de dispositivo.

**Graba bien pero la placa no hace nada**

Comprobá primero el **checksum del vector 7**: la boot ROM exige que la suma de las primeras ocho
palabras de la tabla dé cero. Algunas herramientas pueden corregirlo al programar, pero depender de
eso vuelve distinto cada flujo. La plantilla lo inyecta durante el build y lo verifica antes de
grabar:

```bash
make preflight     # el checksum y todo lo demás que impide arrancar
make vectores      # la tabla de vectores en crudo, si querés verla
```

Para saber si el chip está corriendo tu programa o quedó en el bootloader, mirá dónde está
el PC:

```bash
openocd -f openocd/lpc1769.cfg -c "init; halt; exit"
```

Un PC en `0x1fff0xxx` indica que el núcleo está ejecutando la ROM. Puede tratarse del ISP, de una
rutina IAP o de un instante del arranque; interpretalo junto con el pin ISP, el checksum y el estado
de reset. Una dirección dentro de la FLASH de usuario indica que ya salió de la ROM.

El detalle completo está en
[`tools/lpc_checksum.py`](../../plantilla/tools/lpc_checksum.py) y en el
[05 - El linker script y el startup](../05_del_codigo_al_binario/02-linker-y-startup.md).

**`Warning: checksum mismatch` al hacer verify con openocd**

Lo mismo al revés: openocd parchea el checksum al escribir, así que lo grabado queda
distinto del archivo en disco y la verificación se queja. Inyectándolo en el build (lo que
hace la plantilla) el aviso desaparece.

**`Error: Debug adapter doesn't support any transports`**

Confirmá que OpenOCD fue compilado con soporte CMSIS-DAP y que cargaste
`interface/cmsis-dap.cfg`. Consultá `openocd --version` y los adaptadores disponibles antes de
reemplazar paquetes.

**Se desconecta en medio de la grabación**

Bajá la velocidad en `openocd/lpc1769.cfg`: cambiá `adapter speed 1000` por `500` o `100`.

---

**LPC1769:** [índice](./README.md) ·
**Siguiente:** [02 - La boot ROM, el ISP y el checksum](./02-la-boot-rom-el-isp-y-el-checksum.md)
