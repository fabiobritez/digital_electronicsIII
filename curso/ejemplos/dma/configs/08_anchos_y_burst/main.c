#include <stddef.h>
#include <stdint.h>

#include "lpc17xx_gpdma.h"

#define DMA_MAX_TRANSFERENCIAS 4095u

static const uint8_t origen_demo[16] = {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u};
static uint32_t destino_demo[4];

// Empaqueta cuatro bytes consecutivos en cada word destino.
Status config_dma_m2m_bytes_a_words(const uint8_t *origen, uint32_t *destino, size_t cantidad_bytes)
{
    if (cantidad_bytes == 0u || (cantidad_bytes % sizeof(*destino)) != 0u || (cantidad_bytes / sizeof(*destino)) > DMA_MAX_TRANSFERENCIAS) {
        return ERROR;
    }

    GPDMA_Channel_CFG_T cfg;
    cfg.channelNum = GPDMA_CH_7; // Menor prioridad para M2M.
    cfg.transferSize = (uint32_t)(cantidad_bytes / sizeof(*destino)); // Una transferencia por word destino.
    cfg.type = GPDMA_M2M; // Conversión de ancho en memoria.
    cfg.srcMemAddr = (uint32_t)(uintptr_t)origen; // Buffer de bytes.
    cfg.dstMemAddr = (uint32_t)(uintptr_t)destino; // Buffer de words.
    cfg.srcConn = GPDMA_ADC; // Ignorado en M2M.
    cfg.dstConn = GPDMA_ADC; // Ignorado en M2M.
    cfg.src.width = GPDMA_BYTE; // Lee el origen de a 8 bits.
    cfg.src.burst = GPDMA_BSIZE_32; // Lee 32 bytes por burst.
    cfg.src.increment = ENABLE; // Avanza por cada byte.
    cfg.dst.width = GPDMA_WORD; // Escribe el destino de a 32 bits.
    cfg.dst.burst = GPDMA_BSIZE_8; // Escribe 8 words por burst.
    cfg.dst.increment = ENABLE; // Avanza por cada word.
    cfg.intTC = ENABLE; // Señala el fin.
    cfg.intErr = ENABLE; // Señala errores.
    cfg.linkedList = 0u; // Sin encadenamiento.
    return GPDMA_SetupChannel(&cfg);
}

int main(void)
{
    GPDMA_Init(); // Inicializa el controlador DMA.
    // Configura el empaquetado de 16 bytes en cuatro words.
    if (config_dma_m2m_bytes_a_words(origen_demo, destino_demo, sizeof(origen_demo)) != SUCCESS) {
        while (1) {} // Se detiene si la configuración no es válida.
    }
    GPDMA_ChannelStart(GPDMA_CH_7); // Inicia la transferencia M2M.
    while (GPDMA_IntGetStatus(GPDMA_ENABLED_CH, GPDMA_CH_7) == SET) {} // Espera por polling.
    while (1) {} // Permite revisar destino_demo desde el debugger.
}
