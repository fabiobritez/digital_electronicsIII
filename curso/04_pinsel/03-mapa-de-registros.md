# El mapa completo: PINSEL0-10, PINMODE0-9 y la fórmula del pin

En la página 01 viste la idea (2 bits por pin) con ejemplos sueltos. Acá tenés el **mapa completo**
del bloque PINCON y, sobre todo, la **fórmula** para contestar sin dudar la pregunta que aparece en
cada parcial: *"¿qué registro y qué bits le tocan al pin Px.y?"*.

## El modelo, otra vez, pero exacto

Cada pin `Px.y` tiene hasta **4 funciones** (00, 01, 10, 11), seleccionadas por **2 bits** en algún
`PINSELn`. Como un registro de 32 bits aloja 16 pines (16 × 2 = 32), cada puerto de 32 pines
necesita **dos** registros PINSEL: uno para los pines 0–15 y otro para los 16–31.

> La struct `LPC_PINCON` de CMSIS (en `LPC17xx.h`) los pone consecutivos en memoria: `PINSEL0` en
> `0x4002C000`, y cada uno 4 bytes más arriba. Eso es lo que hace que el driver pueda indexarlos como
> un arreglo (lo vemos abajo).

## PINSEL0-10: qué puerto controla cada uno

| Registro | Dirección | Controla | Estado |
|----------|-----------|----------|--------|
| `PINSEL0` | `0x4002C000` | P0[15:0] | en uso |
| `PINSEL1` | `0x4002C004` | P0[31:16] | en uso |
| `PINSEL2` | `0x4002C008` | P1[15:0] (Ethernet) | en uso |
| `PINSEL3` | `0x4002C00C` | P1[31:16] | en uso |
| `PINSEL4` | `0x4002C010` | P2[15:0] | en uso |
| `PINSEL5` | `0x4002C014` | P2[31:16] | reservado (no hay pines) |
| `PINSEL6` | `0x4002C018` | P3[15:0] | reservado (no hay pines) |
| `PINSEL7` | `0x4002C01C` | P3[31:16] | solo P3.25 y P3.26 |
| `PINSEL8` | `0x4002C020` | P4[15:0] | reservado (no hay pines) |
| `PINSEL9` | `0x4002C024` | P4[31:16] | solo P4.28 y P4.29 |
| `PINSEL10` | `0x4002C028` | **traza/debug** (solo bit 3) | caso especial |

> No te asustes con los "reservados": el LPC1769 no expone todos los 32 pines de cada puerto. P3 y P4
> casi no tienen pines. Por eso `PINSEL5/6/8` directamente no se usan, y `PINSEL7/9` controlan apenas
> dos pines cada uno. Escribir un 1 en un bit reservado no hace nada útil; no lo hagas.

`PINSEL10` es el bicho raro: no selecciona función de ningún pin GPIO. Solo su **bit 3** habilita el
puerto de traza del debugger (TPIU) sobre P2.2–P2.6, y cuando está activo, las señales de traza
salen por esos pines **sin importar lo que diga PINSEL4**. El driver lo maneja con
`PINSEL_ConfigTraceFunc()`. Para programación normal no lo tocás.

## La fórmula (la que tenés que saber de memoria)

Para un pin `Px.y` (puerto `x`, pin `y`):

```
registro PINSEL =  2*x + (y / 16)      // división entera
corrimiento     =  2 * (y % 16)        // bit menos significativo del par
máscara         =  0x3 << corrimiento
```

Lo mismo, en palabras: **el registro arranca en `2*x`** (P0 → 0, P1 → 2, P2 → 4, P3 → 6, P4 → 8); si
el pin es 16 o mayor, **sumás 1** al índice y le restás 16 al número de pin para calcular el
corrimiento. El corrimiento es **el doble del pin dentro de su mitad**.

### Ejemplos resueltos

| Pin | `2*x` | ¿y ≥ 16? | Registro | `y%16` | Corrimiento `2*(y%16)` | Bits |
|-----|-------|----------|----------|--------|------------------------|------|
| **P0.0** | 0 | no | `PINSEL0` | 0 | 0 | `[1:0]` |
| **P0.5** | 0 | no | `PINSEL0` | 5 | 10 | `[11:10]` |
| **P0.16** | 0 | sí (→ +1) | `PINSEL1` | 0 | 0 | `[1:0]` |
| **P0.23** | 0 | sí | `PINSEL1` | 7 | 14 | `[15:14]` |
| **P1.0** | 2 | no | `PINSEL2` | 0 | 0 | `[1:0]` |
| **P2.0** | 4 | no | `PINSEL4` | 0 | 0 | `[1:0]` |
| **P2.10** | 4 | no | `PINSEL4` | 10 | 20 | `[21:20]` |
| **P4.28** | 8 | sí | `PINSEL9` | 12 | 24 | `[25:24]` |

