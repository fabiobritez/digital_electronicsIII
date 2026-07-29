# Electrónica Digital III: LPC1769

Material de estudio para la materia, centrado en el microcontrolador **LPC1769** de NXP
(ARM Cortex-M3). La idea que atraviesa todo el curso es simple: primero entendés cada
periférico escribiendo sus registros a mano, y recién después usás el driver de CMSIS,
sabiendo exactamente qué hace por dentro.

## Por dónde empezar

**El curso completo está en [`curso/`](./curso/README.md).** Ahí está el índice con los 22
módulos en orden pedagógico: C para embebidos, acceso a registros, clock y power, y después
cada periférico (GPIO, SysTick, interrupciones, timers, UART, ADC/DAC, DMA, I2C, SPI, USB,
PWM y más), junto con módulos transversales sobre el proceso de build, arquitectura de
firmware, toolchain y hardware.

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
| [`curso/`](./curso/) | El material de estudio: 22 módulos, ejemplos y ejercicios. **Empezá acá.** |
| [`manual/`](./manual/) | UM10360 (User Manual del LPC17xx) dividido por capítulo, con [índice](./manual/INDEX.md) que mapea cada periférico a su capítulo y registros clave |
| [`library/`](./library/) | CMSIS v2.00 para LPC17xx: drivers de periféricos y más de 100 ejemplos oficiales de NXP |
| `UM10360.pdf` | El manual completo, por si preferís tenerlo entero |
| `tools/` | Scripts de mantenimiento del repo (split del manual, toolchain local) |

## Hardware y software

- **Micro:** LPC1769, ARM Cortex-M3 hasta 120 MHz (la placa de la cátedra corre a 100 MHz),
  512 KB de flash, 64 KB de RAM.
- **IDE:** MCUXpresso (gratuito). También podés armarte un entorno propio con VSCode y
  gcc-arm: está explicado en el [módulo 18](./curso/18_toolchain_y_entorno/).
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
