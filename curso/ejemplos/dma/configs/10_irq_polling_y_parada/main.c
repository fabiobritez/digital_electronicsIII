#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lpc17xx_gpdma.h"

#define CANTIDAD 64u

static uint32_t origen_demo[CANTIDAD];
static uint32_t destino_demo[CANTIDAD];
static GPDMA_LLI_T lli_m2m_anillo;

static volatile bool dma_transferencia_completa[8];
static volatile bool dma_error[8];
static volatile bool polling_ok;

static uint32_t control_lli_m2m_anillo(void)
{
    const size_t cantidad = CANTIDAD;
    uint32_t control = 0u;
    control |= GPDMA_DMACCxControl_TransferSize(cantidad); // Words por vuelta.
    control |= GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_32); // Lee hasta 32 words por burst.
    control |= GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_32); // Escribe hasta 32 words por burst.
    control |= GPDMA_DMACCxControl_SWidth(GPDMA_WORD); // Origen de 32 bits.
    control |= GPDMA_DMACCxControl_DWidth(GPDMA_WORD); // Destino de 32 bits.
    control |= GPDMA_DMACCxControl_SI; // Avanza por el origen.
    control |= GPDMA_DMACCxControl_DI; // Avanza por el destino.
    return control; // El anillo no genera TC en cada vuelta.
}

