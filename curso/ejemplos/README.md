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
| [timers/](./timers/) | Ejemplos a registro (`01_semaforo_registros`, `02_juego_reflejos_registros`) y proyectos con driver (`patterns`, `lineas`) | [08 - Timers](../08_timers/) |
| [uart/](./uart/) | Eco serial a registro y con driver; `printf` por UART, por DMA y por el debugger (RTT), con [las mediciones reproducibles](./uart/MEDICIONES.md) | [09 - UART](../09_uart/) · [Herramientas 06.05](../../herramientas/06_depurar_en_serio/05-redirigir-printf-a-uart.md) |
| [adc_dac/](./adc_dac/) | Dos prácticas de ADC y dos de generación por DAC sin DMA, más proyectos con UART y DMA | [10 - ADC/DAC](../10_adc_dac/) |
| [dma/](./dma/) | Cuatro aplicaciones completas y el catálogo corto [`configs/`](./dma/configs/) con M2M, M2P, P2M, P2P, LLI y periféricos | [11 - DMA](../11_dma/) |

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
