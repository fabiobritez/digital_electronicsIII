# El flash accelerator y los wait states (FLASHCFG)

> Página avanzada y muy ligada a la anterior. Lo hace `SystemInit()` por vos, pero entender el flash
> accelerator explica un montón de cosas: por qué el LPC1769 puede correr a 100 MHz "sin perder
> tiempo", y por qué configurarlo mal hace que el micro **se cuelgue justo cuando subís la frecuencia**.

## El problema: la flash es lenta, el CPU es rápido

El programa vive en la **memoria flash** del LPC1769. El Cortex-M3 ejecuta una instrucción por ciclo
en el mejor caso, así que a 100 MHz pide una palabra de la flash cada 10 ns. Pero la memoria flash
**no puede entregar datos tan rápido**: su tiempo de acceso físico es del orden de 50 ns. Si el CPU
tuviera que esperar a la flash en cada instrucción, correr a 100 MHz no serviría de nada: estaría
parado la mayor parte del tiempo.

La solución de NXP es el **flash accelerator**: un bloque entre el CPU y la flash con un arreglo de
**ocho buffers de 128 bits** que hace **prefetch** (lee por adelantado) líneas completas de la flash.
Cada línea de 128 bits trae **4 instrucciones de 32 bits** (u 8 de 16 bits Thumb) de una sola lectura.
Mientras el CPU ejecuta esas 4, el accelerator ya está trayendo las 4 siguientes. En código secuencial,
el CPU casi nunca espera.

> Por eso el LPC176x ejecuta a "casi 1 instrucción por ciclo" a 100 MHz a pesar de tener una flash
> lenta. El precio se paga en los **saltos**: cuando el programa salta a una dirección que no está en
> ningún buffer, el CPU sí se frena (stall) mientras se trae esa línea. Bucles ajustados y código
> lineal vuelan; código lleno de saltos a direcciones lejanas pierde más.

## FLASHCFG: cuántos ciclos tarda un acceso a flash

El registro que controla esto es **`FLASHCFG`**, en la dirección **`0x400FC000`** (`LPC_SC->FLASHCFG`).
Lo único que vos tocás de él es el campo **`FLASHTIM`** (bits **15:12**): le decís **cuántos ciclos de
CPU** dura un acceso a la flash. El número de ciclos es **`FLASHTIM + 1`**.

