# Módulo 16: Build, linker y startup (sacar la caja negra)

Cuando apretás "compilar" en MCUXpresso, pasan un montón de cosas que quedan ocultas: tu `main` no es
lo primero que corre, las variables globales aparecen ya inicializadas, y el programa "sabe" dónde
está la Flash y la RAM. Este módulo destapa esa caja negra. Es la **continuación natural del
[módulo 1](../01_arquitectura_y_acceso_a_registros/)**: ahí vimos que el micro arranca leyendo la
Flash; acá vemos **cómo tu código termina ahí y cómo arranca de verdad**.

Es de los temas que más separan a quien "hace andar ejemplos" de quien **entiende** su sistema. Y
ahora tenemos una ventaja: en el [módulo 2](../02_arma_tu_propia_libreria/src/build/) ya construimos
un firmware real (`mygpio`) con su propio `startup.c` y `linker script`, **compilado y linkeado de
verdad**. Este módulo lo explica.

## Recorrido

1. [01 - De código a binario: las secciones](./01-de-codigo-a-binario.md)
   Las etapas de build aplicadas al micro, el `.elf`/`.bin`, y las secciones `.text`/`.data`/`.bss`:
   qué va a Flash y qué a RAM, y por qué.
2. [02 - El linker script y el startup](./02-linker-y-startup.md)
   El `.ld` que ubica todo en memoria, y el código de arranque que copia `.data`, pone `.bss` en cero
   y salta a `main`, con el ejemplo real de `mygpio`.

## Cuándo leerlo
Idealmente **justo después del [módulo 1](../01_arquitectura_y_acceso_a_registros/)**, o cuando
quieras entender qué hace MCUXpresso por vos. También es la base para entender los **hard faults**
(módulo 12) por stack overflow.

## Por qué importa, en concreto
- Entender por qué una variable global mal usada se "pisa" (mapa de memoria).
- Saber qué es el stack, dónde está y qué pasa cuando se desborda.
- Poder portar el código a otro micro (cambia el linker script).
- Dejar de tenerle miedo al `startup_*.s` y al `.ld` que genera el IDE.

## Manual
Complementa con el Capítulo 34 (Cortex-M3, tabla de vectores y reset), el 2 (mapa de memoria y
boot ROM) y el 32 (el bootloader: cómo decide si tu código es válido).

---

**Anterior:** [15 - USB](../15_usb/) · **Siguiente:** [17 - Arquitectura de firmware](../17_arquitectura_de_firmware/)
