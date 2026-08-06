# Electrónica Digital III: LPC1769

Material de estudio para la materia, centrado en el microcontrolador **LPC1769** de NXP
(ARM Cortex-M3). La idea que atraviesa todo el curso es simple: primero entendés cada
periférico escribiendo sus registros a mano, y recién después usás el driver de CMSIS,
sabiendo exactamente qué hace por dentro.

## Por dónde empezar

**El curso completo está en [`curso/`](./curso/README.md).** Ahí está el índice con los 19
módulos en orden pedagógico: C para embebidos (que cierra con arquitectura de firmware),
acceso a registros, clock y power, y después cada periférico (GPIO, SysTick, interrupciones,
timers, UART, ADC/DAC, DMA, I2C, SPI, USB, PWM y más), más el hardware de la placa. Aparte
quedan los **anexos A y B**, opcionales, sobre el proceso de build y el toolchain.

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

## Qué hay en el repositorio

| Carpeta | Contenido |
|---------|-----------|
| [`curso/`](./curso/) | El material de estudio: 19 módulos, dos anexos opcionales, ejemplos y ejercicios. **Empezá acá.** |
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
- El [anexo B](./curso/anexos/B_toolchain_y_entorno/) lo explica pieza por pieza, arrancando
  por [el camino completo de `main.c` al LED](./curso/anexos/B_toolchain_y_entorno/00-el-camino-completo.md).
- La instalación paso a paso está para
  [Linux](./curso/anexos/B_toolchain_y_entorno/06-instalacion-linux.md) y
  [Windows](./curso/anexos/B_toolchain_y_entorno/07-instalacion-windows.md).
- Y como grabar depende del hardware que tengas, hay una
  [guía por cada sonda](./curso/anexos/B_toolchain_y_entorno/probes/).

## Hardware y software

- **Micro:** LPC1769, ARM Cortex-M3 hasta 120 MHz (la placa de la cátedra corre a 100 MHz),
  512 KB de flash, 64 KB de RAM.
- **Placa:** LPCXpresso LPC1769 rev D (OM13085), con sonda **CMSIS-DAP** a bordo. Al ser un
  estándar abierto de ARM, se graba y depura con herramientas libres, sin nada de NXP.
- **IDE:** ninguno obligatorio. Funciona con VSCode, con vim/neovim vía clangd, o con
  MCUXpresso si preferís. El [anexo B](./curso/anexos/B_toolchain_y_entorno/) detalla
  [qué es cada pieza del toolchain](./curso/anexos/B_toolchain_y_entorno/04-adentro-del-toolchain.md) y
  [cómo compila y graba MCUXpresso por dentro](./curso/anexos/B_toolchain_y_entorno/05-como-compila-y-graba-mcuxpresso.md).
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
