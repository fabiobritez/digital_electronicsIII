# Plantilla de proyecto para LPC1769

Un proyecto vacío listo para compilar, grabar y depurar el LPC1769 **sin MCUXpresso**,
con herramientas abiertas y un editor cualquiera. Copiá esta carpeta, escribí tu código
en `src/`, y andá.

No hay nada mágico adentro: son seis archivos de texto que podés leer enteros en media
hora. Ese es el punto.

```bash
make            # compila  -> build/firmware.elf, .bin, .hex
make flash      # graba la placa
make debug      # graba y abre gdb, parado en main
make help       # todos los comandos
```

## Arranque rápido

Si estás en Linux y ya tenés el toolchain del repo instalado (desde la raíz:
`bash tools/install_toolchain.sh`, o `bash tools/install_toolchain.sh --mcuxpresso` si
ya tenés MCUXpresso, que trae el mismo compilador adentro y no baja nada):

```bash
cd plantilla
make info       # ¿qué herramientas ve esta máquina?
make            # compilar
```

Deberías ver algo así:

```
  CC      src/main.c
  CC      src/syscalls.c
  CC      startup/startup_lpc1769.c
  LD      build/firmware.elf
Memory region         Used Size  Region Size  %age Used
           FLASH:         608 B       512 KB      0.12%
             RAM:        2080 B        32 KB      6.35%
```

Con eso ya compilaste un firmware real. Para grabarlo hace falta la placa y un grabador
instalado: la instalación completa está en
[el anexo B](../curso/anexos/B_toolchain_y_entorno/), y qué hacer según el debug probe que tengas,
en [la guía de probes](../curso/anexos/B_toolchain_y_entorno/probes/).

## Qué hay adentro

```
plantilla/
├── Makefile                     el build entero, comentado línea por línea
├── src/
│   ├── main.c                   blink del LED P0.22, escribiendo registros a mano
│   └── syscalls.c               el piso que printf() y malloc() esperan encontrar
├── startup/
│   └── startup_lpc1769.c        tabla de vectores + lo que corre antes de main()
├── linker/
│   └── lpc1769.ld               el mapa de memoria del chip
├── openocd/
│   └── lpc1769.cfg              config del grabador/depurador
├── tools/
│   ├── lpc_checksum.py          inyecta el checksum que exige la boot ROM
│   ├── preflight.py             chequea que el firmware vaya a arrancar, sin la placa
│   ├── gen_compile_commands.py  soporte para clangd (vim, neovim, helix...)
│   └── 99-lpc-probes.rules      permisos de USB en Linux
├── debug.gdb                    guion de arranque de gdb
├── .vscode/                     tareas, depuración e IntelliSense
└── .clangd                      lo mismo, para editores que no son VSCode
```

Las cuatro piezas que de verdad importan, y que son las mismas en **cualquier**
microcontrolador:

| Pieza | Qué contesta |
|-------|--------------|
| **Makefile** | qué se compila, con qué flags, y en qué orden |
| **linker script** | qué memorias tiene el chip y dónde va cada parte del programa |
| **startup** | qué corre antes de `main()` y cómo se conectan las interrupciones |
| **grabador** | cómo llegan los bytes del `.bin` a la FLASH del chip |

Cambiar de microcontrolador es cambiar el contenido de esas cuatro, no la estructura.
Por eso vale la pena entenderlas una vez.

## Los comandos

| Comando | Qué hace |
|---------|----------|
| `make` | compila y muestra cuánta FLASH y RAM usás |
| `make V=1` | igual, pero mostrando los comandos completos. **Corré esto una vez.** |
| `make flash` | graba la placa (detecta solo el grabador que tengas) |
| `make debug` | levanta el servidor gdb, graba y frena en `main` |
| `make gdbserver` | solo el servidor, para conectarle VSCode o un gdb aparte |
| `make erase` | borra la FLASH entera |
| `make reset` | resetea la placa sin regrabar |
| `make size` | FLASH y RAM que ocupa el firmware |
| `make preflight` | chequea que el firmware vaya a arrancar. Lo corre `make flash` solo |
| `make lst` | desensamblado con el C intercalado: qué hizo el compilador de verdad |
| `make vectores` | muestra la tabla de vectores y verifica su checksum |
| `make info` | qué compilador, gdb y grabadores encontró en esta máquina |
| `make compile_commands.json` | autocompletado para vim/neovim/helix/emacs |
| `make clean` | borra `build/` |

