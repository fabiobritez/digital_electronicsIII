#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lpc17xx_gpdma.h"
#include "lpc17xx_uart.h"

#define BYTES_POR_BUFFER 16u

static uint8_t buffer_a[BYTES_POR_BUFFER];
static uint8_t buffer_b[BYTES_POR_BUFFER];
static volatile uint32_t bloques_recibidos;
static volatile bool dma_error;

static GPDMA_LLI_T lli_uart_rx_ping_pong[2];

static uint32_t control_lli_uart(void)
{
    const size_t cantidad = BYTES_POR_BUFFER;
    uint32_t control = 0u;
    control |= GPDMA_DMACCxControl_TransferSize(cantidad); // Bytes por buffer.
    control |= GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_1); // Lee un byte por request.
    control |= GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_1); // Escribe un byte por request.
    control |= GPDMA_DMACCxControl_SWidth(GPDMA_BYTE); // Lee RBR de a 8 bits.
    control |= GPDMA_DMACCxControl_DWidth(GPDMA_BYTE); // Guarda cada dato en un byte.
    control |= GPDMA_DMACCxControl_DI; // Avanza por el buffer.
    control |= GPDMA_DMACCxControl_I; // Interrumpe al completar el buffer.
    return control;
}

Status config_dma_uart0_rx_ping_pong(void)
{
    const size_t cantidad_por_buffer = BYTES_POR_BUFFER;

    UART_CFG_T uart;
    uart.baudRate = 115200u; // Velocidad en bits por segundo.
    uart.parity = UART_PARITY_NONE; // Sin paridad.
    uart.dataBits = UART_DBITS_8; // Ocho bits de datos.
    uart.stopBits = UART_STOPBIT_1; // Un bit de stop.

    UART_FIFO_CFG_T fifo;
    fifo.resetRxBuf = ENABLE; // Descarta bytes anteriores.
    fifo.resetTxBuf = DISABLE; // Tx no participa.
    fifo.dmaMode = ENABLE; // Rx genera requests DMA.
    fifo.level = UART_FIFO_TRGLEV0; // Request desde un byte.
    UART_PinConfig(UART_RX0_P0_3);
    UART_Init((LPC_UART_TypeDef *)LPC_UART0, &uart);
    UART_FIFOConfig((LPC_UART_TypeDef *)LPC_UART0, &fifo);

    const uint32_t control = control_lli_uart();
    lli_uart_rx_ping_pong[0].srcAddr = (uint32_t)&LPC_UART0->RBR; // FIFO Rx fijo.
    lli_uart_rx_ping_pong[0].dstAddr = (uint32_t)buffer_a; // Primer buffer.
    lli_uart_rx_ping_pong[0].nextLLI = (uint32_t)&lli_uart_rx_ping_pong[1]; // Luego llena B.
    lli_uart_rx_ping_pong[0].control = control; // Una IRQ al completar A.

    lli_uart_rx_ping_pong[1].srcAddr = (uint32_t)&LPC_UART0->RBR; // Mismo FIFO de origen.
    lli_uart_rx_ping_pong[1].dstAddr = (uint32_t)buffer_b; // Segundo buffer.
    lli_uart_rx_ping_pong[1].nextLLI = (uint32_t)&lli_uart_rx_ping_pong[0]; // Vuelve a llenar A.
    lli_uart_rx_ping_pong[1].control = control; // Una IRQ al completar B.

    GPDMA_Channel_CFG_T cfg;
    cfg.channelNum = GPDMA_CH_1; // Prioridad alta para recepción.
    cfg.transferSize = (uint32_t)cantidad_por_buffer; // Bytes por buffer.
    cfg.type = GPDMA_P2M; // UART Rx a memoria.
    cfg.srcMemAddr = 0u; // El driver obtiene RBR.
    cfg.dstMemAddr = (uint32_t)buffer_a; // El primer bloque llena A.
    cfg.srcConn = GPDMA_UART0_Rx; // El FIFO Rx genera requests.
    cfg.dstConn = 0; // Ignorado en P2M.
    cfg.src.width = GPDMA_BYTE; // Lee un byte desde RBR.
    cfg.src.burst = GPDMA_BSIZE_1; // Recibe un byte por request.
    cfg.src.increment = DISABLE; // RBR queda fijo.
    cfg.dst.width = GPDMA_BYTE; // Guarda un byte por posición.
    cfg.dst.burst = GPDMA_BSIZE_1; // Una escritura por request.
    cfg.dst.increment = ENABLE; // Avanza dentro del buffer.
    cfg.intTC = ENABLE; // A genera TC y habilita la IRQ del canal.
    cfg.intErr = ENABLE; // Interrumpe ante error.
    cfg.linkedList = (uint32_t)&lli_uart_rx_ping_pong[1]; // Después de A carga B.
    return GPDMA_SetupChannel(&cfg);
}

void DMA_IRQHandler(void)
{
    if (GPDMA_IntGetStatus(GPDMA_INTTC, GPDMA_CH_1) == SET) {
        GPDMA_ClearIntPending(GPDMA_CLR_INTTC, GPDMA_CH_1);
        ++bloques_recibidos;
    }
    if (GPDMA_IntGetStatus(GPDMA_INTERR, GPDMA_CH_1) == SET) {
        GPDMA_ClearIntPending(GPDMA_CLR_INTERR, GPDMA_CH_1);
        dma_error = true;
    }
}

int main(void)
{
    GPDMA_Init(); // Inicializa el controlador DMA.
    // Configura UART0 Rx y dos buffers enlazados en forma circular.
    if (config_dma_uart0_rx_ping_pong() != SUCCESS) {
        while (1) {} // Se detiene si la configuración no es válida.
    }
    NVIC_EnableIRQ(DMA_IRQn); // Cuenta buffers completos y detecta errores.
    GPDMA_ChannelStart(GPDMA_CH_1); // Comienza a recibir bytes de UART0.
    while (!dma_error) {} // La recepción continúa mientras no haya errores.
    GPDMA_ChannelGracefulStop(GPDMA_CH_1); // Drena el FIFO antes de detenerse.
    while (1) {} // Conserva ambos buffers para revisarlos.
}
