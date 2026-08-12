# Electrónica Digital III: LPC1769

Material de estudio para la materia, centrado en el microcontrolador **LPC1769** de NXP
(ARM Cortex-M3). La idea que atraviesa todo el curso es simple: primero entendés cada
periférico escribiendo sus registros a mano, y recién después usás el driver de CMSIS,
sabiendo exactamente qué hace por dentro.

## Por dónde empezar

**El curso completo está en [`curso/`](./curso/README.md).** Ahí está el índice con los
módulos en orden pedagógico: C para embebidos (que cierra con arquitectura de firmware),
acceso a registros, clock y power, y después cada periférico (GPIO, SysTick, interrupciones,
timers, UART, ADC/DAC, DMA, I2C, SPI, USB, PWM y más), más el hardware de la placa.

**Todo lo que es herramienta y no materia está aparte, en
[`herramientas/`](./herramientas/README.md):** cómo se compila, cómo se graba y cómo se
depura una placa de desarrollo. Los protocolos JTAG y SWD, el hardware de depuración que trae
el Cortex-M3 adentro del núcleo, qué es una sonda y qué es CMSIS-DAP, el toolchain y el
triplet, y al final todo eso aplicado al LPC1769. Es opcional para cursar, y es lo único del
repositorio que te va a servir igual con cualquier otro microcontrolador.

Si es tu primera vez acá:

1. Abrí [`curso/README.md`](./curso/README.md) y seguí el mapa en orden.
2. Cuando un tema te genere dudas de hardware, buscá el capítulo en
   [`manual/INDEX.md`](./manual/INDEX.md): es el User Manual oficial (UM10360) dividido en
   un PDF por capítulo, para no pelearse con un archivo de 840 páginas.
3. Para practicar, están [`curso/ejemplos/`](./curso/ejemplos/) (código funcional por
   periférico) y [`curso/ejercicios/`](./curso/ejercicios/) (parciales de 2022, 2023 y 2025
   resueltos, con análisis de errores comunes).
4. Antes del parcial, imprimite la
   [referencia rápida](./curso/REFERENCIA_RAPIDA.md): una página con el ritual de arranque,
   las fórmulas y los registros que más se usan.
5. Cuando algo no ande, o cuando te dé curiosidad qué pasa detrás del botón "Build", abrí
   [`herramientas/`](./herramientas/README.md).

## Qué hay en el repositorio

| Carpeta | Contenido |
|---------|-----------|
| [`curso/`](./curso/) | El material de estudio: los módulos del LPC1769 en orden, más ejemplos y ejercicios. **Empezá acá.** |
| [`herramientas/`](./herramientas/) | **Unidad aparte:** programar y depurar placas de desarrollo. Protocolos, sondas, CMSIS-DAP, toolchains, build y depuración |
| [`plantilla/`](./plantilla/) | Proyecto listo para compilar, grabar y depurar sin MCUXpresso. `make`, `make flash`, `make debug` |
| [`manual/`](./manual/) | UM10360 (User Manual del LPC17xx) dividido por capítulo, con [índice](./manual/INDEX.md) que mapea cada periférico a su capítulo y registros clave |
| [`library/`](./library/) | CMSIS v2.00 para LPC17xx: drivers de periféricos y más de 100 ejemplos oficiales de NXP |
| `UM10360.pdf` | El manual completo, por si preferís tenerlo entero |
| [`tools/`](./tools/) | Scripts del repo: instalación del toolchain ARM y split del manual. Ver [`tools/README.md`](./tools/README.md) |

## Compilar y grabar sin MCUXpresso

El repo trae un stack completo y portable, para entender qué pasa detrás del botón
"Build" de un IDE:

```bash
bash tools/install_toolchain.sh    # el compilador, sin sudo y sin internet
cd plantilla
make                               # compila  -> build/firmware.elf, .bin, .hex
make flash                         # graba la placa
make debug                         # graba y abre gdb, parado en main
```

- La [plantilla](./plantilla/) es un proyecto autocontenido: `Makefile`, linker script,
  startup y configuración de editor, todo comentado línea por línea.
- La unidad [herramientas](./herramientas/) lo explica pieza por pieza, arrancando por
  [el mapa completo de `main.c` al LED](./herramientas/01_panorama/01-el-mapa-completo.md).
- La instalación paso a paso está para
  [Linux](herramientas/07_lpc1769/03-instalacion-linux.md) y
  [Windows](herramientas/07_lpc1769/04-instalacion-windows.md).
- Y como grabar depende del hardware que tengas, hay una
  [guía por cada sonda](./herramientas/07_lpc1769/probes/), más una página sobre
  [qué es exactamente una sonda](./herramientas/03_debug_probes/01-un-probe-es-otro-micro.md).

## Hardware y software

- **Micro:** LPC1769, ARM Cortex-M3 hasta 120 MHz (la placa de la cátedra corre a 100 MHz),
  512 KB de flash, 64 KB de RAM.
- **Placa:** LPCXpresso LPC1769 rev D (OM13085), con sonda **CMSIS-DAP** a bordo. Al ser un
  estándar abierto de ARM, se graba y depura con herramientas libres, sin nada de NXP.
- **IDE:** ninguno obligatorio. Funciona con VSCode, con vim/neovim vía clangd, o con
  MCUXpresso si preferís. La unidad [herramientas](./herramientas/) detalla
  [qué es cada pieza del toolchain](herramientas/04_toolchains/03-anatomia-de-la-carpeta.md) y
  [cómo compila y graba MCUXpresso por dentro](herramientas/07_lpc1769/07-mcuxpresso-por-dentro.md).
- **Toolchain:** `bash tools/install_toolchain.sh` deja `arm-none-eabi-gcc` en `tools/toolchain/`
  sin tocar el sistema ni pedir `sudo`.
- **Librería:** CMSIS v2.00 para LPC17xx, incluida en [`library/`](./library/).

## Documentación de referencia

- [Datasheet del LPC1769](https://www.nxp.com/docs/en/data-sheet/LPC1769_68_67_66_65_64_63.pdf)
- [User Manual UM10360](https://www.nxp.com/docs/en/user-guide/UM10360.pdf) (el mismo que está dividido en `manual/`)
- [Cortex-M3 Technical Reference Manual](https://developer.arm.com/documentation/ddi0337/latest/)
- *The Definitive Guide to ARM Cortex-M3/M4*, Joseph Yiu, si querés profundizar en el core

## Contribuciones

Si encontrás un error o algo que se pueda explicar mejor, abrí un issue o mandá un pull
request. El material es de uso académico; la librería CMSIS mantiene su licencia original
de ARM.
