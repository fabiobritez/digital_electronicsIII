# El primer grabado, verificado en la placa

Las páginas anteriores explican **cómo funciona** cada pieza. Esta es distinta: es el
registro de una sesión real, de punta a punta, con la LPCXpresso LPC1769 (OM13085 rev D)
enchufada al USB de una notebook con Ubuntu. Todo lo que está acá se ejecutó y funcionó,
incluidos los errores del camino, que son la parte que más tiempo ahorra.

El resultado: el LED de a bordo parpadeando, con el firmware compilado y grabado sin abrir
MCUXpresso en ningún momento.

> **Fecha de la prueba:** agosto de 2026. Toolchain `arm-none-eabi-gcc` 13.2.1, OpenOCD
> 0.12.0, Ubuntu con kernel 7.0.

## El resumen, si tenés apuro

```bash
bash tools/install_toolchain.sh --mcuxpresso   # una sola vez
sudo cp plantilla/tools/99-lpc-probes.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
# desenchufar y volver a enchufar la placa

cd plantilla
make          # compila
make flash    # chequea, graba y resetea
```

El resto de la página es qué hace cada cosa, qué verificar en cada paso, y qué hacer
cuando falla.

---

## Paso 1: que la PC tenga con qué compilar

El compilador para ARM no viene con Linux: hay que conseguirlo. Si ya tenés MCUXpresso
instalado, **ya lo tenés** y no hace falta bajar nada, porque el IDE trae adentro un
`arm-none-eabi-gcc` perfectamente normal:

```bash
bash tools/install_toolchain.sh --mcuxpresso
```

Eso deja enlaces en `tools/toolchain/`, que es justo donde la plantilla lo busca. La
alternativa (`sudo apt install gcc-arm-none-eabi ...`) sirve igual; las dos están en la
[página 06](./06-instalacion-linux.md).

**Verificá antes de seguir.** Este comando te dice qué encontró en tu máquina:

```bash
cd plantilla && make info
```

```
Configuracion actual
  compilador  : arm-none-eabi-gcc (Arm GNU Toolchain 13.2.rel1 ...) 13.2.1 20231009
  ruta        : ../tools/toolchain/bin/arm-none-eabi-gcc
  gdb         : ../tools/toolchain/bin/arm-none-eabi-gdb
  python      : /usr/bin/python3
  CMSIS       : no (bare metal)
  grabador    : openocd
  detectados  : openocd
```

Si en "ruta" dice `NO ENCONTRADO`, el compilador no está o no está en el PATH, y no tiene
sentido seguir. Es el momento de arreglarlo, no después de tres errores raros.

## Paso 2: que la PC pueda hablar con la placa

Este es **el paso que se saltea todo el mundo**, y el que causa la mayoría de los "no me
detecta la placa".

En Linux, un dispositivo USB recién enchufado pertenece a `root`. Tu usuario no lo puede
abrir. La reacción natural es `sudo openocd`, y anda, pero es la solución equivocada:
después F5 en VSCode no funciona (VSCode no corre como root) y terminás con archivos de
root en tu proyecto.

```bash
sudo cp plantilla/tools/99-lpc-probes.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Y **desenchufá y volvé a enchufar la placa**: las reglas se aplican cuando el dispositivo
se conecta, no a los que ya estaban.

Cómo verificar que funcionó, sin adivinar:

```bash
lsusb | grep -i cmsis
# Bus 001 Device 034: ID 1fc9:001d NXP Semiconductors NXP CMSIS-DAP
```

Ese `1fc9:001d` es el probe de a bordo. Ahora los permisos del nodo:

```bash
ls -l /dev/bus/usb/001/034
```

| Antes de la regla | Después |
|---|---|
| `crw-rw-r-- root root` | `crw-rw----+ root plugdev` |

Lo que importa es que el grupo pase a ser `plugdev` (o que aparezca el `+` de la ACL que
agrega `uaccess`). Con `root root` y sin permiso de escritura, **nada va a funcionar**, y
el síntoma no te lo va a decir: ver más abajo.

## Paso 3: compilar

```bash
cd plantilla
make
```

```
  CC      src/main.c
  CC      src/syscalls.c
  CC      startup/startup_lpc1769.c
  LD      build/firmware.elf
Memory region         Used Size  Region Size  %age Used
           FLASH:         608 B       512 KB      0.12%
             RAM:        2080 B        32 KB      6.35%
  CKSUM   build/firmware.elf
  OBJCOPY build/firmware.bin
  OBJCOPY build/firmware.hex
