#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lpc17xx_gpdma.h"
#include "lpc17xx_uart.h"

#define DMA_MAX_TRANSFERENCIAS 4095u

static volatile bool dma_fin;
static volatile bool dma_error;

static uint32_t control_lli_p2p(size_t cantidad)
{
    uint32_t control = 0u;
    control |= GPDMA_DMACCxControl_TransferSize(cantidad); // Bytes por bloque.
    control |= GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_1); // Lee un byte por request.
    control |= GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_1); // Escribe un byte por request.
    control |= GPDMA_DMACCxControl_SWidth(GPDMA_BYTE); // Lee RBR de a 8 bits.
    control |= GPDMA_DMACCxControl_DWidth(GPDMA_BYTE); // Escribe THR de a 8 bits.
    control |= GPDMA_DMACCxControl_I; // Interrumpe al completar el bloque.
    return control;
}

static void config_uart0_rx_uart1_tx(void)
{
    UART_CFG_T uart;
    uart.baudRate = 115200u; // Misma velocidad en ambas UART.
    uart.parity = UART_PARITY_NONE; // Sin paridad.
    uart.dataBits = UART_DBITS_8; // Ocho bits de datos.
    uart.stopBits = UART_STOPBIT_1; // Un bit de stop.

    UART_FIFO_CFG_T fifo;
    fifo.resetRxBuf = ENABLE; // Vacía el FIFO Rx.
    fifo.resetTxBuf = ENABLE; // Vacía el FIFO Tx.
    fifo.dmaMode = ENABLE; // Habilita requests DMA.
    fifo.level = UART_FIFO_TRGLEV0; // Request Rx desde un byte.
    UART_PinConfig(UART_RX0_P0_3);
    UART_PinConfig(UART_TX1_P0_15);
    UART_Init((LPC_UART_TypeDef *)LPC_UART0, &uart);
    UART_Init((LPC_UART_TypeDef *)LPC_UART1, &uart);
    UART_FIFOConfig((LPC_UART_TypeDef *)LPC_UART0, &fifo);
    UART_FIFOConfig((LPC_UART_TypeDef *)LPC_UART1, &fifo);
}

// UART0 Rx -> UART1 Tx sin pasar por un buffer de aplicación.
Status config_dma_p2p_uart0_uart1(size_t cantidad)
{
    if (cantidad == 0u || cantidad > DMA_MAX_TRANSFERENCIAS) {
        return ERROR;
    }
    config_uart0_rx_uart1_tx();

    GPDMA_Channel_CFG_T cfg;
    cfg.channelNum = GPDMA_CH_0; // Máxima prioridad para dos periféricos.
    cfg.transferSize = (uint32_t)cantidad; // Bytes que se reenvían.
    cfg.type = GPDMA_P2P; // Periférico a periférico.
    cfg.srcMemAddr = 0u; // El driver obtiene RBR.
    cfg.dstMemAddr = 0u; // El driver obtiene THR.
    cfg.srcConn = GPDMA_UART0_Rx; // UART0 produce los datos.
    cfg.dstConn = GPDMA_UART1_Tx; // UART1 consume los datos.
    cfg.src.width = GPDMA_BYTE; // Lee un byte desde RBR.
    cfg.src.burst = GPDMA_BSIZE_1; // Recibe un byte por request.
    cfg.src.increment = DISABLE; // RBR queda fijo.
    cfg.dst.width = GPDMA_BYTE; // Escribe un byte en THR.
    cfg.dst.burst = GPDMA_BSIZE_1; // Envía un byte por request.
    cfg.dst.increment = DISABLE; // THR queda fijo.
    cfg.intTC = ENABLE; // Interrumpe al completar el bloque.
    cfg.intErr = ENABLE; // Interrumpe ante error.
    cfg.linkedList = 0u; // Reenvío finito.
    return GPDMA_SetupChannel(&cfg);
}

static GPDMA_LLI_T uart_p2p_anillo;

Status config_dma_p2p_uart0_uart1_continuo(size_t bloque)
{
    if (bloque == 0u || bloque > DMA_MAX_TRANSFERENCIAS) {
        return ERROR;
    }
    config_uart0_rx_uart1_tx();

    const uint32_t control = control_lli_p2p(bloque);
    uart_p2p_anillo.srcAddr = (uint32_t)(uintptr_t)&LPC_UART0->RBR; // FIFO de entrada fijo.
    uart_p2p_anillo.dstAddr = (uint32_t)(uintptr_t)&LPC_UART1->THR; // FIFO de salida fijo.
    uart_p2p_anillo.nextLLI = (uint32_t)(uintptr_t)&uart_p2p_anillo; // Repite el bloque.
    uart_p2p_anillo.control = control; // Una IRQ por bloque.

    GPDMA_Channel_CFG_T cfg;
    cfg.channelNum = GPDMA_CH_0; // Máxima prioridad para P2P.
    cfg.transferSize = (uint32_t)bloque; // Bytes por vuelta.
    cfg.type = GPDMA_P2P; // UART0 Rx a UART1 Tx.
    cfg.srcMemAddr = 0u; // El driver obtiene RBR.
    cfg.dstMemAddr = 0u; // El driver obtiene THR.
    cfg.srcConn = GPDMA_UART0_Rx; // Request de entrada.
    cfg.dstConn = GPDMA_UART1_Tx; // Request de salida.
    cfg.src.width = GPDMA_BYTE; // Lee un byte desde RBR.
    cfg.src.burst = GPDMA_BSIZE_1; // Recibe un byte por request.
    cfg.src.increment = DISABLE; // RBR permanece fijo.
    cfg.dst.width = GPDMA_BYTE; // Escribe un byte en THR.
    cfg.dst.burst = GPDMA_BSIZE_1; // Envía un byte por request.
    cfg.dst.increment = DISABLE; // THR permanece fijo.
    cfg.intTC = ENABLE; // Habilita la IRQ de la LLI.
    cfg.intErr = ENABLE; // Interrumpe ante error.
    cfg.linkedList = (uint32_t)(uintptr_t)&uart_p2p_anillo; // Cierra el anillo.
    return GPDMA_SetupChannel(&cfg);
}

void DMA_IRQHandler(void)
{
    if (GPDMA_IntGetStatus(GPDMA_INTTC, GPDMA_CH_0) == SET) {
        GPDMA_ClearIntPending(GPDMA_CLR_INTTC, GPDMA_CH_0);
        dma_fin = true;
    }
    if (GPDMA_IntGetStatus(GPDMA_INTERR, GPDMA_CH_0) == SET) {
        GPDMA_ClearIntPending(GPDMA_CLR_INTERR, GPDMA_CH_0);
        dma_error = true;
    }
}

int main(void)
{
    GPDMA_Init(); // Inicializa el controlador DMA.
    // Configura el reenvío directo de 32 bytes entre ambas UART.
    if (config_dma_p2p_uart0_uart1(32u) != SUCCESS) {
        while (1) {} // Se detiene si la configuración no es válida.
    }
    NVIC_EnableIRQ(DMA_IRQn); // Detecta el final del bloque o un error.
    GPDMA_ChannelStart(GPDMA_CH_0); // Habilita el puente UART0 Rx a UART1 Tx.
    while (!dma_fin && !dma_error) {} // Espera que se reenvíen los 32 bytes.
    while (1) {} // El reenvío finito terminó.
}
