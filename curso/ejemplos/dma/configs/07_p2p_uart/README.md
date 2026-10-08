# P2P: UART0 Rx a UART1 Tx

Cada byte recibido por UART0 se transfiere directamente al FIFO Tx de UART1. En este flujo P2P no
hay un buffer intermedio que el programa pueda leer o modificar.

## Conexiones

```text
fuente serial TX (3,3 V) ---- P0.3  / UART0 RX
P0.15 / UART1 TX ------------ receptor serial RX
GND ------------------------- GND común
```

## Bloque finito

`config_dma_p2p_uart0_uart1()` reenvía 32 bytes y luego detiene el canal. Tanto `RBR` como `THR`
permanecen fijos: los bytes pasan por el FIFO interno del DMA, no por un buffer en RAM.

## Reenvío continuo

`config_dma_p2p_uart0_uart1_continuo()` usa una LLI que apunta a sí misma. Cada vuelta reenvía un
bloque y genera una interrupción, pero el canal continúa activo.

## Compatibilidad

Ambas UART usan 115200 baud, 8N1. P2P necesita que origen y destino tengan formatos y velocidades
compatibles. El FIFO interno del DMA admite diferencias breves, pero no corrige baud rates distintos
ni transforma los datos.

## Compilar

```bash
cp curso/ejemplos/dma/configs/07_p2p_uart/main.c plantilla/src/main.c
make -C plantilla USE_CMSIS=1
```