| `FLASHTIM` | Ciclos de CPU por acceso | Válido hasta |
|:----------:|:------------------------:|--------------|
| `0000` | 1 | CCLK ≤ 20 MHz |
| `0001` | 2 | CCLK ≤ 40 MHz |
| `0010` | 3 | CCLK ≤ 60 MHz |
| `0011` | 4 | CCLK ≤ 80 MHz |
| `0100` | **5** | **CCLK ≤ 100 MHz** (y hasta 120 MHz solo en LPC1759/**LPC1769**) |
| `0101` | 6 | "safe": funciona en cualquier condición |

> **Los bits 11:0 NO se tocan.** El manual lo dice explícito: controlan funciones internas del
> accelerator y su valor de reset es `0x03A`. Por eso `SystemInit` hace una lectura-modificación-
> escritura que **preserva** la parte baja:
> ```c
> LPC_SC->FLASHCFG = (LPC_SC->FLASHCFG & ~0x0000F000) | FLASHCFG_Val;
> ```
> El valor de reset completo de `FLASHCFG` es **`0x0000303A`** (FLASHTIM = `0011` = 4 ciclos, parte
> baja `0x03A`).

## Por qué a 100 MHz necesitás 5 ciclos

La cuenta es directa: la flash física tarda un tiempo fijo en responder (≈ 50 ns en el peor caso). A
distintas frecuencias de CPU, ese tiempo fijo equivale a distinta cantidad de **ciclos de CPU**:

```
ciclos necesarios = tiempo_acceso_flash × CCLK
```

- A 20 MHz, un ciclo dura 50 ns → con **1 ciclo** alcanza.
- A 100 MHz, un ciclo dura 10 ns → para cubrir ~50 ns necesitás **5 ciclos**.

Por eso la tabla sube de a un ciclo cada ~20 MHz. Es el mismo concepto de "wait states" que en
cualquier memoria lenta colgada de un bus rápido: le das al hardware permiso para esperar la cantidad
justa de ciclos antes de dar el dato por válido.

CMSIS usa `FLASHCFG_Val = 0x00004000`, o sea **FLASHTIM = `0100` = 5 ciclos**, exacto para los
100 MHz de la placa.

## Qué pasa si lo configurás mal (los dos errores opuestos)

Este registro tiene una trampa pedagógica linda: **se puede romper en las dos direcciones**.

- **Muy pocos wait states para la frecuencia (FLASHTIM bajo a frecuencia alta):** la flash todavía no
  terminó de entregar el dato y el CPU lo lee igual → **lee instrucciones corruptas**. El resultado
  es un micro que **se cuelga, hace HardFault o se comporta de forma errática justo después de subir
  el clock a 100 MHz**. Síntoma clásico: "andaba a baja frecuencia y al activar la PLL se muere". Casi
  siempre es el flash con wait states de menos. **El orden correcto es: configurar el flash para la
  frecuencia ALTA antes (o junto con) subir el clock.**

- **Demasiados wait states (FLASHTIM alto):** no rompe nada, pero **desperdiciás performance**: cada
  acceso que falla en los buffers tarda más ciclos de los necesarios. El `0101` (6 ciclos, "safe")
  anda siempre pero es el más lento. Está pensado como red de seguridad / para silicio futuro, no para
  uso normal.

> **Regla de oro del orden:** si vas a cambiar la frecuencia del CPU,
> - **al SUBIR** la frecuencia: primero subí los wait states del flash, después subí el clock.
> - **al BAJAR** la frecuencia: primero bajá el clock, después (si querés) bajá los wait states.
>
> Así nunca tenés un instante con clock alto y wait states de menos. `SystemInit` lo resuelve fácil:
> setea el flash a 5 ciclos *después* de enganchar la PLL, pero como el código de `SystemInit` corre
> desde flash, NXP eligió el valor que ya sirve para la frecuencia final. En reconfiguraciones en
> caliente, vos sos responsable del orden.

## A registro

```c
#include "LPC17xx.h"

// Poner 5 wait states (FLASHTIM=4) para 100 MHz, preservando los bits bajos:
LPC_SC->FLASHCFG = (LPC_SC->FLASHCFG & ~(0xFu << 12)) | (0x4u << 12);
```

No hay driver dedicado en `lpc17xx_clkpwr` para esto: es algo que normalmente solo toca `SystemInit`.
Si lo tocás vos, acordate del read-modify-write para no pisar la parte baja.

> **Dato útil para depurar:** cambiar `FLASHCFG` **invalida automáticamente los ocho buffers** del
> accelerator (lo dice el manual). O sea: tras escribirlo, la próxima ejecución vuelve a leer de flash
> "fresca". Es seguro cambiarlo en caliente desde ese punto de vista; el peligro es solo el orden
> respecto de la frecuencia.

## Relación con el resto del módulo

- La frecuencia que determina cuántos wait states necesitás es **`CCLK`**, que sale de la PLL0 (página
  [03](./03-arbol-de-clock-y-pll.md)).
- Si bajás la frecuencia para **ahorrar energía** (página [06](./06-modos-de-bajo-consumo.md)), podés
  bajar también los wait states, pero el ahorro real está en bajar el clock, no el flash.

---

**Anterior:** [03 - El árbol de clock y la PLL0](./03-arbol-de-clock-y-pll.md) ·
**Siguiente:** [05 - Clock del USB (PLL1) y CLKOUT](./05-usb-clock-y-clkout.md)
