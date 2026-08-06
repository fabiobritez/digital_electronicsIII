# Clock de periféricos: PCLKSEL

Encender el periférico (PCONP) es la mitad. La otra mitad es **el reloj**: un periférico digital
necesita una señal de clock para funcionar, y de su frecuencia dependen todos los tiempos que vas a
calcular después (cuánto tarda un timer, qué baudrate da una UART, etc.).

> En el LPC176x, PCONP ya le da clock al bloque (es clock gating). Lo que hace `PCLKSEL` es elegir
> **a qué frecuencia** llega ese clock: divide el reloj del CPU antes de entregárselo al periférico.

## De dónde sale el reloj de un periférico

En el LPC1769, el CPU corre a una frecuencia llamada **`CCLK`** (CPU clock). En la configuración por
defecto que arma CMSIS para esta placa, **`CCLK = 100 MHz`** (de dónde sale eso lo vemos en la
[página del árbol de clock](./03-arbol-de-clock-y-pll.md)).

Cada periférico recibe un reloj propio, **`PCLK`**, que se obtiene **dividiendo `CCLK`** por un
factor que vos elegís en los registros `PCLKSEL`:

```
CCLK (100 MHz) ──/divisor──▶ PCLK del periférico
```

Casi todos los periféricos de baja velocidad cuelgan del **bus APB** (Advanced Peripheral Bus), y
cada uno recibe su `PCLK` propio derivado de `CCLK`.

## Los registros PCLKSEL0 y PCLKSEL1

Hay dos registros, cada uno con **2 bits por periférico**:

- `PCLKSEL0` → `0x400FC1A8` (`LPC_SC->PCLKSEL0`)
- `PCLKSEL1` → `0x400FC1AC` (`LPC_SC->PCLKSEL1`)

Los 2 bits de cada periférico se interpretan así:

| Bits | Divisor | PCLK resultante (con CCLK=100 MHz) |
|:----:|---------|-------------------------------------|
| `00` | CCLK / 4 | 25 MHz  ← **valor por defecto tras reset** |
| `01` | CCLK / 1 | 100 MHz |
| `10` | CCLK / 2 | 50 MHz |
| `11` | CCLK / 8 | 12.5 MHz |

> **Muy importante:** por defecto **todos** los periféricos arrancan en `00` → **CCLK/4 = 25 MHz**,
> no a 100 MHz. (Y CMSIS deja `PCLKSEL0_Val = PCLKSEL1_Val = 0x00000000`, así que **no lo cambia**:
> al entrar a `main` siguen todos en /4.) Si calculás un período de timer suponiendo 100 MHz pero el
> timer corre a 25 MHz, te va a dar **4× más lento**. Este es un error clásico en los parciales.

### La rareza del `11`: para el CAN es /6, no /8

Hay una excepción que casi nadie recuerda y que arruina cálculos de baudrate del CAN: cuando ponés
`11` en los campos de **CAN1, CAN2 y el filtro de aceptación (ACF)**, el divisor no es /8 sino
**/6**. Para todos los demás periféricos `11` = /8. Lo dice el manual y el comentario del header
CMSIS (`Hclk / 6` en esos tres campos). Si vas a configurar CAN, tenelo presente.

## El mapa completo de bits

Esta es la tabla que el datasheet te obliga a buscar cada vez. Acá la tenés entera, con los nombres
de campo del manual y el offset que usa el driver CMSIS (`CLKPWR_PCLKSEL_xxx`, que es **el número de
bit**; los de PCLKSEL1 suman 32):

### PCLKSEL0 (`0x400FC1A8`)

| Bits | Campo | Periférico | Bits | Campo | Periférico |
|:----:|-------|------------|:----:|-------|------------|
| 1:0   | `PCLK_WDT`    | Watchdog | 17:16 | `PCLK_SPI`  | SPI |
| 3:2   | `PCLK_TIMER0` | Timer 0  | 21:20 | `PCLK_SSP1` | SSP1 |
| 5:4   | `PCLK_TIMER1` | Timer 1  | 23:22 | `PCLK_DAC`  | DAC |
| 7:6   | `PCLK_UART0`  | UART0    | 25:24 | `PCLK_ADC`  | ADC |
| 9:8   | `PCLK_UART1`  | UART1    | 27:26 | `PCLK_CAN1` | CAN1 *(11=/6)* |
| 13:12 | `PCLK_PWM1`   | PWM1     | 29:28 | `PCLK_CAN2` | CAN2 *(11=/6)* |
| 15:14 | `PCLK_I2C0`   | I2C0     | 31:30 | `PCLK_ACF`  | CAN Acc.Filter *(11=/6)* |

(Los pares 11:10 y 19:18 de PCLKSEL0 están reservados.)

### PCLKSEL1 (`0x400FC1AC`)

