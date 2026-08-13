# Grabar y depurar el LPC1769

Una vez compilado el firmware hay que programar la FLASH y, durante el desarrollo, suele hacer falta
una sesión de debug. Esta página compara los dos caminos usados con la placa y muestra qué
herramienta interviene en cada uno.

> **Si solo querés grabar y seguir:** con la [plantilla](../../plantilla/) es `make flash`,
> que detecta solo qué herramienta tenés instalada. Y si querés las instrucciones exactas
> para **tu** sonda, andá directo a la [guía por sonda](./probes/): hay una página por cada
> una. Esta página explica el mecanismo de fondo, que es lo que te va a servir cuando algo
> falle.
>
> Si la grabación se verifica pero la aplicación no responde, continuá con la
> [página de boot ROM y checksum](./02-la-boot-rom-el-isp-y-el-checksum.md) y con el método de
> diagnóstico del módulo 6.

## Primero: ¿cómo se graba un micro?

Para el trabajo de desarrollo de esta unidad usamos dos vías principales:

1. **Mediante una sonda de debug por SWD o JTAG.** La sonda transporta comandos y datos; el servidor
   ejecuta un algoritmo de programación en el target. Además permite detener el núcleo, poner
   breakpoints e inspeccionar memoria. Algunas LPCXpresso incluyen una sonda y también pueden usarse
   modelos externos compatibles.
2. **Por el bootloader serial (ISP).** El LPC1769 trae de fábrica, en una ROM interna, un
   **bootloader** que sabe recibir el firmware por la **UART0** y grabarlo. No necesita ningún probe:
   solo un adaptador **USB-serial**. No permite depurar, solo grabar.

La FLASH se borra por sectores (16 de 4 KiB y 14 de 32 KiB) y las rutinas IAP aceptan tamaños de
copia definidos por el manual. La herramienta organiza esos detalles y carga un algoritmo en RAM;
ese algoritmo puede invocar las rutinas IAP de la ROM. La sonda no escribe celdas de FLASH de forma
directa.

Veamos cada uno.

---

## Camino A: probe SWD (OpenOCD o pyOCD)

Necesitás un probe. En las placas LPCXpresso el probe está integrado y aparece como un dispositivo
**CMSIS-DAP** al enchufar el USB. Hay dos programas de PC que hablan con el probe y graban la Flash:

### Opción 1: pyOCD

pyOCD es una herramienta de Python. Instalalo en un entorno virtual como se explica en las páginas
de instalación:

```bash
pyocd list                   # ver qué probes detecta
pyocd flash -W -t lpc1768 build/firmware.hex
```

- En la versión probada, el target integrado `lpc1768` usa una geometría de FLASH compatible con
  esta variante. Cuando sea posible, instalá el pack y seleccioná `lpc1769` para no depender de
  esa equivalencia.
- Acepta `.bin`, `.hex` o `.elf`. Con `.bin` asume que va al inicio de la Flash (`0x0`; se cambia
  con `--base-address` si hiciera falta); con `.elf`/`.hex` la dirección ya viene incluida.
- Las acciones posteriores a la programación dependen de las opciones de sesión. Comprobá si tu
  comando deja el target detenido, lo resetea o lo reanuda.

> Si `pyocd list` no muestra la sonda, verificá que estás ejecutando el entorno virtual donde
> instalaste pyOCD e `hidapi`. Después revisá permisos udev y si otra aplicación ya tiene abierto
> el dispositivo.

### Opción 2: OpenOCD

OpenOCD separa la configuración de la sonda de la del target. En el caso mínimo se le pasan dos
archivos: el de la
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

> Los scripts suelen venir con OpenOCD, pero cambiar de sonda puede exigir además ajustar transporte,
> reset, velocidad y selección por número de serie. Conservá separado lo que pertenece a la interfaz
> de lo que pertenece al target.

### Opción 3: LinkServer (el que ya tenés, si instalaste MCUXpresso)

LinkServer puede instalarse con MCUXpresso o como paquete independiente y ofrece programación y
servidor GDB desde la terminal. Estos comandos corresponden a la versión probada; consultá
`LinkServer --help` si usás otra:

