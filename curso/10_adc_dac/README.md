# Módulo 10: ADC y DAC, el puente con el mundo analógico

El mundo real es analógico (temperatura, luz, sonido, posición de un potenciómetro). Para que el micro
lo entienda hay que **convertir**:

- **ADC** (Analog-to-Digital Converter): mide una tensión y la convierte en un número. El LPC1769
  tiene un ADC de **12 bits** (0–4095) y **8 canales**, hasta ~200 kHz de muestreo.
- **DAC** (Digital-to-Analog Converter): convierte un número en una tensión de salida. El LPC1769
  tiene un DAC de **10 bits** (0–1023) en el pin P0.26 (AOUT).

## Recorrido

1. [01 - ADC y DAC a nivel registro](./01-adc-dac-registros.md)
   `ADCR`/`ADGDR`/`ADDR`, conversión por software y lectura; el `DACR` y la generación de tensión.
2. [02 - ADC y DAC con el driver CMSIS](./02-adc-dac-con-driver.md)
   `ADC_Init`/`ADC_ChannelGetData`, modo burst, interrupción, y los combos con Timer + DMA.
3. [03 - Muestreo, Nyquist y aliasing](./03-muestreo-nyquist-y-aliasing.md)
   Cada cuánto leer el ADC: el teorema de Nyquist, el aliasing y el filtro antialiasing.

## El ritual de arranque
- **ADC:** encender `PCONP` bit 12 → `PCLKSEL` y `CLKDIV` (clock de conversión ≤ 13 MHz) → `PINSEL` en
  función ADC **tri-state** → configurar `ADCR` → arrancar y leer.
- **DAC:** rareza: **no usa PCONP**; se habilita configurando P0.26 en función DAC por `PINSEL`.
  Después se escribe el valor en `DACR`.

**Conversiones a tensión** (Vref ≈ 3.3 V):
`V_adc = (cuentas / 4095) × 3.3` · `V_dac = (valor / 1024) × 3.3` (el DAC divide por 1024, no 1023).

## Antes de esto
Módulos 3 (clock/power), 4 (PINSEL: tri-state para analógico). Para los combos, módulos 8 (timers) y
11 (DMA).

## Código listo para probar
En [`../ejemplos/adc_dac/`](../ejemplos/adc_dac/): el passthrough pote → AOUT a registro, y el
voltímetro serial (ADC + UART) con drivers.

## Manual
ADC Capítulo 29 ([`manual/ch29...`](../../manual/ch29_analog-to-digital-converter.pdf)),
DAC Capítulo 30 ([`manual/ch30...`](../../manual/ch30_digital-to-analog-converter.pdf)). Material
original en [`_origen/`](./_origen/).

---

**Anterior:** [09 - UART](../09_uart/) · **Siguiente:** [11 - DMA](../11_dma/)
