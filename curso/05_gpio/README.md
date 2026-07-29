# Módulo 5: GPIO, entradas y salidas digitales

Los **GPIO** (*General Purpose Input/Output*) son los pines digitales de propósito general: con ellos
prendés LEDs, leés botones, manejás relés, generás señales, leés sensores digitales. Es el periférico
más simple y el primero que vas a dominar de punta a punta.

Ya lo probamos en el [módulo 1](../01_arquitectura_y_acceso_a_registros/02-acceso-a-registros-desde-c.md)
para introducir el acceso a registros. Acá lo vemos completo: todos sus registros, la lectura de
entradas, los trucos de acceso, y después el driver.

## Recorrido

1. [01 - GPIO a nivel registro: FIODIR, FIOSET, FIOCLR, FIOPIN, FIOMASK](./01-gpio-registros.md)
   El set completo de registros, salidas y entradas, y un ejemplo LED + botón a registro.
2. [02 - GPIO con el driver CMSIS](./02-gpio-con-driver.md)
   `GPIO_SetDir`, `GPIO_SetValue`, `GPIO_ClearValue`, `GPIO_ReadValue`: lo mismo, empaquetado.
3. [03 - Debounce y filtrado de entradas](./03-debounce-y-filtrado-de-entradas.md)
   Por qué un botón "rebota" y cómo filtrarlo bien (por tiempo y por conteo, con SysTick).
4. [04 - FIOMASK y acceso por byte](./04-fiomask-y-acceso-por-byte.md)
   Trucos avanzados: tratar un grupo de pines como unidad (bus), de forma atómica, sin tocar vecinos.

> Material original en [`_origen/`](./_origen/) (incluye guía de instalación del IDE MCUXpresso en
> [`_origen/0-ide.md`](./_origen/0-ide.md) y un `gpioHandler` de ejemplo).

## Antes de esto
Módulos 1 (registros), 3 (clock/power, aunque GPIO ya viene encendido) y 4 (PINSEL).

## Manual
**Capítulo 9** (GPIO): [`manual/ch09...`](../../manual/ch09_general-purpose-input-output.pdf).
