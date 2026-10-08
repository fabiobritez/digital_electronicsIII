# Ejemplos de ADC y DAC

## Para empezar: solo ADC, sin UART

Estos dos ejemplos forman una secuencia pensada para acompañar la unidad 10. Ambos muestran el
resultado con LEDs y variables del debugger, de modo que no requieren haber estudiado UART ni DAC.

| Carpeta | Nivel | Qué hace |
|---------|-------|----------|
| [01_voltimetro_leds/](./01_voltimetro_leds/) | Básico | Lee AD0.0 por polling, calcula milivoltios y muestra el nivel en cuatro LEDs |
| [02_muestreo_uniforme/](./02_muestreo_uniforme/) | Intermedio | Timer0 dispara el ADC a 100 muestras/s; una ISR aplica un promedio móvil y actualiza los LEDs |

El orden recomendado es hacer primero el voltímetro visual y luego comparar una muestra cruda con la
señal promediada en el debugger. Los README incluyen conexiones, cálculos, compilación y propuestas
para experimentar.

## Introducción al DAC: generar señales sin DMA

La segunda secuencia usa Timer0 para actualizar AOUT a intervalos regulares. El primer ejemplo deja
visible el mecanismo mediante *polling*; el segundo agrega una tabla y una interrupción.

| Carpeta | Nivel | Qué hace |
|---------|-------|----------|
| [03_dac_triangular_polling/](./03_dac_triangular_polling/) | Básico/intermedio | Genera una triangular de 100 Hz calculando cada muestra y consultando el flag de Timer0 |
| [04_dac_seno_interrupcion/](./04_dac_seno_interrupcion/) | Intermedio | Reproduce una tabla senoidal a 250 Hz desde `TIMER0_IRQHandler`, mientras `main` duerme |

Ambos acceden a los registros directamente, usan P0.26/AOUT y evitan deliberadamente el contador DMA
del DAC. El segundo deja clara la motivación para estudiar DMA después: a tasas altas, una interrupción
por muestra consume demasiado CPU.

## Para más adelante

Los siguientes ejemplos incorporan ADC + DAC, UART o DMA y quedan como continuación cuando se hayan
visto esas unidades:

| Archivo | Nivel | Qué hace |
|---------|-------|----------|
| [`adc_dac_registros.c`](./adc_dac_registros.c) | A registro | "Passthrough": lee un pote en AD0.0 (P0.23) y saca la misma tensión por AOUT (P0.26) |
| [`voltimetro_adc_uart.c`](./voltimetro_adc_uart.c) | Drivers CMSIS | Voltímetro serial: manda la tensión leída por UART0 cada 1 s, en mV sin `float` |
| [`osciloscopio_uart/`](./osciloscopio_uart/) | Proyecto completo | Genera 20 kHz con DAC, captura con Timer + ADC + DMA y grafica por UART con sample and hold |
| [`generador_funciones_uart/`](./generador_funciones_uart/) | Proyecto completo | Genera cinco formas con frecuencia y niveles variables, control UART y monitor ADC |

El voltímetro por UART integra tres módulos: ADC
([módulo 10](../../10_adc_dac/)), UART ([módulo 9](../../09_uart/)) y SysTick
([módulo 6](../../06_systick/)) con el patrón de tiempo no bloqueante del
[capítulos 17 a 19 del módulo 0](../../00_lenguaje_c/arquitectura/17-superloop-y-codigo-no-bloqueante.md).

Para el combo ADC/DAC con DMA (muestreo automático, generación de ondas), ver
[`../dma/`](../dma/): `adc_dma_simple.c` y `dac_dma_sin.c`.

Teoría: [módulo 10: ADC/DAC](../../10_adc_dac/).
