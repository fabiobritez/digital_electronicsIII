# Módulo 6: SysTick, la base de tiempo

El **SysTick** es un temporizador de **24 bits** que **forma parte del núcleo Cortex-M3** (no es un
periférico de NXP). Por eso funciona igual en cualquier Cortex-M3/M4 y su documentación está en el
Capítulo 34 (Appendix Cortex-M3), no en los capítulos de NXP.

Es la forma más simple de tener una **base de tiempo precisa**: delays reales (no `for` vacíos),
"ticks" periódicos, timeouts, muestreo a intervalos fijos, o el latido de un sistema operativo de
tiempo real.

## Recorrido

1. [01 - SysTick a nivel registro: CTRL, LOAD, VAL, CALIB](./01-systick-registros.md)
   Por qué es del núcleo (no de NXP) y portable; el contador descendente de 24 bits; los cuatro
   registros campo por campo (incluido `CLKSOURCE`: CCLK vs el reloj externo STCLK); el cálculo
   exacto del reload y el límite de 24 bits; la prioridad de la excepción (`SCB->SHP`); `millis()`,
   `delay_ms` y los casos de borde.
2. [02 - SysTick con CMSIS: `SysTick_Config` y el driver](./02-systick-driver.md)
   La forma estándar de un renglón (qué hace por dentro), el driver de NXP, patrones reales (delay no
   bloqueante, timeouts, muestreo, el tick de un RTOS) y los errores comunes.

> Material original (muy completo del lado driver) en [`_origen/`](./_origen/).

## Antes de esto
Módulos 1 (registros) y 5 (GPIO). Conviene tener leído el concepto de **interrupción**: acá aparece
el primer *handler* (`SysTick_Handler`). Si querés el detalle del NVIC, está en el
[módulo 7](../07_interrupciones/).

## Manual
SysTick es del núcleo ARM: Capítulo 34
([`manual/ch34...`](../../manual/ch34_appendix-cortex-m3-user-guide.pdf)). También hay un capítulo
breve de NXP: [`manual/ch23...`](../../manual/ch23_system-tick-timer.pdf).