```

El programa que se compila es el blink de `src/main.c`: escribe los registros a mano, sin
CMSIS ni drivers.

**Se compila bare metal a propósito.** Sin `USE_CMSIS=1` no corre el `SystemInit()` que
configura la PLL, así que el micro se queda con el oscilador RC interno a 4 MHz. Es el
arranque más difícil de arruinar: no depende del cristal externo ni de que la PLL enganche.
Para el primer encendido de una placa, eso es exactamente lo que querés. Ya vas a subir a
100 MHz cuando sepas que el resto anda.

## Paso 4: los chequeos previos (sin la placa)

Acá está la idea central de esta página: **casi todo lo que hace que una placa no arranque
se puede detectar en el archivo, en la PC, antes de grabar.**

```bash
make preflight
```

```
Chequeos previos al grabado: build/firmware.elf [ELF]

  [ OK ]   checksum de la boot ROM      la suma de las 8 palabras da 0 (vector 7 = 0xEFFF75EE)
  [ OK ]   stack pointer inicial        0x10008000 (tope de la RAM: 0x10008000)
  [ OK ]   Reset_Handler                0x000001B1 (bit Thumb en 1, dentro de la FLASH)
  [ OK ]   CRP en 0x2FC                 la imagen termina en 0x260, no llega a esa palabra
  [ OK ]   tamano                       608 bytes de 524288 (0.12% de la FLASH)

  Todo en orden. Listo para grabar.
```

No hace falta acordarse de correrlo: **`make flash` lo ejecuta solo** antes de grabar, y
si algo falla, no graba. Qué mira cada uno y por qué:

### 1. El checksum de la boot ROM

La causa número uno del "grabé, dijo OK, y la placa no hace nada". La boot ROM suma las
primeras 8 palabras de la tabla de vectores y **exige que dé cero**; si no, decide que la
FLASH está vacía o corrupta y se queda en modo ISP esperando por la UART0, sin correr tu
programa. La plantilla lo inyecta en tiempo de compilación (`tools/lpc_checksum.py`).
Explicación completa en el [anexo A](../A_build_linker_startup/02-linker-y-startup.md).

### 2. El stack pointer inicial

La primera palabra de la tabla. El Cortex-M3 la carga en el SP antes de ejecutar una sola
instrucción. Si no apunta a RAM válida, el primer `push` escribe en el aire. Tiene que caer
dentro de `0x10000000` a `0x10008000`.

### 3. El bit Thumb del Reset_Handler

El Cortex-M3 **solo** ejecuta Thumb-2, y lo señaliza con el bit 0 de la dirección de salto
en 1. Por eso el vector 1 vale `0x000001B1` y no `0x000001B0`: el `1` final no es parte de
la dirección, es el bit de modo. Si queda par, el chip toma un UsageFault en la primera
instrucción. El compilador lo hace bien solo; el chequeo cubre el caso de armar la tabla a
mano en assembler.

### 4. La palabra de CRP: la única que puede arruinar la placa para siempre

Esta merece un párrafo aparte, porque es el único error de esta lista que **no tiene
vuelta atrás**.

El LPC1769 lee la palabra en `0x000002FC` al arrancar. Si encuentra uno de cuatro patrones
exactos, activa el *Code Read Protection*:

| Valor en `0x2FC` | Qué hace |
|---|---|
| `0x12345678` | CRP1: bloquea la lectura por SWD, el ISP sigue |
| `0x87654321` | CRP2: además limita los comandos de ISP |
| `0x43218765` | **CRP3: deshabilita SWD e ISP para siempre** |
| `0x4E697370` | NO_ISP: te quedás sin el bootloader de rescate |

Con CRP3 grabado, la placa no se puede volver a programar ni por SWD ni por el puerto
serie: queda corriendo para siempre lo último que le pusiste. No hay comando, jumper ni
herramienta que lo revierta.

La buena noticia es que el riesgo real es bajísimo: tiene que caer un valor exacto de 32
bits en una dirección exacta. Y en un programa chico como el blink **ni siquiera se graba
esa palabra**: la imagen termina en `0x260`, bastante antes de `0x2FC`, así que ahí queda
la FLASH borrada (`0xFFFFFFFF`), que no activa nada. Igual se chequea, porque el costo de
chequear es cero y el costo de equivocarse es una placa a la basura.

### 5. Que entre en la FLASH

Redundante con el linker, que ya aborta si no entra, pero cubre el caso de grabar un `.bin`
suelto que no pasó por este build.

## Paso 5: grabar

```bash
make flash
```

```
  FLASH   con openocd
