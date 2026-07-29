# Quemar la placa (sin MCUXpresso)

Ya tenés el `mygpio.bin`/`.elf` compilado. Falta lo último: **meterlo en la Flash del micro** y, si
querés, **depurarlo en vivo**. MCUXpresso hace esto con un botón; acá ves los dos caminos para
hacerlo solo, y entendés qué pasa en cada uno.

## Primero: ¿cómo se graba un micro?

Hay dos formas físicas de meter el firmware en la Flash del LPC1769:

1. **Por una sonda de depuración (SWD/JTAG).** Un pequeño hardware (la "sonda" o *probe*) se conecta a
   los pines SWD del micro y escribe la Flash directamente. Es el método pro: además de grabar, te
   deja **depurar** (breakpoints, ver registros). Las placas LPCXpresso traen una sonda **a bordo**
   (LPC-Link / LPC-Link2); también sirven sondas externas como **J-Link** o **ST-Link**.
2. **Por el bootloader serial (ISP).** El LPC1769 trae de fábrica, en una ROM interna, un
   **bootloader** que sabe recibir el firmware por la **UART0** y grabarlo. No necesita ninguna sonda:
   solo un adaptador **USB-serial**. No permite depurar, solo grabar.

En los dos casos la Flash no se escribe byte a byte: se **borra por sectores** (el LPC1769 tiene
30: los primeros 16 de 4 KB y los últimos 14 de 32 KB, capítulo 32 del manual) y se **escribe por
bloques** de 256/512/1024/4096 bytes. Las herramientas lo manejan solas: de hecho, hasta la sonda
termina usando el chip: copia los datos a la RAM y llama a las rutinas **IAP** (*In-Application
Programming*) de la boot ROM para que escriban la Flash.

Veamos cada uno.

---

## Camino A: sonda SWD (OpenOCD o pyOCD)

Necesitás una sonda. En las placas LPCXpresso la sonda está integrada y aparece como un dispositivo
**CMSIS-DAP** al enchufar el USB. Hay dos programas de PC que hablan con la sonda y graban la Flash:

### Opción 1: pyOCD (el más simple)

`pyocd` es una herramienta en Python, fácil de instalar y usar:

```bash
pip install pyocd            # instalar (en un venv, como hicimos con el toolchain)
pyocd list                   # ver qué sondas detecta
pyocd flash -t lpc1768 curso/02_arma_tu_propia_libreria/src/build/mygpio.bin
```

- `-t lpc1768` es el **target**: el LPC1768/1769 son la misma familia, ese target sirve. (Para el
  match exacto: `pyocd pack install LPC1769` y usás `-t lpc1769`.)
- Acepta `.bin`, `.hex` o `.elf`. Con `.bin` asume que va al inicio de la Flash (`0x0`; se cambia
  con `--base-address` si hiciera falta); con `.elf`/`.hex` la dirección ya viene incluida.
- Resetea y arranca el programa al terminar.

### Opción 2: OpenOCD (el estándar de la industria)

`openocd` es más potente (y más usado en CI). Se le pasan dos archivos de configuración: el de la
**interfaz** (la sonda) y el del **target** (el chip):

```bash
openocd -f interface/cmsis-dap.cfg -f target/lpc17xx.cfg \
        -c "program curso/02_arma_tu_propia_libreria/src/build/mygpio.elf verify reset exit"
```

Desglosado:
- `interface/cmsis-dap.cfg`: la sonda es CMSIS-DAP (la de las LPCXpresso). Si usás un J-Link sería
  `interface/jlink.cfg`; un ST-Link, `interface/stlink.cfg`.
- `target/lpc17xx.cfg`: el chip es un LPC17xx.
- `program ... verify reset exit`: graba, **verifica** que quedó bien escrito, **resetea** y sale.

> Estos `.cfg` vienen incluidos con OpenOCD. La diferencia entre sondas es **solo** el archivo de
> interfaz: el flujo es el mismo. Por eso OpenOCD sirve para casi cualquier combinación de sonda +
> chip.

---

## Camino B: bootloader serial (ISP), sin sonda

Si no tenés sonda, el LPC1769 se graba con un **adaptador USB-serial** ($2) usando su bootloader de
fábrica. Pasos:

1. **Conectar la UART0:** TX del adaptador → RXD0 (P0.3), RX → TXD0 (P0.2), GND común.
2. **Entrar en modo ISP:** mantener **P2.10 en bajo** (a GND) durante el **reset**, y un instante
   más al soltarlo: el bootloader muestrea el pin hasta ~3 ms después del reset. Si lo ve bajo,
   arranca el modo ISP en lugar de tu programa. (P2.10 es el pin de entrada a ISP; en muchas placas
   hay un botón "ISP" que hace justo esto.)
3. **Grabar con una herramienta de ISP:**

   **lpc21isp** (línea de comandos, abierto, multiplataforma):
   ```bash
   lpc21isp -control mygpio.hex /dev/ttyUSB0 115200 12000
   ```
   - `mygpio.hex` → lpc21isp trabaja con **Intel HEX**, no `.bin`. Generalo con
     `arm-none-eabi-objcopy -O ihex mygpio.elf mygpio.hex`.
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
> para cuando no tenés sonda o "brickearías" la placa.

---

## ¿Cuál uso?

| | Sonda SWD (OpenOCD/pyOCD) | ISP serial (lpc21isp/FlashMagic) |
|--|--------------------------|----------------------------------|
| Hardware extra | una sonda (o la de a bordo en LPCXpresso) | un adaptador USB-serial barato |
| Permite **depurar** | **sí** (breakpoints, registros) | no, solo grabar |
| Velocidad | rápida | más lenta |
| Recomendado para | el día a día (desarrollo + debug) | cuando no tenés sonda, o producción simple |

Para la materia, si tu placa tiene sonda a bordo (la mayoría de las LPCXpresso), usá **camino A**: te
da grabar **y** depurar.

---

## Depurar con gdb (lo que F5 hace por dentro)

La sonda + OpenOCD también te dan un **depurador**. OpenOCD levanta un "servidor gdb" y `arm-none-eabi-gdb`
se conecta:

```bash
# Terminal 1: OpenOCD como servidor gdb (queda escuchando en el puerto 3333)
openocd -f interface/cmsis-dap.cfg -f target/lpc17xx.cfg

# Terminal 2: gdb del toolchain, conectándose
arm-none-eabi-gdb mygpio.elf
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
- **Grabar** = sonda SWD (OpenOCD/pyOCD) o bootloader ISP serial (lpc21isp/FlashMagic), esta página.
- **Depurar** = OpenOCD + gdb (o F5 en VSCode), igual que MCUXpresso pero destapado.

Ya no dependés de ningún IDE: entendés y controlás cada paso, de tu `.c` hasta los bits en la Flash.

> **Nota:** los comandos de grabado y debug requieren la **placa física** conectada. El compilar y
> linkear (lo que hicimos en todo el curso) no necesita hardware.

---

**Anterior:** [02 - Setup en VSCode](./02-setup-vscode.md) ·
**Volver al** [índice del curso](../README.md)
