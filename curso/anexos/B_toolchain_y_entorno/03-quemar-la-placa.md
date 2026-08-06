# Quemar la placa (sin MCUXpresso)

Ya tenés el firmware compilado. Falta lo último: **meterlo en la Flash del micro** y, si
querés, **depurarlo en vivo**. MCUXpresso hace esto con un botón; acá ves los dos caminos para
hacerlo solo, y entendés qué pasa en cada uno.

> **Si solo querés grabar y seguir:** con la [plantilla](../../../plantilla/) es `make flash`,
> que detecta solo qué herramienta tenés instalada. Y si querés las instrucciones exactas
> para **tu** debug probe, andá directo a la [guía de probes](./probes/): hay una página por
> cada una. Esta página explica el mecanismo de fondo, que es lo que te va a servir cuando
> algo falle.

## Primero: ¿cómo se graba un micro?

Hay dos formas físicas de meter el firmware en la Flash del LPC1769:

1. **Por un debug probe (SWD/JTAG).** Un pequeño hardware (el *probe*) se conecta a
   los pines SWD del micro y escribe la Flash directamente. Es el método pro: además de grabar, te
   deja **depurar** (breakpoints, ver registros). Las placas LPCXpresso traen un probe **a bordo**
   (LPC-Link / LPC-Link2); también sirven probes externos como **J-Link** o **ST-Link**.
2. **Por el bootloader serial (ISP).** El LPC1769 trae de fábrica, en una ROM interna, un
   **bootloader** que sabe recibir el firmware por la **UART0** y grabarlo. No necesita ningún probe:
   solo un adaptador **USB-serial**. No permite depurar, solo grabar.

En los dos casos la Flash no se escribe byte a byte: se **borra por sectores** (el LPC1769 tiene
30: los primeros 16 de 4 KB y los últimos 14 de 32 KB, capítulo 32 del manual) y se **escribe por
bloques** de 256/512/1024/4096 bytes. Las herramientas lo manejan solas: de hecho, hasta el probe
termina usando el chip: copia los datos a la RAM y llama a las rutinas **IAP** (*In-Application
Programming*) de la boot ROM para que escriban la Flash.

Veamos cada uno.

---

## Camino A: probe SWD (OpenOCD o pyOCD)

Necesitás un probe. En las placas LPCXpresso el probe está integrado y aparece como un dispositivo
**CMSIS-DAP** al enchufar el USB. Hay dos programas de PC que hablan con el probe y graban la Flash:

### Opción 1: pyOCD (el más simple)

`pyocd` es una herramienta en Python, fácil de instalar y usar:

```bash
pip install pyocd            # instalar (en un venv, como hicimos con el toolchain)
pyocd list                   # ver qué probes detecta
pyocd flash -W -t lpc1768 build/firmware.hex
```

- `-t lpc1768` es el **target**: el LPC1768/1769 son la misma familia, ese target sirve. (Para el
  match exacto: `pyocd pack install LPC1769` y usás `-t lpc1769`.)
- Acepta `.bin`, `.hex` o `.elf`. Con `.bin` asume que va al inicio de la Flash (`0x0`; se cambia
  con `--base-address` si hiciera falta); con `.elf`/`.hex` la dirección ya viene incluida.
- Resetea y arranca el programa al terminar.

> Si `pyocd list` te dice `No available debug probes are connected` con la placa enchufada,
> instalá `hidapi` (`pip install hidapi`): el probe de las LPCXpresso es CMSIS-DAP **v1**, que
> habla por HID, y sin ese módulo pyocd no ve ninguna sonda v1 y no te explica por qué. Si aun
> así no aparece, son los permisos de udev (paso 2 de la
> [página 08](./08-primer-grabado-verificado.md)).

### Opción 2: OpenOCD (el estándar de la industria)

`openocd` es más potente (y más usado en CI). Se le pasan dos archivos de configuración: el de la
**interfaz** (el probe) y el del **target** (el chip):

