# Módulo 11: DMA, transferencias sin CPU

El **DMA** (Direct Memory Access) es un "robot copiador" de datos. Normalmente mover datos (de un
periférico a memoria, de memoria a memoria, de memoria a un periférico) lo hace el CPU, dato por dato,
gastando tiempo. El **GPDMA** del LPC1769 hace esas transferencias **por su cuenta**, en paralelo,
dejando al CPU libre. Tiene **8 canales** independientes.

Es el periférico más avanzado del curso, pero la idea es simple: "copiá N datos de acá para allá,
avisame cuando termines".

## Recorrido

1. [01 - DMA: concepto y registros](./01-dma-concepto-y-registros.md)
   Los 8 canales y su prioridad, los tipos de transferencia (M2M/M2P/P2M/P2P), flow control y DMA
   request, y el mapa completo de registros: globales (`DMACConfig`, `DMACIntStat/TCStat/ErrStat`,
   `DMACEnbldChns`, `DMACSoftBReq/SReq`, `DMACSync`) y por canal (`DMACCSrcAddr`, `DMACCDestAddr`,
   `DMACCControl`, `DMACCConfig`, `DMACCLLI`) campo por campo.
2. [02 - DMA con el driver CMSIS](./02-dma-con-driver.md)
   `GPDMA_Init`/`GPDMA_Setup`, la struct `GPDMA_Channel_CFG_Type`, las request lines (`GPDMA_CONN_*`) y
   el multiplexado `DMAREQSEL`. Ejemplos M2M y P2M (ADC→buffer), combos reales y errores comunes.
3. [03 - Linked lists y transferencias circulares](./03-linked-lists.md)
   La struct `GPDMA_LLI_Type`, scatter-gather, partir transferencias > 4095, el anillo (seno continuo
   por DAC) y el doble buffer ping-pong de ADC.

## Para qué se usa
- **ADC → memoria:** muestrear a alta velocidad sin leer cada muestra con el CPU.
- **Memoria → DAC:** reproducir una forma de onda continua (tabla de seno).
- **Memoria → memoria:** copiar bloques grandes rápido.
- **UART/SPI/I2S ↔ memoria:** streaming sin saturar el CPU.

## Antes de esto
Es el último periférico: conviene tener vistos los módulos 7 (interrupciones), 8 (timers) y
10 (ADC/DAC), porque el DMA se combina con todos ellos.

## Manual
Capítulo 31: [`manual/ch31_general-purpose-dma.pdf`](../../manual/ch31_general-purpose-dma.pdf).
Material original en [`_origen/`](./_origen/), ejemplos en [`../ejemplos/dma/`](../ejemplos/dma/).

---

**Anterior:** [10 - ADC/DAC](../10_adc_dac/) · **Siguiente:** [12 - Debug](../12_debug/)
