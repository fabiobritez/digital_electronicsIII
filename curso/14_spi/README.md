# Módulo 14: SPI / SSP (plus)

El **SPI** (Serial Peripheral Interface) es otro bus serial síncrono, pero más rápido y simple que el
I2C. Se usa para tarjetas SD, memorias flash, displays TFT, conversores ADC/DAC externos, módulos de
radio (nRF24, LoRa). Donde el I2C prioriza pocos cables, el SPI prioriza **velocidad** y
**full-duplex** (transmite y recibe a la vez).

Usa cuatro señales:

| Señal | Significado |
|-------|-------------|
| **SCLK** | reloj, lo genera el maestro |
| **MOSI** | Master Out, Slave In: datos del maestro al esclavo |
| **MISO** | Master In, Slave Out: datos del esclavo al maestro |
| **SS / CS** | Slave Select (Chip Select): el maestro lo baja para hablarle a *ese* esclavo |

No hay direcciones como en I2C: cada esclavo tiene su propia línea **CS**. Para hablar con uno, el
maestro baja su CS, intercambia bytes y lo sube.

El LPC1769 tiene dos controladores recomendados, **SSP0** y **SSP1** (Synchronous Serial Port), más un
**SPI** legacy. Usaremos **SSP**, que es el moderno. Capítulos 17 (SPI) y 18 (SSP).

## Recorrido

1. [01 - SPI: el protocolo, los 4 modos y el SPI legacy a registro](./01-spi-registros.md)
   Las 4 señales, full-duplex, multi-esclavo y daisy chain, los 4 modos CPOL/CPHA en detalle, y el
   SPI legacy (`SPCR`/`SPSR`/`SPDR`/`SPCCR`) con su ritual frágil de flags. Por qué se prefiere el SSP.
2. [02 - SSP a registro y con el driver CMSIS](./02-ssp-con-driver.md)
   Registros del SSP (`CR0`/`CR1`/`DR`/`SR`/`CPSR`), las FIFOs de 8, el cálculo de clock
   (CPSR×SCR), los tres formatos de trama, y el driver `SSP_Init` / `SSP_ReadWrite` con un ejemplo
   de memoria flash SPI.
3. [03 - SSP por interrupción y DMA](./03-ssp-interrupciones-dma.md)
   `IMSC`/`RIS`/`MIS`/`ICR` y las cuatro fuentes (ROR/RT/RX/TX), recepción por interrupción, y
   `DMACR` + GPDMA para bloques grandes. Cuándo usar polling, interrupción o DMA.

## El ritual de arranque aplicado al SSP
1. **Encender** → `PCONP` (SSP0 = bit 21, SSP1 = bit 10; SPI legacy = bit 8).
2. **Clockear** → `PCLKSEL` (define junto a `CPSR`/`SCR` la velocidad de SCK; por defecto `CCLK/4`).
3. **Pines** → `PINSEL` (SSP0: SCK0=P0.15, SSEL0=P0.16, MISO0=P0.17, MOSI0=P0.18, función 2;
   SSP1: SSEL1=P0.6, SCK1=P0.7, MISO1=P0.8, MOSI1=P0.9, función 2). El CS suele manejarse como
   GPIO común.
4. **Configurar** → tamaño de palabra (4–16 bits), formato (SPI/TI/Microwire), modo (CPOL/CPHA),
   velocidad (`CPSR` + `SCR`). Configurá todo **antes** de poner `SSE=1`.
5. **Usar** → escribir en `DR`, leer `DR` (cada transferencia es simultánea TX y RX, vía FIFO).

## Antes de esto
Módulos 3 (clock/power), 4 (PINSEL), 5 (GPIO, para el CS).

## Manual
SSP Capítulo 18 ([`manual/ch18_ssp0-1.pdf`](../../manual/ch18_ssp0-1.pdf)), SPI legacy Capítulo 17
([`manual/ch17_spi.pdf`](../../manual/ch17_spi.pdf)). Ejemplos:
[`../../library/examples/SSP/`](../../library/examples/SSP/) y `SPI/`.

---

**Anterior:** [13 - I2C](../13_i2c/) · **Siguiente:** [15 - USB](../15_usb/)