```bash
openocd -f interface/cmsis-dap.cfg -f target/lpc17xx.cfg \
        -c "program build/firmware.elf verify reset exit"
```

Desglosado:
- `interface/cmsis-dap.cfg`: el probe es CMSIS-DAP (el de las LPCXpresso). Si usás un J-Link sería
  `interface/jlink.cfg`; un ST-Link, `interface/stlink.cfg`.
- `target/lpc17xx.cfg`: el chip es un LPC17xx.
- `program ... verify reset exit`: graba, **verifica** que quedó bien escrito, **resetea** y sale.

> Estos `.cfg` vienen incluidos con OpenOCD. La diferencia entre probes es **solo** el archivo de
> interfaz: el flujo es el mismo. Por eso OpenOCD sirve para casi cualquier combinación de probe +
> chip.

### Opción 3: LinkServer (el que ya tenés, si instalaste MCUXpresso)

MCUXpresso instala **LinkServer**, el grabador nativo de NXP, como un paquete **separado del IDE**.
O sea que lo podés usar desde la terminal sin abrir Eclipse:

```bash
LS=/usr/local/LinkServer_1.6.133          # ajustá la version

$LS/LinkServer probes                     # ver los probes conectados
$LS/LinkServer flash LPC1769 load app.axf # grabar (.axf, .elf, .hex, .s19 o .bin con --addr)
$LS/LinkServer flash LPC1769 verify app.axf
$LS/LinkServer gdbserver LPC1769          # servidor gdb, igual que OpenOCD
```

Soporta LPC-Link, LPC-Link2, MCU-Link y cualquier probe CMSIS-DAP que **reporte un número de
serie**. En la [página 05](./05-como-compila-y-graba-mcuxpresso.md) se explica cómo funciona por
dentro.

> **Ojo: con la placa de la cátedra (OM13085) esto no funciona.** Su probe declara el descriptor
> USB `iSerial` vacío, y LinkServer le pasa ese serial vacío a su motor de grabado, que corta con
> `Ee(E1). Probe serial number not found`. Pasarle el índice (`--probe '#1'`) tampoco lo salva.
> Está probado y documentado en la [página 08](./08-primer-grabado-verificado.md): para esa placa,
> usá **OpenOCD**.

### Opción 4: J-Link

Si tenés un probe SEGGER (o MCUXpresso te instaló el paquete J-Link):

```bash
JLinkExe -device LPC1769 -if SWD -speed 4000
J-Link> loadfile app.hex
J-Link> r        # reset
J-Link> g        # go
```

---

## Camino B: bootloader serial (ISP), sin probe

Si no tenés probe, el LPC1769 se graba con un **adaptador USB-serial** ($2) usando su bootloader de
fábrica. Pasos:

1. **Conectar la UART0:** TX del adaptador → RXD0 (P0.3), RX → TXD0 (P0.2), GND común.
2. **Entrar en modo ISP:** mantener **P2.10 en bajo** (a GND) durante el **reset**, y un instante
   más al soltarlo: el bootloader muestrea el pin hasta ~3 ms después del reset. Si lo ve bajo,
   arranca el modo ISP en lugar de tu programa. (P2.10 es el pin de entrada a ISP; en muchas placas
   hay un botón "ISP" que hace justo esto.)
3. **Grabar con una herramienta de ISP:**

   **lpc21isp** (línea de comandos, abierto, multiplataforma):
   ```bash
   lpc21isp -control build/firmware.hex /dev/ttyUSB0 115200 12000
   ```
   - `firmware.hex` → lpc21isp trabaja con **Intel HEX**, no `.bin`. Generalo con
     `arm-none-eabi-objcopy -O ihex firmware.elf firmware.hex` (el Makefile de la plantilla ya lo genera).
   - `/dev/ttyUSB0` → el puerto del adaptador (en Windows sería `COM3`, etc.).
   - `115200` → baudrate a usar. El bootloader no tiene uno fijo: lo **detecta** midiendo el primer
     carácter que le manda la herramienta (*auto-baud*).
   - `12000` → la frecuencia del **cristal en kHz** (12 MHz). El bootloader la necesita para sus
     cuentas internas.
   - `-control` → usa las líneas RTS/DTR del adaptador para resetear y entrar a ISP **automáticamente**
     (si la placa está cableada para eso; si no, hacés el reset+P2.10 a mano).

   **FlashMagic** (GUI, Windows): la versión con interfaz gráfica de lo mismo. Elegís el chip
   (LPC1769), el puerto COM, el `.hex`, y "Start".

