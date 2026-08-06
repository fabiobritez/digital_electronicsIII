# Power: encender periféricos con PCONP

## El registro PCONP

`PCONP` (Power Control for Peripherals) está en la dirección **`0x400FC0C4`**, dentro del bloque de
System Control. Es un registro de 32 bits donde **cada bit enciende o apaga un periférico**:

- bit en **1** → periférico **alimentado** (encendido), responde a sus registros y recibe su clock.
- bit en **0** → periférico **apagado**: no consume, **no responde** y, además, **sus registros leen 0**
  o basura (no podés ni siquiera configurarlo).

Con CMSIS lo accedés como `LPC_SC->PCONP`.

> **Detalle fino que casi nadie menciona:** en el LPC176x, "power control" en realidad lo que hace es
> **darle clock** al periférico (gating de reloj). Apagar un bit no corta una línea de alimentación
> física: corta la señal de clock que llega a ese bloque. Por eso un periférico apagado igual "existe"
> en el mapa de memoria, pero como no tiene clock, su lógica está congelada y no procesa escrituras.
> El efecto práctico (no consume, no responde) es el mismo, pero entender que es *clock gating* explica
> por qué encender PCONP es instantáneo y no hay que esperar ningún tiempo de arranque.

### Mapa de bits de PCONP (LPC176x)

Estos son los nombres EXACTOS según el manual (cap. 4, Tabla 46):

| Bit | Símbolo | Periférico | Bit | Símbolo | Periférico |
|----:|---------|------------|----:|---------|------------|
| 1  | `PCTIM0`  | Timer/Counter 0 | 17 | `PCMCPWM`| Motor Control PWM |
| 2  | `PCTIM1`  | Timer/Counter 1 | 18 | `PCQEI`  | Quadrature Encoder |
| 3  | `PCUART0` | UART0           | 19 | `PCI2C1` | I2C1 |
| 4  | `PCUART1` | UART1           | 21 | `PCSSP0` | SSP0 |
| 6  | `PCPWM1`  | PWM1            | 22 | `PCTIM2` | Timer 2 |
| 7  | `PCI2C0`  | I2C0            | 23 | `PCTIM3` | Timer 3 |
| 8  | `PCSPI`   | SPI (legacy)    | 24 | `PCUART2`| UART2 |
| 9  | `PCRTC`   | RTC             | 25 | `PCUART3`| UART3 |
| 10 | `PCSSP1`  | SSP1            | 26 | `PCI2C2` | I2C2 |
| 12 | `PCADC`   | ADC             | 27 | `PCI2S`  | I2S |
| 13 | `PCCAN1`  | CAN1            | 29 | `PCGPDMA`| GPDMA (DMA) |
| 14 | `PCCAN2`  | CAN2            | 30 | `PCENET` | Ethernet |
| 15 | `PCGPIO`  | GPIO (incluye las interrupciones de GPIO) | 31 | `PCUSB` | USB |
| 16 | `PCRIT`   | Repetitive Int. Timer | | | |

> **Ojo con los `#define` de CMSIS:** en `lpc17xx_clkpwr.h` casi todos coinciden
> (`CLKPWR_PCONP_PCTIM0`, etc.), pero unos pocos usan un nombre distinto al del manual:
> el ADC es `CLKPWR_PCONP_PCAD` (no PCADC), los CAN son `CLKPWR_PCONP_PCAN1`/`PCAN2`
> (no PCCAN1/2) y el Motor PWM es `CLKPWR_PCONP_PCMC` (no PCMCPWM). El bit es el mismo;
> solo cambia el nombre.

Los bits **0, 5, 11, 20, 28** están reservados (no los toques). Notá que **no hay bit para SysTick,
WDT, EINT, ni el DAC**: SysTick es parte del núcleo Cortex-M3; el WDT, el Pin Connect y el System
Control **no se pueden apagar** (lo dice el manual, secc. 4.8.9); y el DAC se habilita por su pin
(P0.26 vía PINSEL1, lo vemos en su módulo).

> Tabla completa: Capítulo 4, sección PCONP, en
> [`manual/ch04...`](../../manual/ch04_clocking-and-power-control.pdf).

### El valor de reset NO es "todo apagado"

Acá hay una sutileza que confunde mucho, y el manual encima se contradice a sí mismo. La tabla
resumen del capítulo 4 (Tabla 14) dice que el reset value de PCONP es `0x03BE`; pero la tabla
detallada bit por bit (Tabla 46, la que manda) da reset value 1 en los bits
**1, 2, 3, 4, 6, 7, 8, 9, 10, 15, 19, 21, 26**, que suman **`0x042887DE`**. O sea que, a nivel de
silicio, varios periféricos arrancan **prendidos**: Timer0, Timer1, UART0, UART1, PWM1, I2C0, SPI,
RTC, SSP1, GPIO, I2C1, SSP0, I2C2. Y arrancan **apagados** el ADC (12), los CAN (13/14), los
Timers 2/3 (22/23), UART2/3 (24/25), DMA, Ethernet, USB, etc.

El código de arranque de CMSIS, además, escribe el registro completo. `SystemInit()` (la rutina que
corre antes de tu `main`) hace:

```c
LPC_SC->PCONP = 0x042887DE;   // PCONP_Val en system_LPC17xx.c
```

Fijate que es **exactamente el mismo valor** que el reset de la Tabla 46: NXP eligió `PCONP_Val`
para reproducir el estado por defecto del chip. Así que da igual a cuál de las dos tablas del manual
le creas: cuando llegás a `main()`, el estado es `0x042887DE`.