Info : CMSIS-DAP: SWD supported
Info : CMSIS-DAP: FW Version = 1.0
Info : CMSIS-DAP: Interface Initialised (SWD)
Info : SWD DPIDR 0x2ba01477
Info : [lpc17xx.cpu] Cortex-M3 r2p0 processor detected
Info : [lpc17xx.cpu] target has 6 breakpoints, 4 watchpoints
[lpc17xx.cpu] halted due to debug-request, current mode: Thread
xPSR: 0x01000000 pc: 0x1fff0080 msp: 0x10001ffc
** Programming Started **
** Programming Finished **
** Verify Started **
** Verified OK **
** Resetting Target **
```

Tres cosas para leer de ahí, que la mayoría pasa por alto:

- **`SWD DPIDR 0x2ba01477`**: la sonda habló con el chip y el chip contestó con su
  identificador de debug port. Si llegaste acá, el hardware está bien. Todo lo que falle
  después es software.
- **`Cortex-M3 r2p0`**: es el núcleo correcto, no una placa distinta.
- **`pc: 0x1fff0080`**: el PC estaba dentro de la **boot ROM** (que vive en `0x1FFF0000`),
  no en tu código. O sea: antes de este grabado, la FLASH no tenía un programa válido, y el
  chip estaba en el bootloader. Es exactamente lo que se espera de una placa en blanco o
  con un checksum mal puesto.

Y una que **no** aparece: el aviso `Warning: checksum mismatch`. OpenOCD parchea el
checksum al escribir, así que lo grabado suele quedar distinto del archivo en disco y el
`verify` se queja. Como la plantilla ya lo inyectó en el build, no hay nada que parchear y
la verificación pasa limpia.

## Paso 6: verificar que anda de verdad

`Verified OK` significa "los bytes quedaron escritos", **no** "el programa corre". Son
cosas distintas: una imagen con el checksum mal se graba y se verifica perfecto, y la placa
igual no arranca. Vale la pena mirar de verdad.

La prueba más barata es el LED. La prueba que no depende de tus ojos, con el programa
corriendo:

```bash
openocd -f openocd/lpc1769.cfg \
  -c "init" \
  -c "mdw 0x2009C000" \
  -c "mdw 0x2009C014" -c "sleep 200" -c "mdw 0x2009C014" -c "sleep 200" \
  -c "mdw 0x2009C014" -c "exit"
```

```
0x2009c000: 00400000      <- FIO0DIR: el bit 22 en 1, el LED es salida
0x2009c014: 3fbf8fff      <- FIO0PIN: bit 22 en 0, LED apagado
0x2009c014: 3fff8fff      <- bit 22 en 1, LED encendido
0x2009c014: 3fbf8fff      <- y de vuelta
```

La diferencia entre `0x3fbf8fff` y `0x3fff8fff` es exactamente `0x00400000`, que es el bit
22. Está parpadeando, medido en el registro y no a ojo.

Y para confirmar dónde está ejecutando:

```bash
openocd -f openocd/lpc1769.cfg -c "init; halt; exit"
```

```
xPSR: 0x21000000 pc: 0x00000164 msp: 0x10007fe8
```

`pc: 0x00000164` cae dentro de la imagen (que va de `0x0` a `0x260`): está corriendo **tu
código**, no la boot ROM como antes de grabar. Y el MSP en `0x10007fe8`, justo debajo del
tope de RAM, confirma que el stack se inicializó donde correspondía.

Ese contraste, `0x1fff0080` antes y `0x00000164` después, es la forma más directa de
responder "¿está corriendo mi programa o quedó en el bootloader?".

Qué estaba haciendo el chip exactamente en cada uno de esos dos momentos, y todo lo que
pasó antes de llegar ahí, está en
[16 - El arranque paso a paso](../A_build_linker_startup/03-el-arranque-paso-a-paso.md).

---

## Los problemas que aparecieron de verdad

Esta parte no es hipotética: es lo que falló en esta sesión, en orden.

### LinkServer no funciona con el probe de la OM13085

Parece la opción obvia si ya tenés MCUXpresso instalado, y detecta la sonda bien:

```
$ LinkServer probes
  #  Description    Serial
---  -------------  --------
  1  NXP CMSIS-DAP