> El bootloader ISP **siempre está** en la ROM del chip: no se puede borrar. Es la red de seguridad
> para cuando no tenés probe o "brickearías" la placa.

---

## ¿Cuál uso?

| | Probe SWD (OpenOCD/pyOCD) | ISP serial (lpc21isp/FlashMagic) |
|--|--------------------------|----------------------------------|
| Hardware extra | un probe (o el de a bordo en LPCXpresso) | un adaptador USB-serial barato |
| Permite **depurar** | **sí** (breakpoints, registros) | no, solo grabar |
| Velocidad | rápida | más lenta |
| Recomendado para | el día a día (desarrollo + debug) | cuando no tenés probe, o producción simple |

Para la materia, si tu placa tiene probe a bordo (la mayoría de las LPCXpresso), usá **camino A**: te
da grabar **y** depurar.

---

## Depurar con gdb (lo que F5 hace por dentro)

El probe + OpenOCD también te dan un **depurador**. OpenOCD levanta un "servidor gdb" y `arm-none-eabi-gdb`
se conecta:

```bash
# Terminal 1: OpenOCD como servidor gdb (queda escuchando en el puerto 3333)
openocd -f interface/cmsis-dap.cfg -f target/lpc17xx.cfg

# Terminal 2: gdb, conectándose
gdb-multiarch build/firmware.elf   # o arm-none-eabi-gdb, si tu toolchain lo trae
(gdb) target remote :3333      # conectar a OpenOCD
(gdb) load                     # grabar el firmware
(gdb) break main               # poner un breakpoint
(gdb) continue                 # correr hasta el breakpoint
(gdb) print contador           # ver una variable
(gdb) monitor reg              # ver registros del CPU
```

Esto es **exactamente** lo que hace MCUXpresso (o el botón **F5** de VSCode con Cortex-Debug): por
debajo corre OpenOCD + gdb. La extensión Cortex-Debug (módulo 12 y página anterior) te pone una
interfaz gráfica encima, con la configuración en `launch.json`. Cuando apretás F5, VSCode:

1. corre la tarea de build (compila),
2. lanza OpenOCD/pyOCD,
3. conecta gdb, graba el firmware,
4. para en `main` y te deja depurar con clicks.

Todo con piezas abiertas que ahora conocés una por una.

## Resumen del módulo

- **Compilar** = el toolchain (`arm-none-eabi-gcc` + binutils), página 01. Se hace en cualquier PC.
- **Editar cómodo** = VSCode + extensiones C/C++ y Cortex-Debug, página 02.
- **Grabar** = probe SWD (OpenOCD/pyOCD) o bootloader ISP serial (lpc21isp/FlashMagic), esta página.
- **Depurar** = OpenOCD + gdb (o F5 en VSCode), igual que MCUXpresso pero destapado.

Ya no dependés de ningún IDE: entendés y controlás cada paso, de tu `.c` hasta los bits en la Flash.

> **Nota:** los comandos de grabado y debug requieren la **placa física** conectada. El compilar y
> linkear (lo que hicimos en todo el curso) no necesita hardware.

---

**Anterior:** [02 - Setup en VSCode](./02-setup-vscode.md) ·
**Siguiente:** [04 - Adentro del toolchain](./04-adentro-del-toolchain.md) ·
**Volver al** [índice del curso](../../README.md)