**¿Por qué te importa este detalle en la práctica?**

- Cuando llegás a tu `main()`, ya pasó `SystemInit()`. El estado "real" que ves es
  `0x042887DE`. Algunos periféricos que vas a usar mucho (Timer0, UART0, GPIO)
  ya vienen encendidos por defecto.
- Esto explica por qué a veces "te funciona sin tocar PCONP": ese bit ya estaba
  prendido por la config de CMSIS. Pero **no te confíes**: el ADC, los Timers 2 y 3, las UART2/3, el
  DMA y el USB **sí** arrancan apagados, y son justamente los que más te van a hacer renegar.
- Regla segura: **prendé siempre el PCONP del periférico que vas a usar**, aunque "ya estuviera". Un
  `|=` sobre un bit que ya está en 1 no hace daño, y te ahorra depender de un valor que podría cambiar
  si alguien edita `system_LPC17xx.c`.

## El patrón: "encendé el periférico primero"

Antes de configurar un periférico, prendé su bit en `PCONP` con un OR (para no apagar los demás):

```c
#include "LPC17xx.h"

// Encender el Timer 0 (bit 1)
LPC_SC->PCONP |= (1u << 1);

// Encender el ADC (bit 12)  <- este SÍ arranca apagado
LPC_SC->PCONP |= (1u << 12);

// Encender UART2 (bit 24)   <- este también arranca apagado
LPC_SC->PCONP |= (1u << 24);
```

A nivel registro pelado (sin CMSIS) sería:
```c
#define PCONP  (*(volatile uint32_t *) 0x400FC0C4)
PCONP |= (1u << 1);   // Timer 0
```

> **Error clásico:** olvidarse este paso. Configurás todo el Timer, lo arrancás… y el contador
> no avanza. El 90% de las veces es que **faltó `PCONP`**. Hacelo siempre lo primero en tu función
> de init del periférico.

> **Tip de depuración:** si un periférico "no hace nada" y revisaste la config, leé `LPC_SC->PCONP`
> en el debugger y fijate si su bit está en 1. Síntoma típico de PCONP apagado: **al escribir un
> registro de configuración del periférico y volver a leerlo, lee 0** (no "engancha" el valor). Eso
> es señal casi segura de clock gating: el bloque no tiene clock para latchear lo que escribís.

## El driver CMSIS hace lo mismo

CMSIS provee `CLKPWR_ConfigPPWR()` para esto, con nombres simbólicos:

```c
#include "lpc17xx_clkpwr.h"

CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM0, ENABLE);   // encender Timer 0
CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCAD,   ENABLE);   // encender ADC
CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCAD,   DISABLE);  // apagarlo cuando no se usa
```

Por dentro, `CLKPWR_ConfigPPWR` hace exactamente `LPC_SC->PCONP |= PPType;` (o `&= ~PPType` para
apagar), lo mismo que arriba, pero con el bit identificado por nombre. Notá que el argumento es la
**máscara ya corrida** (`1<<bit`), no el número de bit: por eso usás `CLKPWR_PCONP_PCAD` y no `12`.

> **¿De dónde salen `ENABLE` y `DISABLE`?** No son palabras de C: son constantes de un `enum`
> (`FunctionalState`) que define NXP en `lpc_types.h`, junto con `SUCCESS`/`ERROR` y `SET`/`RESET`
> que vas a ver más adelante. Están explicadas en
> [Módulo 2 - Las tres capas de CMSIS](../02_arma_tu_propia_libreria/01-las-tres-capas.md#el-vocabulario-propio-de-nxp-enable-success-set).

> Muchos drivers de init (por ej. `TIM_Init`, `UART_Init`) **ya encienden el PCONP por vos** adentro.
> Conviene saberlo: a veces el paso de PCONP "desaparece" porque el driver lo hace. Pero cuando
> trabajás a registro, sos vos quien tiene que acordarse.

## Apagar para ahorrar energía

Como `PCONP` controla el clock de cada bloque, apagar periféricos que no usás **baja el consumo**
(clave en dispositivos a batería). Para apagar, borrás el bit:

```c
LPC_SC->PCONP &= ~(1u << 12);   // apagar el ADC cuando no se usa
```

Cada bloque encendido consume corriente dinámica aunque no lo uses (su clock está conmutando). El
caso más caro es dejar prendidos bloques grandes que no usás: **Ethernet (30), USB (31) y el GPDMA
(29)** consumen bastante. Si tu proyecto no los toca, dejalos en 0.

> **Cuidado al apagar:** apagar un periférico le **borra su estado**. Si apagás la UART en medio de
> una transmisión, perdés lo que estaba en curso. Y si lo volvés a encender, arranca "limpio" y hay
> que reconfigurarlo. Apagá solo lo que de verdad no vas a usar por un rato largo.
>
> Caso particular del ADC (nota de la Tabla 46 del manual): antes de borrar `PCADC` hay que limpiar
> el bit `PDN` de `AD0CR`, y al revés al encender (primero `PCADC`, después `PDN`). Lo retomamos en
> el módulo del ADC.

Esto conecta con los modos de bajo consumo (Sleep / Power-down) que también están en el Capítulo 4 y
que vemos en la [última página del módulo](./06-modos-de-bajo-consumo.md).

---

**Módulo:** [Clock y Power](./README.md) ·
**Siguiente:** [02 - Clock de periféricos: PCLKSEL](./02-clock-pclksel.md)
