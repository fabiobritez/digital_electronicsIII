# Clock del USB (PLL1) y CLKOUT (depurar el reloj con un pin)

> Página avanzada y opcional. Dos temas que cierran el árbol de clock: cómo se generan los **48 MHz
> exactos** que el USB necesita, y cómo **sacar un reloj interno por un pin** para verificarlo con un
> osciloscopio. Si no vas a usar USB, leé al menos la parte de CLKOUT: es la mejor herramienta para
> depurar problemas de clock.

## Por qué el USB es especial

El USB full-speed exige un reloj de **exactamente 48 MHz**, con muy poca tolerancia. No podés sacarlo
de cualquier división de los 100 MHz del CPU (100/48 no es entero). El LPC1769 te da **dos caminos**
para conseguir esos 48 MHz:

1. **PLL1** (la "USB PLL"): una segunda PLL dedicada que multiplica directamente el cristal hasta
   48 MHz. Es lo que usa la config por defecto de CMSIS.
2. **Dividir la PLL0** con `USBCLKCFG`: si elegís M/N en PLL0 tal que `Fcco` sea múltiplo de 48 MHz,
   podés sacar el USB clock de ahí. Más enredado; rara vez se usa en la materia.

## Camino 1: la PLL1

La PLL1 toma el cristal (`Fosc = 12 MHz`) directamente (**no** pasa por `CLKSRCSEL`, siempre usa el
oscilador principal) y tiene su propia fórmula con un multiplicador **M** y un post-divisor **P**:

```
Fusb = M × Fosc                 (la salida que va al USB)
Fcco1 = Fusb × 2 × P            (la frecuencia interna del CCO de la PLL1)
```

con dos restricciones:
- **`Fusb = 48 MHz`** (lo que el USB necesita).
- **`Fcco1` entre 156 MHz y 320 MHz.**

Para 12 MHz de cristal: `M = 4` → `Fusb = 4 × 12 = 48 MHz`. Y con `P = 2`:
`Fcco1 = 48 × 2 × 2 = 192 MHz`, que cae en 156–320 MHz. Listo.

En el registro **`PLL1CFG`** (`0x400FC0A4`): M va en bits 4:0 **menos 1**, P va en bits 6:5 codificado
(`00`=1, `01`=2, `10`=4, `11`=8):

```
M = 4 → MSEL = 3 = 0b00011
P = 2 → PSEL = 1 → (1 << 5) = 0b0100000
PLL1CFG = 0x23   <- PLL1CFG_Val en system_LPC17xx.c
```

### La secuencia de la PLL1 es idéntica a la de la PLL0

Mismo patrón: configurar → feed → enable → feed → esperar lock → connect → feed. Solo cambian los
registros (`PLL1CON`, `PLL1CFG`, `PLL1FEED` en `0x400FC0AC`, `PLL1STAT`) y las posiciones de los bits
de estado (en `PLL1STAT`: `PLOCK1` es el bit **10**, `PLLC1_STAT` el **9**, `PLLE1_STAT` el **8**):

```c
LPC_SC->PLL1CFG  = 0x23;
LPC_SC->PLL1FEED = 0xAA;  LPC_SC->PLL1FEED = 0x55;

LPC_SC->PLL1CON  = 0x01;                    // enable
LPC_SC->PLL1FEED = 0xAA;  LPC_SC->PLL1FEED = 0x55;
while (!(LPC_SC->PLL1STAT & (1u << 10)));   // esperar PLOCK1

LPC_SC->PLL1CON  = 0x03;                    // enable + connect
LPC_SC->PLL1FEED = 0xAA;  LPC_SC->PLL1FEED = 0x55;
```

Cuando la PLL1 está conectada, su salida de 48 MHz va directo al bloque USB y **`USBCLKCFG` se ignora**.

## Camino 2: USBCLKCFG (PLL0 → USB)

