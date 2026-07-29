# Módulo 1: Arquitectura y acceso a registros

> **El módulo más importante del curso.** Si entendés esto, todo lo demás (GPIO, timers, UART, ADC…)
> es la misma idea repetida. Si no lo entendés, vas a memorizar recetas sin saber por qué funcionan.

La idea central, en una frase:

> **Un "registro" del microcontrolador no es nada exótico: es una dirección de memoria. Configurar
> un periférico es escribir números en direcciones de memoria. Y eso, en C, se hace con un puntero.**

Todo lo que hace cualquier librería (CMSIS incluida) es ponerle nombres lindos a esas direcciones.
Nada más. En el [módulo 2](../02_arma_tu_propia_libreria/) vas a construir tu propia librería para
comprobarlo.

## Recorrido

1. [01 - El mapa de memoria del LPC1769](./01-mapa-de-memoria.md)
   Dónde vive la Flash, la RAM y los periféricos. Qué significa "memory-mapped I/O".
2. [02 - Cómo se accede a un registro desde C](./02-acceso-a-registros-desde-c.md)
   Punteros a `volatile`, máscaras de bits, y un **LED parpadeando escribiendo direcciones a mano**,
   sin ninguna librería.
3. [03 - De direcciones sueltas a structs estilo CMSIS](./03-de-direcciones-a-structs-cmsis.md)
   Cómo pasar de `*(uint32_t*)0x2009C000` a `LPC_GPIO0->FIODIR`. Acá nace la idea de "driver".
4. [04 - Bit-banding](./04-bit-banding.md) *(avanzado, opcional)*
   Acceso atómico a bits individuales: cada bit como una dirección propia. Cuándo sirve y cuándo no.

## Qué necesitás antes
- Módulo 0, sobre todo [05 - Punteros](../00_lenguaje_c/05-punteros.md),
  [07 - Structs/unions](../00_lenguaje_c/07-estructuras-uniones-enums.md) y
  [08 - Ancho fijo y `volatile`](../00_lenguaje_c/08-tipos-de-ancho-fijo-y-volatile.md).

## Manual de referencia
- Mapa de memoria: [`manual/ch02_memory-map.pdf`](../../manual/ch02_memory-map.pdf)
- GPIO (lo usamos de ejemplo): [`manual/ch09...`](../../manual/ch09_general-purpose-input-output.pdf)
