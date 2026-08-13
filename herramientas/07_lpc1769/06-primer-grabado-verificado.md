# El primer grabado, verificado en la placa

Las páginas anteriores explican el mecanismo. Esta registra una sesión concreta con una LPCXpresso
LPC1769 OM13085 rev. D y una notebook con Ubuntu. Las salidas se conservan como evidencia del
montaje; no deben interpretarse como resultados garantizados para cualquier versión o placa.

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

Una instalación Linux general no suele incluir el cross-compiler `arm-none-eabi-gcc`. En esta
máquina ya estaba dentro de MCUXpresso, así que el script enlazó esa distribución sin descargar
otra:

```bash
bash tools/install_toolchain.sh --mcuxpresso
```

Eso deja un enlace en `tools/toolchain/`, donde lo busca la plantilla. GDB puede requerir una
instalación separada si el binario de MCUXpresso depende de bibliotecas ausentes en el sistema. Las
alternativas están en la
[página 03](./03-instalacion-linux.md).

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

Si la ruta dice `NO ENCONTRADO`, corregí la instalación o `CROSS` antes de intentar compilar.

## Paso 2: que la PC pueda hablar con la placa

La enumeración USB y los permisos son independientes de la compilación. En esta distribución, el
nodo se creó con acceso restringido y OpenOCD no pudo abrirlo como usuario normal.

Ejecutarlo como `root` puede confirmar el diagnóstico, pero no resuelve el acceso desde el editor.
La solución permanente es una regla udev adecuada:

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

| Antes de la regla | Después en esta máquina |
|---|---|
| `crw-rw-r-- root root` | `crw-rw----+ root plugdev` |

Otra distribución puede usar un grupo diferente o una ACL de `uaccess`. Lo importante es que el
usuario que ejecuta OpenOCD tenga los permisos necesarios.

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

La prueba usa el arranque mínimo. Sin `USE_CMSIS=1`, esta plantilla no configura la PLL mediante
`SystemInit()` y mantiene el reloj inicial de 4 MHz. Así se comprueban primero reset, vectores,
GPIO y grabación sin sumar el cristal externo ni la PLL. La prueba a 100 MHz queda para una segunda
etapa.

## Paso 4: los chequeos previos (sin la placa)

Antes de grabar se pueden detectar varias condiciones estructurales de la imagen:

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

La boot ROM suma las primeras ocho palabras de la tabla de vectores y exige que el resultado sea
cero. Si no encuentra código válido, inicia el autobaud de ISP por UART0 en lugar de ejecutar la
aplicación. La plantilla inyecta el checksum durante el build (`tools/lpc_checksum.py`).
Explicación completa en [05-02](../05_del_codigo_al_binario/02-linker-y-startup.md) y en
[07-02](./02-la-boot-rom-el-isp-y-el-checksum.md).

### 2. El stack pointer inicial

La primera palabra de la tabla. El Cortex-M3 la carga en el SP antes de ejecutar una sola
instrucción. Si no apunta a RAM válida, el primer `push` escribe en el aire. Tiene que caer
dentro de `0x10000000` a `0x10008000`.

### 3. El bit Thumb del Reset_Handler

El Cortex-M3 **solo** ejecuta Thumb-2, y lo señaliza con el bit 0 de la dirección de salto
en 1. Por eso el vector 1 vale `0x000001B1` y no `0x000001B0`: el `1` final no es parte de
la dirección, es el bit de modo. Si queda par, el procesador detecta un estado inválido al tomar el vector; con los faults
configurables deshabilitados, el error puede escalar a HardFault. El compilador suele generar el bit
correcto; el chequeo cubre tablas armadas o modificadas a mano.

### 4. La palabra de CRP

El LPC1769 interpreta tres patrones en `0x000002FC` como CRP1, CRP2 y CRP3. Los tres deshabilitan
el acceso de debug y restringen el ISP; CRP3 también impide forzar ISP con P2.10 cuando existe código
válido. La aplicación todavía puede prever actualización mediante IAP o invocar ISP, pero si no lo
hace puede no quedar una vía práctica de recuperación.

El valor `0x4E697370` (`NO_ISP`) aparece en documentación de otras familias y no es una opción
CRP del LPC1769 según UM10360. El chequeo no debe mezclar tablas de dispositivos distintos.

En este blink, la imagen terminaba en `0x260`, antes de `0x2FC`; la palabra permanecía borrada
como `0xFFFFFFFF`. Aun así se controla porque un cambio de tamaño o de linker script puede hacer
que la imagen alcance esa dirección. Si un curso no enseña CRP de forma intencional, el preflight
debe rechazar los tres patrones del LPC1769.

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

- **`SWD DPIDR 0x2ba01477`**: hubo comunicación con el debug port. Esto valida la cadena USB,
  sonda y SWD para esa operación; no descarta todos los problemas eléctricos o de la aplicación.
- **`Cortex-M3 r2p0`**: OpenOCD reconoció el núcleo esperado.
- **`pc: 0x1fff0080`**: en el instante del halt el PC estaba dentro de la boot ROM. Como todo reset
  pasa por esa ROM, esta lectura aislada no demuestra que la FLASH anterior fuera inválida.

En esta sesión tampoco apareció `Warning: checksum mismatch`. La imagen ya contenía el checksum
correcto, por lo que el flujo probado no necesitó modificar ese vector durante la programación.

## Paso 6: verificar que anda de verdad

