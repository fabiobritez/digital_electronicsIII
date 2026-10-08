# UART por DMA

UART0 transmite `UART0 por DMA` a 115200 baud, 8N1, usando el canal DMA 3. El FIFO solicita nuevos
bytes sin que el CPU tenga que escribirlos uno por uno.

## Pines usados

| UART | TX | RX |
|---|---|---|
| UART0 | P0.2 | P0.3 |
| UART1 | P0.15 | P0.16 |
| UART2 | P0.10 | P0.11 |
| UART3 | P0.0 | P0.1 |

Para transmitir, conectá TX de la placa con RX del adaptador. Para recibir, conectá TX del adaptador
con RX de la placa. En ambos casos uní las masas y usá señales de 3,3 V.

## Transmisión

`config_dma_uart_tx()` recorre el mensaje en RAM y escribe siempre en el FIFO Tx. El FIFO solicita
cada byte cuando dispone de espacio. El `main()` utiliza esta variante con UART0.

## Recepción

`config_dma_uart_rx()` lee siempre el FIFO Rx y avanza por el buffer de memoria. Rx usa un canal de
mayor prioridad que Tx porque un byte recibido puede perderse si no se retira a tiempo.

## Full-duplex

`config_dma_uart_full_duplex()` configura dos canales independientes, uno por dirección. Primero se
inicia Rx y después Tx para no perder una respuesta que llegue apenas comienza la transmisión.

El parámetro `numero` permite repetir las tres configuraciones con UART0, UART1, UART2 o UART3.

## Compilar

```bash
cp curso/ejemplos/dma/configs/05_uart/main.c plantilla/src/main.c
make -C plantilla USE_CMSIS=1
```
