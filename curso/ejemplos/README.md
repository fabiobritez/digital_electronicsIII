# Ejemplos prácticos: LPC1769

Código completo y funcional para varios periféricos. Cada ejemplo es autocontenido y pensado para
importar en MCUXpresso, compilar y cargar.

> **Cómo usarlos:** primero leé el módulo del periférico en el [curso](../README.md); después abrí el
> ejemplo, entendelo, cargalo y experimentá cambiando parámetros.

## Ejemplos incluidos en este repo

| Carpeta | Qué muestra | Módulo del curso |
|---------|-------------|------------------|
| [gpio/](./gpio/) | Handler de GPIO, control de LEDs, lectura de botones | [05 - GPIO](../05_gpio/) |
| [systick/](./systick/) | Interrupción periódica, base de tiempo | [06 - SysTick](../06_systick/) |
| [interrupciones/](./interrupciones/) | Interrupción por GPIO, NVIC, prioridades | [07 - Interrupciones](../07_interrupciones/) |
| [timers/](./timers/) | `patterns/` (patrones), `lineas/` (control de líneas), match/capture | [08 - Timers](../08_timers/) |
| [uart/](./uart/) | Eco serial: a registro (9600) y con driver (115200) | [09 - UART](../09_uart/) |
| [adc_dac/](./adc_dac/) | Passthrough analógico a registro; voltímetro serial con drivers | [10 - ADC/DAC](../10_adc_dac/) |
| [dma/](./dma/) | `m2m.c`, `adc_dma_simple.c`, `dac_dma_sin.c`, `lli_example.c` | [11 - DMA](../11_dma/) |

> **Nota:** para I2C, SPI, USB y el resto de los periféricos, ver los más de 100 ejemplos
> oficiales de NXP en [`../../library/examples/`](../../library/examples/) (organizados por
> periférico: I2C, SPI, SSP, UART, ADC, DAC, USB, CAN, I2S, RTC, WDT, etc.).

## Recomendaciones

- Cada ejemplo usa la **biblioteca CMSIS** del repo ([`../../library/`](../../library/)).
- Leé los **comentarios del código**: explican cada sección.
- Ante una duda de un registro, abrí el capítulo correspondiente del manual desde
  [`../../manual/INDEX.md`](../../manual/INDEX.md).

## Enlaces

- Curso completo: [../README.md](../README.md)
- Ejercicios de parcial: [../ejercicios/](../ejercicios/)
- Lenguaje C: [../00_lenguaje_c/](../00_lenguaje_c/)