void DMA_IRQHandler(void)
{
    for (GPDMA_CH canal = GPDMA_CH_0; canal <= GPDMA_CH_7; ++canal) {
        if (GPDMA_IntGetStatus(GPDMA_INTTC, canal) == SET) {
            GPDMA_ClearIntPending(GPDMA_CLR_INTTC, canal);
            dma_transferencia_completa[canal] = true;
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
        if (GPDMA_IntGetStatus(GPDMA_RAW_INTERR, canal) == SET) {
            GPDMA_ClearIntPending(GPDMA_CLR_INTERR, canal);
            return false;
        }
    }
    if (GPDMA_IntGetStatus(GPDMA_RAW_INTERR, canal) == SET) {
        GPDMA_ClearIntPending(GPDMA_CLR_INTERR, canal);
        return false;
    }
    return true;
}

void pausar_y_reanudar_dma(void)
{
    const GPDMA_CH canal = GPDMA_CH_7;
    GPDMA_ChannelPause(canal); // Bloquea transferencias nuevas y conserva el estado.
    GPDMA_ChannelResume(canal); // Continúa desde el punto pausado.
}

void detener_dma_sin_perder_datos(void)
{
    const GPDMA_CH canal = GPDMA_CH_7;
    GPDMA_ChannelGracefulStop(canal); // Drena el FIFO y deshabilita el canal.
}

Status config_dma_m2m_interrupcion(void)
{
    const uint32_t *origen = origen_demo;
    uint32_t *destino = destino_demo;
    const size_t cantidad = CANTIDAD;

    GPDMA_Channel_CFG_T cfg;
    cfg.channelNum = GPDMA_CH_7; // Menor prioridad para M2M.
    cfg.transferSize = (uint32_t)cantidad; // Words que se copian.
    cfg.type = GPDMA_M2M; // Memoria a memoria.
    cfg.srcMemAddr = (uint32_t)origen; // Inicio del origen.
    cfg.dstMemAddr = (uint32_t)destino; // Inicio del destino.
    cfg.srcConn = 0; // Ignorado en M2M.
    cfg.dstConn = 0; // Ignorado en M2M.
    cfg.src.width = GPDMA_WORD; // Lee words de 32 bits.
    cfg.src.burst = GPDMA_BSIZE_32; // Agrupa hasta 32 lecturas.
    cfg.src.increment = ENABLE; // Recorre el origen.
    cfg.dst.width = GPDMA_WORD; // Escribe words de 32 bits.
    cfg.dst.burst = GPDMA_BSIZE_32; // Agrupa hasta 32 escrituras.
    cfg.dst.increment = ENABLE; // Recorre el destino.
    cfg.intTC = ENABLE; // El bloque genera TC y habilita su IRQ.
    cfg.intErr = ENABLE; // Deja pasar la interrupción de error.
    cfg.linkedList = 0u; // Transferencia finita.
    return GPDMA_SetupChannel(&cfg);
}

Status config_dma_m2m_polling(void)
{
    const uint32_t *origen = origen_demo;
    uint32_t *destino = destino_demo;
    const size_t cantidad = CANTIDAD;

    GPDMA_Channel_CFG_T cfg;
    cfg.channelNum = GPDMA_CH_7; // Menor prioridad para M2M.
    cfg.transferSize = (uint32_t)cantidad; // Words que se copian.
    cfg.type = GPDMA_M2M; // Memoria a memoria.
    cfg.srcMemAddr = (uint32_t)origen; // Inicio del origen.
    cfg.dstMemAddr = (uint32_t)destino; // Inicio del destino.
    cfg.srcConn = 0; // Ignorado en M2M.
    cfg.dstConn = 0; // Ignorado en M2M.
    cfg.src.width = GPDMA_WORD; // Lee words de 32 bits.
    cfg.src.burst = GPDMA_BSIZE_32; // Agrupa hasta 32 lecturas.
    cfg.src.increment = ENABLE; // Recorre el origen.
    cfg.dst.width = GPDMA_WORD; // Escribe words de 32 bits.
    cfg.dst.burst = GPDMA_BSIZE_32; // Agrupa hasta 32 escrituras.
    cfg.dst.increment = ENABLE; // Recorre el destino.
    cfg.intTC = DISABLE; // TC queda enmascarado.
    cfg.intErr = DISABLE; // El error se consulta como estado raw.
    cfg.linkedList = 0u; // Transferencia finita.
    return GPDMA_SetupChannel(&cfg);
}

Status config_dma_m2m_anillo(void)
{
    const uint32_t *origen = origen_demo;
    uint32_t *destino = destino_demo;
    const size_t cantidad = CANTIDAD;
    const uint32_t control = control_lli_m2m_anillo();

    lli_m2m_anillo.srcAddr = (uint32_t)origen; // Reinicia el origen.
    lli_m2m_anillo.dstAddr = (uint32_t)destino; // Reinicia el destino.
    lli_m2m_anillo.nextLLI = (uint32_t)&lli_m2m_anillo; // Se enlaza consigo misma.
    lli_m2m_anillo.control = control; // Repite sin generar TC.

    GPDMA_Channel_CFG_T cfg;
    cfg.channelNum = GPDMA_CH_7; // Menor prioridad para M2M.
    cfg.transferSize = (uint32_t)cantidad; // Words por vuelta.
    cfg.type = GPDMA_M2M; // Memoria a memoria.
    cfg.srcMemAddr = (uint32_t)origen; // Inicio del origen.
    cfg.dstMemAddr = (uint32_t)destino; // Inicio del destino.
    cfg.srcConn = 0; // Ignorado en M2M.
    cfg.dstConn = 0; // Ignorado en M2M.
    cfg.src.width = GPDMA_WORD; // Lee words de 32 bits.
    cfg.src.burst = GPDMA_BSIZE_32; // Agrupa hasta 32 lecturas.
    cfg.src.increment = ENABLE; // Recorre el origen.
    cfg.dst.width = GPDMA_WORD; // Escribe words de 32 bits.
    cfg.dst.burst = GPDMA_BSIZE_32; // Agrupa hasta 32 escrituras.
    cfg.dst.increment = ENABLE; // Recorre el destino.
    cfg.intTC = DISABLE; // Evita una IRQ en cada vuelta.
    cfg.intErr = ENABLE; // Los errores sí llegan a la ISR.
    cfg.linkedList = (uint32_t)&lli_m2m_anillo; // Repite indefinidamente.
    return GPDMA_SetupChannel(&cfg);
}

int main(void)
{
    // Prepara un bloque conocido para las tres demostraciones.
    for (size_t i = 0u; i < CANTIDAD; ++i) {
        origen_demo[i] = (uint32_t)i;
    }

    GPDMA_Init(); // Inicializa el controlador DMA.
    NVIC_EnableIRQ(DMA_IRQn); // Se usa en la prueba por IRQ y para errores del anillo.

    // Primero espera una transferencia finita mediante una interrupción.
    if (config_dma_m2m_interrupcion() != SUCCESS) {
        while (1) {}
    }
    GPDMA_ChannelStart(GPDMA_CH_7);
    while (!dma_transferencia_completa[GPDMA_CH_7] && !dma_error[GPDMA_CH_7]) {}
    if (dma_error[GPDMA_CH_7]) {
        while (1) {}
    }

    // Luego repite la copia, esta vez consultando directamente el canal.
    if (config_dma_m2m_polling() != SUCCESS) {
        while (1) {}
    }
    GPDMA_ChannelStart(GPDMA_CH_7);
    polling_ok = esperar_dma_por_polling(GPDMA_CH_7);
    if (!polling_ok) {
        while (1) {}
    }

    // Finalmente mantiene un anillo activo para probar su control manual.
    if (config_dma_m2m_anillo() != SUCCESS) {
        while (1) {}
    }
    GPDMA_ChannelStart(GPDMA_CH_7);
    pausar_y_reanudar_dma(); // Conserva y recupera el estado del anillo.
    detener_dma_sin_perder_datos(); // Espera el FIFO y deshabilita el canal.
    while (1) {} // Las tres demostraciones finalizaron.
}