`Verified OK` significa "los bytes quedaron escritos", **no** "el programa corre". Son
cosas distintas: una imagen con el checksum mal se graba y se verifica perfecto, y la placa
igual no arranca. Vale la pena mirar de verdad.

La observación inicial es el LED. Para obtener otra evidencia, se leyó el registro GPIO mientras el
programa estaba en ejecución:

```bash
openocd -f openocd/lpc1769.cfg \
  -c "init" \
  -c "mdw 0x2009C000" \
  -c "mdw 0x2009C014" -c "sleep 200" -c "mdw 0x2009C014" -c "sleep 200" \
  -c "mdw 0x2009C014" -c "exit"
```

```
0x2009c000: 00400000      <- FIO0DIR: el bit 22 en 1, el LED es salida
0x2009c014: 3fbf8fff      <- FIO0PIN: bit 22 en 0
0x2009c014: 3fff8fff      <- FIO0PIN: bit 22 en 1
0x2009c014: 3fbf8fff      <- y de vuelta
```

La diferencia es `0x00400000`, el bit 22. Esto confirma que el registro cambia; el nivel visible
del LED también depende de si está conectado de forma activa alta o activa baja.

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

La segunda lectura, tomada después de reanudar y luego detener la aplicación, sí ubica el PC dentro
de la imagen. Para distinguir de forma concluyente el camino de arranque, también se puede poner un
breakpoint en `Reset_Handler` o `main`.

Qué estaba haciendo el chip exactamente en cada uno de esos dos momentos, y todo lo que
pasó antes de llegar ahí, está en
[05-03 - El arranque paso a paso](../05_del_codigo_al_binario/03-el-arranque-paso-a-paso.md).

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

En la versión ensayada, seleccionar `--probe '#1'` terminó pasando igualmente un serial vacío al
motor. La alternativa verificada para esta combinación fue OpenOCD. Una versión nueva de LinkServer
o una sonda con otro descriptor puede comportarse distinto.

### pyOCD dice que no hay ninguna sonda, y sí la hay

```
$ pyocd list
No available debug probes are connected
```

Con la placa enchufada y funcionando. Dos causas posibles, y conviene descartarlas en este
orden:

1. **Entorno o backend HID.** Comprobá que estás ejecutando el pyOCD del entorno donde instalaste
   `hidapi`:
   ```bash
   python -m pip show pyocd hidapi
   ```
2. **Permisos u ocupación.** Revisá la regla udev y cerrá otras sesiones que puedan tener abierta la
   sonda.

`lsusb` solo confirma que el kernel enumeró el dispositivo; no demuestra que el proceso pueda
abrirlo.

### OpenOCD: `unable to find a matching CMSIS-DAP device`

```
Warn : could not read product string for device 0x1fc9:0x001d: Operation timed out
Error: unable to find a matching CMSIS-DAP device
```

El mensaje puede deberse a permisos, a otra aplicación usando la interfaz o a que el dispositivo no
responde. No se puede distinguir la causa solamente por esa línea.

### La sonda se cuelga y hay que reenchufarla

En esta sesión, después de varios intentos fallidos, la sonda dejó de responder hasta volver a
enumerarla. No se determinó desde el host si la causa fue el firmware, el driver o la herramienta.

Primero se cerraron los procesos que podían estar usando la interfaz y después se desconectó y
reconectó el cable de la sonda. Eso reinició el LPC11U35 y la conexión USB.

Al reconectar, la placa se re-enumera con otro número de dispositivo (`Device 032` pasa a
ser `Device 034`), lo cual es normal y no significa nada. Después de eso, OpenOCD conectó
a la primera.

## Tabla de síntomas

| Lo que ves | Lo que es |
|---|---|
| `arm-none-eabi-gcc: command not found` | falta el toolchain o no está en el PATH: `make info` |
| `cannot open linker script file nano.specs` | falta `libnewlib-arm-none-eabi` |
| `unable to find a matching CMSIS-DAP device` | faltan las reglas de udev, o no reenchufaste |
| `could not read product string ... timed out` | revisá procesos, permisos y reenumerá la sonda si no responde |
| `No available debug probes are connected` (pyocd) | revisá entorno Python, backend HID, permisos y ocupación |
| `Ee(E1). Probe serial number not found` | la versión probada de LinkServer no acepta el descriptor vacío; usá el flujo OpenOCD verificado |
| Graba y verifica, pero la aplicación no responde | ejecutá `make preflight` y diagnosticá reset, PC, clocks, GPIO y hardware |
| `Warning: checksum mismatch` en el verify | compará el vector 7 del archivo con el contenido programado |
| El PC aparece en `0x1fff0xxx` | está ejecutando ROM en ese instante; investigá ISP, IAP y momento del halt |
| Parpadea 25 veces más rápido de lo esperado | compilaste con `USE_CMSIS=1`: el core está a 100 MHz |

---

## Qué quedó probado

- Compilar sin MCUXpresso, con el toolchain que el propio MCUXpresso trae adentro.
- Los cinco chequeos previos, incluido el de CRP, sobre `.elf` y sobre `.bin`.
- Grabar y verificar con OpenOCD por el probe CMSIS-DAP de a bordo.
- Confirmar en los registros del chip que el programa corre y que el pin conmuta.
- Que el LED de la placa parpadea, que era la idea.

---

**LPC1769:** [índice](./README.md) ·
**Anterior:** [05 - Grabar y depurar](./05-grabar-y-depurar.md) ·
**Siguiente:** [07 - MCUXpresso por dentro](./07-mcuxpresso-por-dentro.md)
