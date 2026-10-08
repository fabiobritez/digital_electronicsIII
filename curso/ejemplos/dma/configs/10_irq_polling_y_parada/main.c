#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lpc17xx_gpdma.h"

#define CANTIDAD 64u

static uint32_t origen_demo[CANTIDAD];
static uint32_t destino_demo[CANTIDAD];
static GPDMA_LLI_T lli_anillo;

volatile bool dma_fin[8];
volatile bool dma_error[8];

void DMA_IRQHandler(void)
{
    for (GPDMA_CH canal = GPDMA_CH_0; canal <= GPDMA_CH_7; ++canal) {
        if (GPDMA_IntGetStatus(GPDMA_INTTC, canal) == SET) {
            GPDMA_ClearIntPending(GPDMA_CLR_INTTC, canal);
            dma_fin[canal] = true;
        }
        if (GPDMA_IntGetStatus(GPDMA_INTERR, canal) == SET) {
            GPDMA_ClearIntPending(GPDMA_CLR_INTERR, canal);
            dma_error[canal] = true;
        }
    }
}

bool esperar_dma_por_polling(GPDMA_CH canal)
{
    while (GPDMA_IntGetStatus(GPDMA_ENABLED_CH, canal) == SET) {
        if (GPDMA_IntGetStatus(GPDMA_INTERR, canal) == SET) {
            GPDMA_ClearIntPending(GPDMA_CLR_INTERR, canal);
            return false;
        }
    }
    return true;
}

void pausar_y_reanudar_dma(GPDMA_CH canal)
{
    GPDMA_ChannelPause(canal); // Conserva el estado y drena el FIFO.
    GPDMA_ChannelResume(canal); // Continúa desde el punto pausado.
}

void detener_dma_sin_perder_datos(GPDMA_CH canal)
{
    GPDMA_ChannelGracefulStop(canal); // Para reutilizar, configurar de nuevo.
}

Status config_dma_m2m_anillo(void)
{
    const uint32_t *origen = origen_demo;
    uint32_t *destino = destino_demo;
    const size_t cantidad = CANTIDAD;
    uint32_t control = 0u;
    control |= GPDMA_DMACCxControl_TransferSize(cantidad); // Words por vuelta.
    control |= GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_32); // Lee hasta 32 words por burst.
    control |= GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_32); // Escribe hasta 32 words por burst.
    control |= GPDMA_DMACCxControl_SWidth(GPDMA_WORD); // Origen de 32 bits.
    control |= GPDMA_DMACCxControl_DWidth(GPDMA_WORD); // Destino de 32 bits.
    control |= GPDMA_DMACCxControl_SI; // Avanza por el origen.
    control |= GPDMA_DMACCxControl_DI; // Avanza por el destino.
    control |= GPDMA_DMACCxControl_I; // Interrumpe al completar una vuelta.
    lli_anillo.srcAddr = (uint32_t)(uintptr_t)origen; // Reinicia el origen.
    lli_anillo.dstAddr = (uint32_t)(uintptr_t)destino; // Reinicia el destino.
    lli_anillo.nextLLI = (uint32_t)(uintptr_t)&lli_anillo; // Se enlaza consigo misma.
    lli_anillo.control = control; // Conserva la misma configuración.

    GPDMA_Channel_CFG_T config;
    config.channelNum = GPDMA_CH_7; // Menor prioridad para M2M.
    config.transferSize = (uint32_t)cantidad; // Words por vuelta.
    config.type = GPDMA_M2M; // Memoria a memoria.
    config.srcMemAddr = (uint32_t)(uintptr_t)origen; // Inicio del origen.
    config.dstMemAddr = (uint32_t)(uintptr_t)destino; // Inicio del destino.
    config.srcConn = GPDMA_ADC; // Ignorado en M2M.
    config.dstConn = GPDMA_ADC; // Ignorado en M2M.
    config.src.width = GPDMA_WORD; // Lee el origen de a 32 bits.
    config.src.burst = GPDMA_BSIZE_32; // Agrupa hasta 32 lecturas.
    config.src.increment = ENABLE; // Recorre el origen.
    config.dst.width = GPDMA_WORD; // Escribe el destino de a 32 bits.
    config.dst.burst = GPDMA_BSIZE_32; // Agrupa hasta 32 escrituras.
    config.dst.increment = ENABLE; // Recorre el destino.
    config.intTC = ENABLE; // Habilita TC de la LLI.
    config.intErr = ENABLE; // Interrumpe ante error.
    config.linkedList = (uint32_t)(uintptr_t)&lli_anillo; // Repite indefinidamente.
    return GPDMA_SetupChannel(&config);
}

int main(void)
{
    // Prepara un bloque con valores conocidos para la copia circular.
    for (size_t i = 0u; i < CANTIDAD; ++i) {
        origen_demo[i] = (uint32_t)i;
    }
    GPDMA_Init(); // Inicializa el controlador DMA.
    // Configura una LLI circular para mantener el canal activo.
    if (config_dma_m2m_anillo() != SUCCESS) {
        while (1) {} // Se detiene si la configuración no es válida.
    }
    NVIC_EnableIRQ(DMA_IRQn); // Atiende cada vuelta y cualquier error.
    GPDMA_ChannelStart(GPDMA_CH_7); // Inicia las copias circulares.
    pausar_y_reanudar_dma(GPDMA_CH_7); // Demuestra una pausa que conserva el estado.
    detener_dma_sin_perder_datos(GPDMA_CH_7); // Detiene el canal después de drenar el FIFO.
    while (1) {} // El canal quedó detenido y puede inspeccionarse.
}
