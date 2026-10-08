#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lpc17xx_gpdma.h"
#include "lpc17xx_uart.h"

typedef struct {
    LPC_UART_TypeDef *uart;
    GPDMA_CONNECTION tx;
    GPDMA_CONNECTION rx;
    UART_PIN_OPTION pin_tx;
    UART_PIN_OPTION pin_rx;
} uart_dma_t;

static const uart_dma_t uart_dma[4] = {
    {(LPC_UART_TypeDef *)LPC_UART0, GPDMA_UART0_Tx, GPDMA_UART0_Rx, UART_TX0_P0_2, UART_RX0_P0_3},
    {(LPC_UART_TypeDef *)LPC_UART1, GPDMA_UART1_Tx, GPDMA_UART1_Rx, UART_TX1_P0_15, UART_RX1_P0_16},
    {(LPC_UART_TypeDef *)LPC_UART2, GPDMA_UART2_Tx, GPDMA_UART2_Rx, UART_TX2_P0_10, UART_RX2_P0_11},
    {(LPC_UART_TypeDef *)LPC_UART3, GPDMA_UART3_Tx, GPDMA_UART3_Rx, UART_TX3_P0_0, UART_RX3_P0_1},
};

static const uint8_t mensaje_uart[] = "UART por DMA\r\n";
static uint8_t recepcion_uart[sizeof(mensaje_uart) - 1u];

static void config_uart_dma(unsigned numero)
{
    UART_CFG_T uart;
    uart.baudRate = 115200u; // Velocidad en bits por segundo.
    uart.parity = UART_PARITY_NONE; // Sin paridad.
    uart.dataBits = UART_DBITS_8; // Ocho bits de datos.
    uart.stopBits = UART_STOPBIT_1; // Un bit de stop.

    UART_FIFO_CFG_T fifo;
    fifo.resetRxBuf = ENABLE; // Vacía el FIFO de recepción.
    fifo.resetTxBuf = ENABLE; // Vacía el FIFO de transmisión.
    fifo.dmaMode = ENABLE; // Las requests se dirigen al DMA.
    fifo.level = UART_FIFO_TRGLEV0; // Request Rx desde un byte.
    UART_Init(uart_dma[numero].uart, &uart);
    UART_FIFOConfig(uart_dma[numero].uart, &fifo);
}

static Status config_canal_dma_uart_tx(unsigned numero)
{
    const uint8_t *datos = mensaje_uart;
    const size_t cantidad = sizeof(mensaje_uart) - 1u;

    GPDMA_Channel_CFG_T cfg;
    cfg.channelNum = GPDMA_CH_3; // Prioridad menor que Rx.
    cfg.transferSize = (uint32_t)cantidad; // Cantidad de bytes.
    cfg.type = GPDMA_M2P; // Memoria a UART Tx.
    cfg.srcMemAddr = (uint32_t)datos; // Inicio del mensaje.
    cfg.dstMemAddr = 0u; // El driver obtiene THR.
    cfg.srcConn = 0; // Ignorado en M2P.
    cfg.dstConn = uart_dma[numero].tx; // Request del FIFO Tx elegido.
    cfg.src.width = GPDMA_BYTE; // Lee el mensaje de a un byte.
    cfg.src.burst = GPDMA_BSIZE_1; // Envía un byte por request.
    cfg.src.increment = ENABLE; // Recorre el mensaje.
    cfg.dst.width = GPDMA_BYTE; // Escribe un byte en THR.
    cfg.dst.burst = GPDMA_BSIZE_1; // Una escritura por request.
    cfg.dst.increment = DISABLE; // THR queda fijo.
    cfg.intTC = DISABLE; // No usa interrupción TC.
    cfg.intErr = DISABLE; // No usa interrupción de error.
    cfg.linkedList = 0u; // Un único bloque.
    return GPDMA_SetupChannel(&cfg);
}