Opciones, que van después del comando:

| Opción | Para qué |
|--------|----------|
| `USE_CMSIS=1` | compila y linkea los drivers CMSIS de NXP |
| `OPT=-O2` | cambia la optimización (por defecto `-Og`) |
| `FLASHER=pyocd` | fuerza un grabador (`openocd`, `pyocd`, `linkserver`, `lpc21isp`) |
| `CROSS=...` | usa otro toolchain |
| `V=1` | muestra los comandos completos |

Por ejemplo: `make USE_CMSIS=1 flash`

## Dos modos: con CMSIS y sin CMSIS

Por defecto la plantilla compila **bare metal**: solo tu código, ni una línea de
biblioteca. Es el modo del parcial 1, donde se programa a registro.

Con `make USE_CMSIS=1` se suman los drivers de NXP que están en
[`../library/`](../library/). Se agrega una diferencia importante y poco obvia: CMSIS
trae `SystemInit()`, que el startup llama antes de `main()` y que **configura la PLL y
deja el core a 100 MHz**. Sin CMSIS, el micro se queda con el oscilador RC interno de
4 MHz.

Es decir que el mismo `main.c`, compilado de las dos formas, parpadea a velocidades
distintas. No es un error: es la demostración más barata de por qué los delays por
conteo de lazos no sirven y de que el clock es algo que se configura (módulos 3 y 6).

Linkear CMSIS **no** engorda el binario con drivers que no usás: `-ffunction-sections`
más `--gc-sections` descartan todo lo que no se llama. Con los 25 drivers compilados, el
blink pasa de 608 a 844 bytes, y esos 236 bytes son `SystemInit()`.

## El detalle que hace perder una tarde: el checksum

El LPC1769 no arranca cualquier cosa que encuentre en la FLASH. Antes de darle el
control a tu programa, la boot ROM suma las primeras 8 palabras de la tabla de vectores
y exige que el resultado sea **cero**. Si no da cero, asume que la FLASH está vacía y se
queda esperando en modo ISP.

Como las primeras 7 palabras son el stack pointer y los handlers, la única forma de que
la suma dé cero es poner en la octava (el vector 7, offset `0x1C`, que ARM declara
"reservado") el complemento a dos de las otras siete.

El problema es que cada herramienta lo maneja distinto: `openocd` y `lpc21isp` lo
parchean solos, **pyocd no**. Y cuando openocd lo parchea, el archivo en disco queda
distinto de lo grabado y el `verify` falla con un warning.

Por eso esta plantilla lo inyecta en tiempo de compilación, con
[`tools/lpc_checksum.py`](tools/lpc_checksum.py). Así los cuatro grabadores funcionan
igual y el `verify` pasa limpio. Para verlo:

```bash
make vectores
```

## Los chequeos antes de grabar

El checksum no es lo único que puede impedir que la placa arranque. Casi todo lo que
falla se puede detectar **en el archivo, antes de tocar el hardware**:

```bash
make preflight
```

```
  [ OK ]   checksum de la boot ROM      la suma de las 8 palabras da 0 (vector 7 = 0xEFFF75EE)
  [ OK ]   stack pointer inicial        0x10008000 (tope de la RAM: 0x10008000)
  [ OK ]   Reset_Handler                0x000001B1 (bit Thumb en 1, dentro de la FLASH)
  [ OK ]   CRP en 0x2FC                 la imagen termina en 0x260, no llega a esa palabra
  [ OK ]   tamano                       608 bytes de 524288 (0.12% de la FLASH)
```

No hace falta acordarse: **`make flash` lo corre solo** y no graba si algo falla. Qué
mira cada uno está explicado en [`tools/preflight.py`](tools/preflight.py).

