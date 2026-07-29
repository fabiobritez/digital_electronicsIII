# Módulo 2: Armá tu propia librería (tu mini-CMSIS)

> **Por qué este módulo existe.** Es muy fácil caer en la trampa de pensar que CMSIS "viene dado",
> que es intocable, y que uno solo puede *usar* lo que NXP/ARM escribieron. **Falso.** CMSIS es código
> C común, escrito por personas, que vos podés leer, copiar, modificar y reemplazar. En este módulo
> lo vas a comprobar: **vas a construir tu propia librería de GPIO desde cero**, con la misma
> arquitectura que CMSIS, y después vas a ver cómo adaptarla a *otro* microcontrolador.

El objetivo no es que tires CMSIS a la basura (en la materia lo vas a seguir usando). El objetivo es
que entiendas que **el hardware es tuyo**: si mañana trabajás con un STM32, un RP2040 o un AVR, vas a
poder armar tu propia capa de acceso, o entender la de cualquier fabricante, porque ya sabés cómo se
construye.

## Lo que vas a lograr

Al terminar este módulo vas a tener una librería propia, `mygpio`, que se usa así:

```c
#include "mygpio.h"

int main(void) {
    mygpio_dir(0, 22, SALIDA);     // P0.22 como salida
    while (1) {
        mygpio_set(0, 22);         // prender
        for (volatile int i=0;i<1000000;i++);
        mygpio_clr(0, 22);         // apagar
        for (volatile int i=0;i<1000000;i++);
    }
}
```

…y vas a entender **cada línea de adentro**, porque la escribiste vos.

## Recorrido

1. [01 - Las tres capas de CMSIS](./01-las-tres-capas.md)
   Anatomía: capa del núcleo (Cortex-M3), capa del dispositivo (las structs del LPC) y capa de
   driver (las funciones). Qué hace cada una y por qué están separadas.
2. [02 - Construyendo mi propia librería de GPIO](./02-construyendo-mi-gpio.md)
   Paso a paso: el header con las structs, el `.c` con las funciones, y el `main` que la usa.
   Código completo y compilable.
3. [03 - Llevarla a otro hardware (abrir la mente)](./03-portabilidad-otro-hardware.md)
   Cómo la misma idea se adapta a otro chip, qué es una capa de abstracción de hardware (HAL), y
   cómo diseñar una API que no dependa de un micro puntual.

## Requisito
Tener fresco el [módulo 1](../01_arquitectura_y_acceso_a_registros/), sobre todo la
[página 03](../01_arquitectura_y_acceso_a_registros/03-de-direcciones-a-structs-cmsis.md): este
módulo es su continuación natural.