Si **no** usás la PLL1, el clock del USB sale de dividir la salida de la PLL0 con **`USBCLKCFG`**
(`0x400FC108`, bits 3:0): el divisor es `USBSEL + 1`, y tenés que llegar a 48 MHz. Para que esto
funcione, `Fcco` de la PLL0 tiene que ser un **múltiplo par de 48 MHz** (o sea múltiplo de 96 MHz):
dividir por un número impar arruinaría el duty cycle del 50% que el USB exige. Por eso la Tabla 39
del manual solo admite tres valores de `USBSEL`: **5** (÷6, con Fcco = 288 MHz), **7** (÷8, 384 MHz)
y **9** (÷10, 480 MHz). Pero entonces la PLL0 ya no te da cómodo los 100 MHz de CPU. Por eso,
**si querés USB y 100 MHz de CPU al mismo tiempo, lo natural es usar la PLL1** y dejar la PLL0
dedicada al CPU. CMSIS hace exactamente eso (`PLL1_SETUP = 1`).

> En `system_LPC17xx.c`, `USBCLKCFG` solo se escribe en la rama `#else` (cuando `PLL1_SETUP == 0`).
> Con la PLL1 activa, ese registro ni se toca.

## CLKOUT: sacar un reloj interno por un pin

Esta es la herramienta de oro para depurar el clock. El LPC1769 puede **enrutar un reloj interno
hacia un pin físico** (P1.27, función CLKOUT), para que lo midas con un osciloscopio o un frecuencímetro
y confirmes a qué frecuencia está corriendo de verdad el micro.

Lo controla **`CLKOUTCFG`** (`0x400FC1C8`):

| Bits | Campo | Qué hace |
|:----:|-------|----------|
| 3:0  | `CLKOUTSEL`  | qué reloj sacar: `0`=CCLK (CPU), `1`=Main osc, `2`=IRC, `3`=USB clock, `4`=RTC |
| 7:4  | `CLKOUTDIV`  | divisor de salida: valor + 1 (1 a 16). Sirve para bajar la frecuencia a algo medible |
| 8    | `CLKOUT_EN`  | habilita la salida (switching sin glitches) |
| 9    | `CLKOUT_ACT` | (solo lectura) indica que CLKOUT está activo |

Ejemplo: sacar el **CCLK dividido por 8** (100 MHz → 12.5 MHz, más cómodo de medir) por el pin:

```c
#include "LPC17xx.h"

// 1) conectar el pin P1.27 a la función CLKOUT (PINSEL, lo vemos en el módulo 4)
LPC_PINCON->PINSEL3 |= (0x1u << 22);   // P1.27 -> CLKOUT (func 01)

// 2) CLKOUTSEL=0 (CCLK), CLKOUTDIV=8 -> campo 7, CLKOUT_EN=1
LPC_SC->CLKOUTCFG = (0u << 0) | ((8u - 1u) << 4) | (1u << 8);
```

Si medís ~12.5 MHz en P1.27, confirmás que el CPU está a 100 MHz. Si medís otra cosa, tu PLL no quedó
como creías. Es la forma más directa de **ver** el clock real en vez de deducirlo.

> El divisor acá no es solo comodidad: el manual avisa que si sacás el CCLK y este supera los
> ~50 MHz, **tenés que dividirlo**, porque el pin no llega a conmutar con niveles lógicos razonables
> a esa frecuencia. Sacar 100 MHz "crudos" por CLKOUT te daría una señal degradada o directamente
> nada medible.

> Con el driver: `CLKPWR_CLKOUTCFG_CLKOUTSEL_CPU`, `CLKPWR_CLKOUTCFG_CLKOUTDIV(n)` y
> `CLKPWR_CLKOUTCFG_CLKOUT_EN` arman el mismo valor con nombres simbólicos.

> **Para qué te sirve de verdad:** cuando algo de timing "no da" (un timer que mide mal, una UART con
> baudrate raro), antes de sospechar del periférico, sacá CCLK por CLKOUT y verificá la frecuencia
> base. Muchas veces el problema no es el periférico sino que el CPU no está a la frecuencia que
> asumías. También podés sacar el clock del USB para confirmar que son 48 MHz justos.

---

**Anterior:** [04 - El flash accelerator y los wait states](./04-flash-accelerator.md) ·
**Siguiente:** [06 - Modos de bajo consumo](./06-modos-de-bajo-consumo.md)