| Bits | Campo | Periférico | Bits | Campo | Periférico |
|:----:|-------|------------|:----:|-------|------------|
| 1:0   | `PCLK_QEI`     | Quad. Encoder | 17:16 | `PCLK_UART2`  | UART2 |
| 3:2   | `PCLK_GPIOINT` | Interrupciones de GPIO | 19:18 | `PCLK_UART3` | UART3 |
| 5:4   | `PCLK_PCB`     | Pin Connect   | 21:20 | `PCLK_I2C2`   | I2C2 |
| 7:6   | `PCLK_I2C1`    | I2C1          | 23:22 | `PCLK_I2S`    | I2S |
| 11:10 | `PCLK_SSP0`    | SSP0          | 27:26 | `PCLK_RIT`    | Rep. Int. Timer |
| 13:12 | `PCLK_TIMER2`  | Timer 2       | 29:28 | `PCLK_SYSCON` | System Control |
| 15:14 | `PCLK_TIMER3`  | Timer 3       | 31:30 | `PCLK_MC`     | Motor Control |

(Los pares 9:8 y 25:24 de PCLKSEL1 están reservados.)

> Dos detalles de la misma sección del manual: el **RTC no aparece acá** porque su clock está
> **fijo en CCLK/8** (no se elige por PCLKSEL); y si usás CAN, **`PCLK_CAN1` y `PCLK_CAN2` tienen
> que tener el mismo divisor**.

> Tabla del manual: Capítulo 4
> ([`manual/ch04...`](../../manual/ch04_clocking-and-power-control.pdf)).

## Cómo configurarlo (a registro)

Supongamos que querés que el **Timer 0** corra al máximo, a `CCLK/1 = 100 MHz`. Los bits de Timer 0
en `PCLKSEL0` son los `3:2`. Para poner `01` ahí sin pisar el resto:

```c
#include "LPC17xx.h"

LPC_SC->PCLKSEL0 &= ~(0x3u << 2);   // 1) limpiar los 2 bits (queda 00)
LPC_SC->PCLKSEL0 |=  (0x1u << 2);   // 2) poner 01 -> CCLK/1 = 100 MHz
```

El patrón **"limpiar y luego poner"** es general para campos de varios bits: primero ponés en 0 todo
el campo con `&= ~(máscara)`, después escribís el valor con `|=`. Si solo hicieras `|=`, no podrías
volver un bit a 0.

> Mismo patrón a registro pelado:
> ```c
> #define PCLKSEL0 (*(volatile uint32_t *)0x400FC1A8)
> PCLKSEL0 = (PCLKSEL0 & ~(0x3u << 2)) | (0x1u << 2);
> ```

## Por qué esto te va a perseguir todo el curso

Cada vez que calcules un tiempo, necesitás saber el `PCLK` de ese periférico:

- **Timer:** período = `(valor_de_match + 1) × (prescaler+1) / PCLK`. Si no sabés `PCLK`, no sabés
  cuánto dura.
- **UART:** el baudrate se calcula a partir de `PCLK` del UART y los divisores `DLL/DLM` (+ el
  `FDR` fraccional). Un `PCLK` distinto da un baudrate distinto.
- **ADC:** el clock de conversión sale de dividir `PCLK` del ADC con `CLKDIV`; hay que mantener el
  clock del ADC ≤ 13 MHz. Con `PCLK=25 MHz` necesitás dividir al menos por 2.

Por eso, en cada periférico que veamos, **el primer dato que vas a fijar es su `PCLK`**.

> **Error clásico de alto nivel (el que más se ve en el parcial):** tenés una UART andando a 9600
> baud con `PCLK = 25 MHz` y los `DLL/DLM` calculados para eso. Después, "para ir más rápido",
> cambiás el `PCLK` de la UART a `01` (CCLK/1 = 100 MHz). De golpe el baudrate real se vuelve **4×**
> (sale basura por el TX). El divisor de la UART estaba calculado para 25 MHz, no para 100. Regla:
> **si cambiás el PCLK de un periférico, recalculá TODO lo que dependía de él** (baudrates,
> prescalers, divisores de ADC). No es "subir la frecuencia y listo".

## El driver CMSIS

```c
#include "lpc17xx_clkpwr.h"

// Poner el clock del Timer 0 en CCLK/1
CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_TIMER0, CLKPWR_PCLKSEL_CCLK_DIV_1);

// Consultar a qué frecuencia (Hz) está corriendo un periférico:
uint32_t pclk_timer0 = CLKPWR_GetPCLK(CLKPWR_PCLKSEL_TIMER0);
```

`CLKPWR_GetPCLK()` es utilísima: te devuelve el `PCLK` real en Hz, así no tenés que asumirlo. Por
dentro lee `PCLKSEL`, lo traduce a divisor (4/1/2/8) y divide `SystemCoreClock`. Cuando uses drivers,
apoyate en ella para los cálculos en vez de hardcodear 25 MHz.

> **Ojo:** `CLKPWR_GetPCLK` mapea `11` siempre a /8. Para CAN (donde `11` es /6) ese valor saldría
> mal. En la práctica casi nadie pone CAN en `11`, pero es bueno saber que el driver tiene ese punto
> ciego.

---

**Anterior:** [01 - Power: PCONP](./01-power-pconp.md) ·
**Siguiente:** [03 - El árbol de clock y la PLL](./03-arbol-de-clock-y-pll.md)