El cuarto merece atención especial, porque es el único error de la lista que **no tiene
vuelta atrás**. El LPC1769 lee la palabra en `0x000002FC` al arrancar, y si encuentra
`0x43218765` activa el CRP3: deshabilita SWD **e** ISP para siempre. Esa placa no se
puede volver a programar por ningún medio. El riesgo real es bajísimo (tiene que caer un
valor exacto de 32 bits en una dirección exacta, y los programas chicos ni siquiera
graban esa zona), pero chequearlo sale gratis y equivocarse sale una placa.

## Editores

**VSCode**: abrí la carpeta y aceptá las extensiones que ofrece. `Ctrl+Shift+B` compila,
`F5` graba y depura. Los tres archivos de `.vscode/` están comentados.

**vim, neovim, helix, emacs, Sublime, Zed**: corré `make compile_commands.json` una vez.
Cualquier editor con clangd va a tener el mismo autocompletado y la misma navegación que
VSCode. La configuración está en [`.clangd`](.clangd).

En los dos casos, el editor **no compila nada**: llama al Makefile. Podés cambiar de
editor cuando quieras sin tocar el proyecto.

### Ver los registros por nombre en el debugger (opcional)

Un archivo `.svd` describe todos los periféricos del chip, y hace que el debugger te
muestre `PINSEL1` o `T0MR0` por nombre y con los bits desglosados, en vez de direcciones
crudas. Para conseguirlo:

```bash
pyocd pack install LPC1769
find ~/.local/share/cmsis-pack-manager -name '*.svd' | grep -i lpc17
```

Copiá el `.svd` a `tools/` y descomentá la línea `svdFile` de
[`.vscode/launch.json`](.vscode/launch.json).

## Llevártelo a otro lado

La plantilla es autocontenida salvo por dos rutas relativas, que apuntan al repo del
curso:

- el toolchain, en `../tools/toolchain/`
- CMSIS, en `../library/CMSISv2p00_LPC17xx/`

Si copiás la carpeta a otro lugar, las dos se ajustan sin editar nada:

```bash
make CROSS=arm-none-eabi-                       # toolchain del sistema
make USE_CMSIS=1 CMSIS_DIR=/ruta/a/CMSIS        # CMSIS en otro lado
```

Y si no vas a usar CMSIS, no hace falta nada: bare metal no depende de ninguna ruta
externa.

## Si algo no anda

| Síntoma | Causa casi segura |
|---------|-------------------|
| `arm-none-eabi-gcc: command not found` | falta el toolchain. `bash ../tools/install_toolchain.sh` |
| `make: *** No rule to make target` | estás parado en otra carpeta. `cd plantilla` |
| `unable to find CMSIS-DAP device` / `LIBUSB_ERROR_ACCESS` | permisos de USB. Instalá [`tools/99-lpc-probes.rules`](tools/99-lpc-probes.rules) |
| `could not read product string ... timed out` | la sonda quedó trabada. Reenchufá el cable USB |
| `No available debug probes are connected` (pyocd) | falta `hidapi`, o faltan los permisos de USB |
| `Ee(E1). Probe serial number not found` | LinkServer no anda con la sonda de la OM13085. Usá `FLASHER=openocd` |
| Graba bien pero la placa no hace nada | el checksum del vector 7. Verificá con `make preflight` |
| El LED parpadea rapidísimo | compilaste con `USE_CMSIS=1`: el core está a 100 MHz |
| VSCode subraya en rojo pero compila | es IntelliSense, no tu código. Corré `make compile_commands.json` |
| `region RAM overflowed` | tus variables globales no entran en 32 KB |
| El programa se cuelga sin razón | poné un breakpoint en `Default_Handler`: es una interrupción sin handler |

## Para entender qué hace cada pieza

- [Anexo A: build, linker y startup](../curso/anexos/A_build_linker_startup/) — qué son las
  secciones, cómo funciona el linker script y qué hace el startup.
- [Anexo B: toolchain y entorno](../curso/anexos/B_toolchain_y_entorno/) — qué es
  `arm-none-eabi-gcc`, cómo instalar todo en Linux y Windows, y cómo grabar según la
  probe que tengas.
- [Módulo 12: debug](../curso/12_debug/) — cómo usar el debugger y qué hacer con un hard
  fault.
