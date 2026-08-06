# ADC y DAC con el driver CMSIS

Los drivers `lpc17xx_adc` y `lpc17xx_dac` esconden el cálculo del `CLKDIV` y el manejo de los bits de
`ADCR`/`DACR`. Vos pedís "muestreá a tal frecuencia, canal tal" y leés el resultado.

## ADC con driver

```c
#include "lpc17xx_adc.h"
#include "lpc17xx_pinsel.h"

void adc_init(void) {
    // Pin AD0.0 = P0.23, función 1, tri-state (el driver no toca PINSEL)
    PINSEL_CFG_Type pin;
    pin.Portnum = 0; pin.Pinnum = 23; pin.Funcnum = 1;
    pin.Pinmode = PINSEL_PINMODE_TRISTATE; pin.OpenDrain = 0;
    PINSEL_ConfigPin(&pin);

    ADC_Init(LPC_ADC, 200000);                 // enciende PCONP, fija CLKDIV para 200 kHz
    ADC_ChannelCmd(LPC_ADC, 0, ENABLE);        // habilitar canal 0
}

uint16_t adc_leer(void) {
    ADC_StartCmd(LPC_ADC, ADC_START_NOW);      // arrancar una conversión
    while (!(ADC_ChannelGetStatus(LPC_ADC, 0, ADC_DATA_DONE))) { }  // esperar DONE
    return ADC_ChannelGetData(LPC_ADC, 0);     // 0..4095
}
```

> **Ojo con el `rate`:** el segundo argumento de `ADC_Init` es la **tasa de muestreo** deseada (en
> muestras/s), no `f_ADC`. El driver calcula internamente `CLKDIV = PCLK_ADC / (rate × 65) − 1`
> (porque una conversión son 65 ciclos). Como es **división entera**, pedir la tasa máxima exacta puede
> redondear `CLKDIV` para abajo y dejar `f_ADC` por encima de 13 MHz: con `PCLK_ADC = 25 MHz`,
> `ADC_Init(LPC_ADC, 200000)` da `25M/(200000·65) − 1 = 1.92 − 1 → CLKDIV = 0` y `f_ADC = 25 MHz`
> (¡inválido!). Para quedarte tranquilo, pedí una tasa un poco menor (p. ej. `190000`) o bajá `PCLK_ADC`.

| Función | Equivale a |
|---------|-----------|
| `ADC_Init(LPC_ADC, rate)` | PCONP + `CLKDIV` (para esa tasa de muestreo) + `PDN=1` |
| `ADC_ChannelCmd(LPC_ADC, ch, ENABLE)` | bit del canal en `SEL` |
| `ADC_StartCmd(LPC_ADC, ADC_START_NOW)` | `START` en `ADCR` |
| `ADC_ChannelGetData(LPC_ADC, ch)` | leer los 12 bits de `ADDRch` |
| `ADC_GlobalGetData(LPC_ADC)` | leer `ADGDR` |

### Modo burst (muestreo automático y continuo)

En burst, el ADC recorre los canales habilitados **solo, sin que arranques cada conversión**. Útil
para leer varios canales todo el tiempo:

```c
ADC_Init(LPC_ADC, 200000);
ADC_ChannelCmd(LPC_ADC, 0, ENABLE);
ADC_ChannelCmd(LPC_ADC, 1, ENABLE);
ADC_BurstCmd(LPC_ADC, ENABLE);     // arranca a convertir 0 y 1 sin parar
// luego, en cualquier momento: ADC_ChannelGetData(LPC_ADC, 0/1)
```

### Conversión por interrupción
`ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE)` + `NVIC_EnableIRQ(ADC_IRQn)`: el handler `ADC_IRQHandler`
se dispara al terminar la conversión y leés el dato ahí, sin esperar en un `while`.

## DAC con driver

```c
#include "lpc17xx_dac.h"
#include "lpc17xx_pinsel.h"

void dac_init(void) {
    PINSEL_CFG_Type pin;
    pin.Portnum = 0; pin.Pinnum = 26; pin.Funcnum = 2;   // AOUT
    pin.Pinmode = PINSEL_PINMODE_TRISTATE; pin.OpenDrain = 0;
    PINSEL_ConfigPin(&pin);
    DAC_Init(LPC_DAC);
}

// sacar una tensión: valor 0..1023
DAC_UpdateValue(LPC_DAC, 512);   // ~1.65 V
```

> **Qué hace `DAC_Init` de verdad:** casi nada: solo pone `BIAS = 0` (700 µA, settling 1 µs). **No**
> configura el PINSEL (por eso el `PINSEL_ConfigPin` va antes) ni toca `PCONP` (el DAC no tiene bit ahí).
> `DAC_UpdateValue` escribe `VALUE` preservando el `BIAS` que hayas elegido. Para el modo DMA/timer
> están `DAC_SetDMATimeOut` (carga `DACCNTVAL`) y `DAC_ConfigDAConverterControl` (bits `DBLBUF_ENA`,
> `CNT_ENA` y `DMA_ENA` de `DACCTRL`).

## Combo potente: DAC + Timer + DMA para generar un seno

La forma profesional de generar una onda continua: una **tabla** de valores en memoria, un **timer**
que marca el ritmo de muestreo, y el **DMA** copiando cada valor de la tabla al `DACR`, todo sin CPU.

```c
const uint16_t tabla_seno[64] = { /* valores 0..1023 de un seno */ };
// DMA configurado memoria->DAC, disparado por el timer (DMA request del DAC)
// El CPU queda libre; la señal sale sola y no se corta.
```

El ejemplo completo está en [`../ejemplos/dma/dac_dma_sin.c`](../ejemplos/dma/). Lo entendés del todo
después del [módulo 11 (DMA)](../11_dma/).

De forma simétrica, **ADC + DMA** permite llenar un buffer de muestras a alta velocidad sin que el CPU
lea cada una: ver [`../ejemplos/dma/adc_dma_simple.c`](../ejemplos/dma/).

## Errores comunes

| Error | Corrección |
|-------|-----------|
| Pull-up/down en el pin de ADC | tri-state siempre (altera la medición) |
| `clk_adc` > 13 MHz | elegir `CLKDIV` / `rate` para no pasarse |
| Olvidar `PDN=1` (a registro) | encender el ADC; `ADC_Init` lo hace |
| Esperar al ADC en burst con un `while DONE` mal | en burst leés el último dato, no arrancás |
| Suponer que el DAC usa PCONP | no lo usa: se habilita por PINSEL |
| Leer 16 bits crudos sin desplazar | el resultado está en bits 4–15 (driver ya lo entrega limpio) |

## Ejercicios
1. **Voltímetro:** leé un potenciómetro por ADC y mostrá la tensión por UART (combina módulos 9 y 10).
   Cuando lo tengas, comparalo con la versión resuelta:
   [`../ejemplos/adc_dac/voltimetro_adc_uart.c`](../ejemplos/adc_dac/voltimetro_adc_uart.c).
2. **Dimmer:** usá el ADC para controlar el brillo de un LED por PWM (módulo 8).
3. Generá una onda **triangular** por DAC con una tabla y un timer (sin DMA primero, después con DMA).
4. Reescribí la lectura de ADC **a registro** y verificá el `CLKDIV` para no pasar 13 MHz.

> Material original: [`_origen/07_ADC_DAC.md`](./_origen/07_ADC_DAC.md).

---

**Anterior:** [01 - ADC/DAC a nivel registro](./01-adc-dac-registros.md) ·
**Siguiente:** [03 - Muestreo, Nyquist y aliasing](./03-muestreo-nyquist-y-aliasing.md)