```bash
LS=/usr/local/LinkServer_1.6.133          # ajustá la version

$LS/LinkServer probes                     # ver los probes conectados
$LS/LinkServer flash LPC1769 load app.axf # grabar (.axf, .elf, .hex, .s19 o .bin con --addr)
$LS/LinkServer flash LPC1769 verify app.axf
$LS/LinkServer gdbserver LPC1769          # servidor gdb, igual que OpenOCD
```

Su documentación enumera familias de sondas NXP como LPC-Link2, MCU-Link y modelos LPC11U35/OpenSDA.
La compatibilidad concreta depende del dispositivo, el firmware y la versión. En la
[página 07](./07-mcuxpresso-por-dentro.md) se desarma el flujo probado.

> Con la OM13085 y la versión ensayada, LinkServer falló porque la sonda declara un `iSerial`
> vacío. Seleccionarla por índice produjo el mismo error. La ruta verificada para esa combinación es
> OpenOCD; el registro de la prueba está en la [página 06](./06-primer-grabado-verificado.md).

### Opción 4: J-Link

Si tenés una sonda SEGGER y sus herramientas instaladas:

```bash
JLinkExe -device LPC1769 -if SWD -speed 4000
J-Link> loadfile app.hex
J-Link> r        # reset
J-Link> g        # go
```

---

## Camino B: bootloader serial (ISP), sin probe

Si no tenés sonda, el LPC1769 puede programarse con un adaptador USB-serie de nivel lógico
compatible mediante el bootloader de fábrica. Pasos:

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

> El código ISP está en ROM y no se borra, pero su acceso puede quedar restringido por CRP o por el
> hardware de la placa. No lo tomes como ruta de recuperación hasta haber probado cómo entrar.

---

## ¿Cuál uso?

| | Probe SWD (OpenOCD/pyOCD) | ISP serial (lpc21isp/FlashMagic) |
|--|--------------------------|----------------------------------|
| Hardware extra | un probe (o el de a bordo en LPCXpresso) | un adaptador USB-serial barato |
| Permite **depurar** | **sí** (breakpoints, registros) | no, solo grabar |
| Rendimiento | depende de la sonda, el servidor y el algoritmo | depende de UART, autobaud y herramienta |
| Uso típico | desarrollo con inspección interactiva | programación sin sonda o flujo de producción controlado |

En la placa de la cátedra, la sonda integrada permite usar el camino A sin hardware adicional. El
camino B sigue siendo útil para entender y probar una vía independiente.

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

Éste es el esquema que sigue una sesión basada en OpenOCD. MCUXpresso suele usar LinkServer y otros
IDE pueden elegir servidores diferentes, pero todos separan una interfaz de usuario, un cliente GDB
y un servidor que controla la sonda. En la configuración de VSCode incluida, F5:

1. corre la tarea de build (compila),
2. lanza el servidor configurado,
3. conecta gdb, graba el firmware,
4. para en `main` y te deja depurar con clicks.

La secuencia deja claro en qué capa buscar cuando falla el build, la conexión o la ejecución.

## Resumen del módulo

- **Compilar** = el toolchain (`arm-none-eabi-gcc` + binutils), [parte 04](../04_toolchains/).
  Se hace en cualquier PC, sin hardware.
- **Editar cómodo** = VSCode o clangd, [04-05](../04_toolchains/05-el-editor-y-el-entorno.md).
- **Grabar** = sonda SWD (OpenOCD o pyOCD) o bootloader ISP serial (lpc21isp, FlashMagic), esta página.
- **Depurar** = OpenOCD + gdb (o F5 en VSCode), igual que MCUXpresso pero destapado.

El mismo modelo permite cambiar de editor o servidor sin confundirlos con el compilador y el
firmware.

> **Nota:** los comandos de grabado y debug requieren la **placa física** conectada. El compilar y
> linkear (lo que hicimos en todo el curso) no necesita hardware.

---

**LPC1769:** [índice](./README.md) ·
**Anterior:** [04 - Instalación en Windows](./04-instalacion-windows.md) ·
**Siguiente:** [06 - El primer grabado, verificado](./06-primer-grabado-verificado.md)
