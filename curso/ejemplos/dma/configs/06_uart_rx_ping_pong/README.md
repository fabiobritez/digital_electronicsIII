# UART Rx con ping-pong

UART0 recibe continuamente a 115200 baud, 8N1. El DMA alterna dos buffers de 16 bytes y genera una
interrupción al completar cada uno.

Solamente se configura P0.3 porque la transmisión de UART0 no participa en este ejemplo.

## Conexión

```text
USB-UART TX (3,3 V) ----- P0.3 / UART0 RX
USB-UART GND ------------ GND
```

## Funcionamiento

El primer bloque se recibe en A. La siguiente LLI selecciona B y luego otra LLI vuelve a A:

```text
UART0 Rx ──→ buffer A ──→ buffer B ──→ buffer A ──→ ...
```

El FIFO Rx queda como origen fijo y solamente incrementa la dirección del buffer. Cada byte recibido
genera una request y cada bloque completo genera una interrupción.

`bloques_recibidos` cuenta bloques de 16 bytes; no representa finales de línea ni tramas de longitud
variable. Observá el contador y ambos buffers desde el debugger mientras enviás datos.

## Tiempo de procesamiento

Procesá el buffer opuesto al que el DMA está llenando. Si el procesamiento tarda más que la llegada
de 16 bytes, el DMA volverá al buffer y pisará datos todavía no consumidos.

## Compilar

```bash
cp curso/ejemplos/dma/configs/06_uart_rx_ping_pong/main.c plantilla/src/main.c
make -C plantilla USE_CMSIS=1
```
