# Ejemplos de ADC y DAC

| Archivo | Nivel | Qué hace |
|---------|-------|----------|
| [`adc_dac_registros.c`](./adc_dac_registros.c) | A registro | "Passthrough": lee un pote en AD0.0 (P0.23) y saca la misma tensión por AOUT (P0.26) |
| [`voltimetro_adc_uart.c`](./voltimetro_adc_uart.c) | Drivers CMSIS | Voltímetro serial: manda la tensión leída por UART0 cada 1 s, en mV sin `float` |

El voltímetro es el ejercicio 1 del módulo 10 resuelto, e integra tres módulos: ADC
([módulo 10](../../10_adc_dac/)), UART ([módulo 9](../../09_uart/)) y SysTick
([módulo 6](../../06_systick/)) con el patrón de tiempo no bloqueante del
[capítulos 17 a 19 del módulo 0](../../00_lenguaje_c/17-superloop-y-codigo-no-bloqueante.md).

Para el combo ADC/DAC con DMA (muestreo automático, generación de ondas), ver
[`../dma/`](../dma/): `adc_dma_simple.c` y `dac_dma_sin.c`.

Teoría: [módulo 10: ADC/DAC](../../10_adc_dac/).