```

Pero fijate en la columna `Serial`: **está vacía**. El probe de esta placa declara el
descriptor USB `iSerial` con una cadena vacía, y LinkServer construye la llamada a su motor
de grabado pasándole ese serial vacío. Resultado:

```
Nc: Connecting to probe serial '' core 0 - Ee(E1). Probe serial number not found
Ed:02: Failed on connect: Ee(E1). Probe serial number not found
Et:31: No connection to chip's debug port
```

Pasarle el índice en vez del serial (`--probe '#1'`) **no lo arregla**: LinkServer sigue
mandando `--probeserial ''` por debajo. No hay forma de darle la vuelta desde la línea de
comandos.

**Conclusión: con esta placa, usá OpenOCD.** No pierdas la tarde con LinkServer. (Con
LPC-Link2 y MCU-Link, que sí reportan número de serie, LinkServer anda bien: ver la
[guía 03 de probes](./probes/03-lpc-link2-y-mcu-link.md).)

### pyOCD dice que no hay ninguna sonda, y sí la hay

```
$ pyocd list
No available debug probes are connected
```

Con la placa enchufada y funcionando. Dos causas posibles, y conviene descartarlas en este
orden:

1. **Falta el backend HID.** El probe de la OM13085 es CMSIS-DAP **v1**, que se comunica
   por HID, y pyocd necesita el módulo `hidapi` para eso. Sin él no ve ninguna sonda v1 y
   no te avisa por qué:
   ```bash
   pip install hidapi
   ```
2. **Faltan los permisos de udev.** Este es más traicionero. pyocd filtra los dispositivos
   buscando la cadena `CMSIS-DAP` en el nombre del producto, pero si no tenés permiso de
   escritura sobre el nodo USB, la biblioteca **no puede leer ese nombre**: lo devuelve
   vacío, pyocd no lo reconoce como sonda y lo descarta en silencio. El dispositivo aparece
   en `lsusb`, aparece en `hid.enumerate()`, y aun así pyocd insiste en que no hay nada.

Ese segundo caso es la razón por la que el paso 2 de esta página existe.

### OpenOCD: `unable to find a matching CMSIS-DAP device`

```
Warn : could not read product string for device 0x1fc9:0x001d: Operation timed out
Error: unable to find a matching CMSIS-DAP device
```

El mismo problema visto desde OpenOCD, con una diferencia importante: si el mensaje es
`Operation timed out` y ya instalaste las reglas de udev, entonces no es un problema de
permisos sino el siguiente.

### La sonda se cuelga y hay que reenchufarla

Después de un intento fallido de conexión (LinkServer peleándose con el serial vacío, o
pyocd sin permisos), el probe **queda trabado**. El síntoma es claro: algo que recién
funcionaba deja de funcionar. En esta sesión, `pyocd list` mostró la sonda con su ID único
y un minuto después decía "No available debug probes are connected", sin que nada cambiara.

**La solución es desenchufar el cable USB y volver a enchufarlo.** Sí, es "apagar y prender
de nuevo", pero acá tiene una explicación concreta: el firmware CMSIS-DAP del LPC11U35 se
queda esperando el final de una transacción que el host nunca completó, y el único que lo
saca de ahí es un reset del bus.

Al reconectar, la placa se re-enumera con otro número de dispositivo (`Device 032` pasa a
ser `Device 034`), lo cual es normal y no significa nada. Después de eso, OpenOCD conectó
a la primera.

## Tabla de síntomas

| Lo que ves | Lo que es |
|---|---|
| `arm-none-eabi-gcc: command not found` | falta el toolchain o no está en el PATH: `make info` |
| `cannot open linker script file nano.specs` | falta `libnewlib-arm-none-eabi` |
| `unable to find a matching CMSIS-DAP device` | faltan las reglas de udev, o no reenchufaste |
| `could not read product string ... timed out` | la sonda está trabada: reenchufá el cable |
| `No available debug probes are connected` (pyocd) | falta `hidapi`, o faltan los permisos de udev |
| `Ee(E1). Probe serial number not found` | LinkServer con una sonda sin número de serie: usá OpenOCD |
| Graba, dice `Verified OK`, y la placa no hace nada | el checksum del vector 7: `make preflight` |
| `Warning: checksum mismatch` en el verify | lo mismo al revés; con la plantilla no debería aparecer |
| El PC queda en `0x1fff0xxx` después de resetear | está en la boot ROM: no hay código de usuario válido |
| Parpadea 25 veces más rápido de lo esperado | compilaste con `USE_CMSIS=1`: el core está a 100 MHz |

---

## Qué quedó probado

- Compilar sin MCUXpresso, con el toolchain que el propio MCUXpresso trae adentro.
- Los cinco chequeos previos, incluido el de CRP, sobre `.elf` y sobre `.bin`.
- Grabar y verificar con OpenOCD por el probe CMSIS-DAP de a bordo.
- Confirmar en los registros del chip que el programa corre y que el pin conmuta.
- Que el LED de la placa parpadea, que era la idea.

---

**Anterior:** [07 - Instalación en Windows](./07-instalacion-windows.md) ·
**Módulo:** [18](./README.md) ·
**Volver al** [índice del curso](../../README.md)