Fijate que **P0.0, P0.16, P1.0, P2.0 y P4.28 caen todos en los bits `[1:0]`** de su registro: el
corrimiento depende del pin *dentro de su mitad*, no de su número absoluto. Ese es el error de cálculo
clásico: usar `2*y` directo cuando `y ≥ 16` (te da un corrimiento de 32+, que no existe).

### A registro, paso a paso (P2.10 como función EINT0 = `01`)

P2.10 → `PINSEL4`, bits `[21:20]`:

```c
#include <LPC17xx.h>

LPC_PINCON->PINSEL4 &= ~(0x3u << 20);   // limpiar bits 21:20
LPC_PINCON->PINSEL4 |=  (0x1u << 20);   // 01 = EINT0
```

## PINMODE0-9: el mismo esquema, otra base

`PINMODE` usa **idéntica** estructura (2 bits por pin, 16 pines por registro), pero arranca en
`0x4002C040`:

| Registro | Dirección | Controla |
|----------|-----------|----------|
| `PINMODE0` | `0x4002C040` | P0[15:0] |
| `PINMODE1` | `0x4002C044` | P0[31:16] |
| `PINMODE2` | `0x4002C048` | P1[15:0] |
| `PINMODE3` | `0x4002C04C` | P1[31:16] |
| `PINMODE4` | `0x4002C050` | P2[15:0] |
| `PINMODE5` | `0x4002C054` | P2[31:16] (reservado) |
| `PINMODE6` | `0x4002C058` | P3[15:0] (reservado) |
| `PINMODE7` | `0x4002C05C` | P3[31:16] (solo P3.25/26) |
| `PINMODE9` | `0x4002C064` | P4[31:16] (solo P4.28/29) |

> El manual no documenta un `PINMODE8` (el puerto 4 bajo no tiene pines configurables), pero la
> struct de CMSIS **sí** define `PINMODE0`…`PINMODE9` de corrido en memoria (`PINMODE8` queda como
> relleno en `0x4002C060`, y no hay `PINMODE10`). Gracias a eso la **misma fórmula** sirve:
> `índice = 2*x + (y/16)`, corrimiento `2*(y%16)`. Solo cambia la base de la que partís.

## PINMODE_OD0-4: acá la fórmula cambia (1 bit por pin)

El open-drain es distinto: **1 bit por pin**, así que un solo registro cubre los 32 pines de un
puerto. Hay un PINMODE_OD por puerto, no por mitad:

| Registro | Dirección | Controla | Bit del pin `Px.y` |
|----------|-----------|----------|--------------------|
| `PINMODE_OD0` | `0x4002C068` | Puerto 0 | bit `y` |
| `PINMODE_OD1` | `0x4002C06C` | Puerto 1 | bit `y` |
| `PINMODE_OD2` | `0x4002C070` | Puerto 2 | bit `y` |
| `PINMODE_OD3` | `0x4002C074` | Puerto 3 | bit `y` |
| `PINMODE_OD4` | `0x4002C078` | Puerto 4 | bit `y` |

Acá la cuenta es directa: `PINMODE_OD<x>`, bit `y`. Para P0.27 en open-drain: `PINMODE_OD0 |= (1 << 27)`.

> Cuidado: P0.27 y P0.28 (los pines de I²C0) **ignoran** sus bits en `PINMODE_OD0`. Son open-drain de
> hardware y se configuran por `I2CPADCFG`. Ese caso lo cubre la
> [página 04](./04-pinmode-opendrain-tolerancia.md).

## Por qué el driver puede usar un solo `for` mental

Mirá el truco que usa el driver en
[`lpc17xx_pinsel.c`](../../library/CMSISv2p00_LPC17xx/Drivers/src/lpc17xx_pinsel.c): toma el puntero
a `PINSEL0` y lo trata como un arreglo de `uint32_t`. Después suma el índice `2*portnum (+1 si pin ≥ 16)`:

```c
uint32_t *pPinCon = (uint32_t *)&LPC_PINCON->PINSEL0;
uint32_t idx = 2 * portnum;
if (pinnum >= 16) { pinnum -= 16; idx++; }
pPinCon[idx] &= ~(0x3u << (pinnum * 2));
pPinCon[idx] |=  (funcnum << (pinnum * 2));
```

Es **exactamente la fórmula de arriba**, hecha código. El driver no hace nada raro: es esta cuenta que
acabás de aprender, escrita una sola vez para no equivocarte cada vez. Eso justifica por qué, una vez
que entendés la fórmula, usar el driver es lo razonable.

---

**Anterior:** [01 - La función de los pines](./01-funcion-de-los-pines.md) ·
**Módulo:** [PINSEL](./README.md) ·
**Siguiente:** [04 - PINMODE, open-drain, I²C y tolerancia 5 V](./04-pinmode-opendrain-tolerancia.md)
