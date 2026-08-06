# Anexo A: Build, linker y startup (sacar la caja negra)

> **Es un [anexo](../README.md): opcional.** No entra en los parciales. Si usás MCUXpresso, todo esto
> lo hace el IDE por vos y tu código anda igual. Leelo cuando quieras saber **qué** hace, o cuando
> algo se rompa de una forma que no se explica mirando solo tu `main.c`.

Cuando apretás "compilar" en MCUXpresso, pasan un montón de cosas que quedan ocultas: tu `main` no es
lo primero que corre, las variables globales aparecen ya inicializadas, y el programa "sabe" dónde
está la Flash y la RAM. Este módulo destapa esa caja negra. Es la **continuación natural del
[módulo 1](../../01_arquitectura_y_acceso_a_registros/)**: ahí vimos que el micro arranca leyendo la
Flash; acá vemos **cómo tu código termina ahí y cómo arranca de verdad**.

Es de los temas que más separan a quien "hace andar ejemplos" de quien **entiende** su sistema. Y
ahora tenemos una ventaja: en el [módulo 2](../../02_arma_tu_propia_libreria/src/build/) ya construimos
un firmware real (`mygpio`) con su propio `startup.c` y `linker script`, **compilado y linkeado de
verdad**. Este módulo lo explica.

## Recorrido

1. [01 - De código a binario: las secciones](./01-de-codigo-a-binario.md)
   Las etapas de build aplicadas al micro, el `.elf`/`.bin`, y las secciones `.text`/`.data`/`.bss`:
   qué va a Flash y qué a RAM, y por qué.
2. [02 - El linker script y el startup](./02-linker-y-startup.md)
   El `.ld` que ubica todo en memoria, y el código de arranque que copia `.data`, pone `.bss` en cero
   y salta a `main`, con el ejemplo real de `mygpio`.
3. [03 - El arranque paso a paso: de 0 V a `main()`](./03-el-arranque-paso-a-paso.md)
   La secuencia completa, sin saltear eslabones: el POR y el brown-out, el oscilador interno
   (y por qué el cristal no participa), los temporizadores de la Flash, qué decide la boot ROM
   y en qué orden, las dos lecturas que hace el Cortex-M3, y `RSID` para saber por qué se
   reseteó. Con los tiempos del manual y los registros medidos sobre la placa.

## Las versiones completas

Los archivos de este módulo son **mínimos a propósito**: tienen lo justo para que se
entienda la idea sin distracciones. Cuando quieras las versiones de producción, están en la
[plantilla](../../../plantilla/), comentadas línea por línea:

| Archivo | Qué agrega sobre la versión mínima |
|---------|-----------------------------------|
| [`linker/lpc1769.ld`](../../../plantilla/linker/lpc1769.ld) | las dos SRAM del bus AHB, heap y stack con verificación de que la RAM alcanza, `.init_array`, `.ARM.exidx` |
| [`startup/startup_lpc1769.c`](../../../plantilla/startup/startup_lpc1769.c) | la tabla de vectores **completa** (35 IRQ del LPC1769), `SystemInit()`, `__libc_init_array()` |
| [`Makefile`](../../../plantilla/Makefile) | el build entero: dependencias automáticas, `--gc-sections`, mapa de memoria, grabado y depuración |
| [`tools/lpc_checksum.py`](../../../plantilla/tools/lpc_checksum.py) | el checksum del vector 7 que exige la boot ROM |

Y para ver todo el recorrido de una sola vez, de `main.c` al LED encendido:
[el camino completo](../B_toolchain_y_entorno/00-el-camino-completo.md).

## Cuándo leerlo
Idealmente **justo después del [módulo 1](../../01_arquitectura_y_acceso_a_registros/)**, o cuando
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

**Anexos:** [índice](../README.md) · **El otro anexo:** [18 - Toolchain y entorno propio](../B_toolchain_y_entorno/) ·
**Volver al** [mapa del curso](../../README.md)
