# Módulo 7: Interrupciones (NVIC, EINT y GPIO)

Hasta ahora, para reaccionar a algo (un botón, un timer) hicimos *polling*: preguntar en un bucle
"¿ya pasó? ¿ya pasó?". Las **interrupciones** invierten eso: el hardware **avisa** cuando algo pasa y
el CPU **deja lo que estaba haciendo**, atiende el evento (en una función llamada *handler* o *ISR*)
y vuelve. Es más eficiente y permite reaccionar rápido sin desperdiciar el CPU.

El bloque que gestiona todas las interrupciones del Cortex-M3 es el **NVIC** (Nested Vectored
Interrupt Controller). Ya lo tocamos sin nombrarlo: `SysTick_Handler` (módulo 6) es una interrupción.

## Recorrido

1. [01 - NVIC, modelo de excepciones y tabla de vectores](./01-nvic-y-vectores.md)
   El modelo de excepciones del Cortex-M3 (excepciones del sistema vs IRQs), la tabla de vectores y
   VTOR, el stacking automático (por qué un ISR es una función C normal), habilitar interrupciones,
   pending/active, y la prioridad a fondo: 32 niveles, group vs subpriority, PRIGROUP, preempción
   anidada e inversión de prioridad.
2. [02 - Interrupciones externas (EINT) y por GPIO](./02-eint-y-gpio.md)
   Reaccionar a un pin: los EINT0–3 dedicados (modo/polaridad, glitch filter, wake de Power-down) y
   las interrupciones por cambio en cualquier pin de los puertos 0 y 2 (bloque GPIOINT), incluido
   cómo demultiplexar el vector compartido `EINT3_IRQHandler`.
3. [03 - Secciones críticas y atomicidad](./03-secciones-criticas-y-atomicidad.md)
   El problema que `volatile` no resuelve: condiciones de carrera entre la ISR y el `main`, y cómo
   protegerse (`__disable_irq`, ring buffers, no compartir estado mutable).

> Material original en [`_origen/`](./_origen/) (NVIC y GPIO/EINT por separado).

## Antes de esto
Módulos 1 (registros), 5 (GPIO) y 6 (SysTick: ahí viste tu primer handler). El concepto de
`volatile` ([módulo 0, cap. 08](../00_lenguaje_c/08-tipos-de-ancho-fijo-y-volatile.md)) es central:
las variables compartidas entre la ISR y el `main` **deben** ser `volatile`.

## Manual
NVIC: Capítulo 6 ([`manual/ch06...`](../../manual/ch06_nested-vectored-interrupt-controller.pdf)) y
el detalle del núcleo en el Capítulo 34. Interrupciones externas: Capítulo 3 §3.6
([`manual/ch03...`](../../manual/ch03_system-control.pdf)). Interrupciones por GPIO: Capítulo 9 §9.5.6
([`manual/ch09...`](../../manual/ch09_general-purpose-input-output.pdf)).
