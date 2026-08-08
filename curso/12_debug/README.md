# Módulo 12: Debug, cómo encontrar por qué no anda

En embebidos no hay una consola por defecto ni se ve fácil qué pasa adentro del chip. Saber **depurar**
es la diferencia entre pelearse horas con un bug y resolverlo en minutos. Este módulo junta las
herramientas para ver qué está haciendo tu programa y un método para no perderte.

## Recorrido

1. [01 - Imprimir para depurar](./01-imprimir-para-depurar.md)
   El LED de diagnóstico, la UART como consola, `printf` por serial y el debug framework de NXP
   (`_DBG`, `_DBD`, `_DBH`).
2. [02 - El debugger y un método](./02-debugger-y-metodo.md)
   El debugger SWD/JTAG (breakpoints, paso a paso, ver registros en vivo), el checklist del "no anda"
   y los hard faults.
3. [03 - La consola por el cable del debugger (RTT)](./03-consola-por-el-debugger-rtt.md)
   `printf` y entrada de teclado **sin UART, sin pines y sin conversor USB-serie**, por el mismo
   cable con el que grabás. 17 µs de CPU por línea contra 4091 µs de la UART por polling. Es la
   forma más cómoda de depurar en el laboratorio, y la guía incluye la puesta a punto en Ubuntu 24 y
   qué pasa (y qué no) si además usás MCUXpresso.

> **Si venís a buscar una sola cosa de este módulo, que sea el capítulo 3.** Tener una consola que
> no gasta la UART ni cuesta tiempo de CPU cambia cómo se depura el resto del curso.

## La idea central
La mayoría de los "no funciona" de la materia son una de seis cosas: el periférico **sin encender**
(PCONP), **sin clock** o con el `PCLK` mal calculado, los **pines** mal configurados (PINSEL), una
**bandera de interrupción sin limpiar**, una variable de ISR **sin `volatile`**, o una **cuenta de
tiempo/baudrate** mal hecha. El debugger, que te
deja leer los registros en vivo, es la forma más rápida de descubrir cuál.

## Se apoya en todo lo anterior
Módulos 3 (PCONP/PCLKSEL), 4 (PINSEL), 7 (interrupciones), 9 (UART) y 0 cap. 08 (`volatile`).

## Manual
Depuración por hardware (núcleo Cortex-M3): Capítulos 33 y 34. Framework de debug del repo en
[`_origen/`](./_origen/).

---

**Anterior:** [11 - DMA](../11_dma/) · **Siguiente:** [13 - I2C](../13_i2c/)