static Status config_canal_dma_uart_rx(unsigned numero)
{
    uint8_t *datos = recepcion_uart;
    const size_t cantidad = sizeof(recepcion_uart);

    GPDMA_Channel_CFG_T cfg;
    cfg.channelNum = GPDMA_CH_1; // Rx tiene más prioridad que Tx.
    cfg.transferSize = (uint32_t)cantidad; // Cantidad de bytes.
    cfg.type = GPDMA_P2M; // UART Rx a memoria.
    cfg.srcMemAddr = 0u; // El driver obtiene RBR.
    cfg.dstMemAddr = (uint32_t)datos; // Inicio del buffer.
    cfg.srcConn = uart_dma[numero].rx; // Request del FIFO Rx elegido.
    cfg.dstConn = 0; // Ignorado en P2M.
    cfg.src.width = GPDMA_BYTE; // Lee un byte desde RBR.
    cfg.src.burst = GPDMA_BSIZE_1; // Recibe un byte por request.
    cfg.src.increment = DISABLE; // RBR queda fijo.
    cfg.dst.width = GPDMA_BYTE; // Guarda un byte por posición.
    cfg.dst.burst = GPDMA_BSIZE_1; // Una escritura por request.
    cfg.dst.increment = ENABLE; // Recorre el buffer.
    cfg.intTC = DISABLE; // No usa interrupción TC.
    cfg.intErr = DISABLE; // No usa interrupción de error.
    cfg.linkedList = 0u; // Un único bloque.
    return GPDMA_SetupChannel(&cfg);
}

Status config_dma_uart_tx(unsigned numero)
{
    if (numero > 3u) {
        return ERROR;
    }
    UART_PinConfig(uart_dma[numero].pin_tx); // Configura solamente el pin usado.
    config_uart_dma(numero); // Prepara formato y FIFO.
    return config_canal_dma_uart_tx(numero);
}

Status config_dma_uart_rx(unsigned numero)
{
    if (numero > 3u) {
        return ERROR;
    }
    UART_PinConfig(uart_dma[numero].pin_rx); // Configura solamente el pin usado.
    config_uart_dma(numero); // Prepara formato y FIFO.
    return config_canal_dma_uart_rx(numero);
}

// Full-duplex = dos transferencias unidireccionales y dos canales.
Status config_dma_uart_full_duplex(unsigned numero)
{
    if (numero > 3u) {
        return ERROR;
    }
    UART_PinConfig(uart_dma[numero].pin_tx); // Habilita la salida serial.
    UART_PinConfig(uart_dma[numero].pin_rx); // Habilita la entrada serial.
    config_uart_dma(numero); // Inicializa una sola vez el periférico.
    const Status estado_rx = config_canal_dma_uart_rx(numero);
    const Status estado_tx = config_canal_dma_uart_tx(numero);
    return (estado_rx == SUCCESS && estado_tx == SUCCESS) ? SUCCESS : ERROR;
}

int main(void)
{
    GPDMA_Init(); // Inicializa el controlador DMA.
    // Configura UART0 y una transferencia de memoria al FIFO Tx.
    if (config_dma_uart_tx(0u) != SUCCESS) {
        while (1) {} // Se detiene si la configuración no es válida.
    }
    GPDMA_ChannelStart(GPDMA_CH_3); // Inicia el envío del mensaje.
    while (GPDMA_IntGetStatus(GPDMA_ENABLED_CH, GPDMA_CH_3) == SET && GPDMA_IntGetStatus(GPDMA_RAW_INTERR, GPDMA_CH_3) == RESET) {} // Espera por polling.
    if (GPDMA_IntGetStatus(GPDMA_RAW_INTERR, GPDMA_CH_3) == SET) {
        GPDMA_ClearIntPending(GPDMA_CLR_INTERR, GPDMA_CH_3); // Limpia el error detectado.
        while (1) {}
    }
    while (1) {} // La transmisión terminó.
}
