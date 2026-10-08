#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lpc17xx_gpdma.h"

#define CANTIDAD 16u

static uint32_t origen_a[CANTIDAD];
static uint32_t origen_b[CANTIDAD];
static uint32_t origen_c[CANTIDAD];
static uint32_t destino_demo[3u * CANTIDAD];
static const uint32_t valor_fill = 0xA5A5A5A5u;
static volatile bool dma_fin;
static volatile bool dma_error;

static uint32_t control_lli(void)
{
    const size_t cantidad = CANTIDAD;
    uint32_t control = 0u;
    control |= GPDMA_DMACCxControl_TransferSize(cantidad); // Cantidad de words del bloque.
    control |= GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_32); // Lee hasta 32 words por burst.
    control |= GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_32); // Escribe hasta 32 words por burst.
    control |= GPDMA_DMACCxControl_SWidth(GPDMA_WORD); // El origen se lee de a 32 bits.
    control |= GPDMA_DMACCxControl_DWidth(GPDMA_WORD); // El destino se escribe de a 32 bits.
    control |= GPDMA_DMACCxControl_SI; // Avanza la dirección de origen.
    control |= GPDMA_DMACCxControl_DI; // Avanza la dirección de destino.
    return control;
}

// M2M arranca sin requests. Se usa canal 7, la menor prioridad.
Status config_dma_m2m_words(void)
{
    const uint32_t *origen = origen_a;
    uint32_t *destino = destino_demo;
    const size_t cantidad = CANTIDAD;

    GPDMA_Channel_CFG_T cfg;
    cfg.channelNum = GPDMA_CH_7; // Menor prioridad para M2M.
    cfg.transferSize = (uint32_t)cantidad; // Cantidad de words.
    cfg.type = GPDMA_M2M; // Memoria a memoria, sin request.
    cfg.srcMemAddr = (uint32_t)(uintptr_t)origen; // Inicio del origen.
    cfg.dstMemAddr = (uint32_t)(uintptr_t)destino; // Inicio del destino.
    cfg.srcConn = GPDMA_ADC; // Ignorado en M2M.
    cfg.dstConn = GPDMA_ADC; // Ignorado en M2M.
    cfg.src.width = GPDMA_WORD; // Lee el origen de a 32 bits.
    cfg.src.burst = GPDMA_BSIZE_32; // Agrupa hasta 32 lecturas.
    cfg.src.increment = ENABLE; // Avanza por el origen.
    cfg.dst.width = GPDMA_WORD; // Escribe el destino de a 32 bits.
    cfg.dst.burst = GPDMA_BSIZE_32; // Agrupa hasta 32 escrituras.
    cfg.dst.increment = ENABLE; // Avanza por el destino.
    cfg.intTC = ENABLE; // Interrumpe al completar.
    cfg.intErr = ENABLE; // Interrumpe ante error.
    cfg.linkedList = 0u; // Sin LLI.
    return GPDMA_SetupChannel(&cfg);
}

// Un origen fijo permite llenar un buffer con la misma word: SI=0, DI=1.
Status config_dma_m2m_fill(void)
{
    const uint32_t *valor = &valor_fill;
    uint32_t *destino = destino_demo;
    const size_t cantidad = CANTIDAD;

    GPDMA_Channel_CFG_T cfg;
    cfg.channelNum = GPDMA_CH_7; // Menor prioridad para M2M.
    cfg.transferSize = (uint32_t)cantidad; // Cantidad de words.
    cfg.type = GPDMA_M2M; // Memoria a memoria, sin request.
    cfg.srcMemAddr = (uint32_t)(uintptr_t)valor; // Word que se repite.
    cfg.dstMemAddr = (uint32_t)(uintptr_t)destino; // Inicio del destino.
    cfg.srcConn = GPDMA_ADC; // Ignorado en M2M.
    cfg.dstConn = GPDMA_ADC; // Ignorado en M2M.
    cfg.src.width = GPDMA_WORD; // Lee el origen de a 32 bits.
    cfg.src.burst = GPDMA_BSIZE_32; // Agrupa hasta 32 lecturas.
    cfg.src.increment = DISABLE; // Repite siempre la misma word.
    cfg.dst.width = GPDMA_WORD; // Escribe el destino de a 32 bits.
    cfg.dst.burst = GPDMA_BSIZE_32; // Agrupa hasta 32 escrituras.
    cfg.dst.increment = ENABLE; // Avanza por el buffer.
    cfg.intTC = ENABLE; // Interrumpe al completar.
    cfg.intErr = ENABLE; // Interrumpe ante error.
    cfg.linkedList = 0u; // Sin LLI.
    return GPDMA_SetupChannel(&cfg);
}

// Los registros describen A; las dos LLI describen B y C.
static GPDMA_LLI_T lli_m2m_bloques[2];

Status config_dma_m2m_tres_bloques(void)
{
    const uint32_t *a = origen_a;
    const uint32_t *b = origen_b;
    const uint32_t *c = origen_c;
    uint32_t *destino = destino_demo;
    const size_t cantidad_por_bloque = CANTIDAD;

    const uint32_t control = control_lli();

    lli_m2m_bloques[0].srcAddr = (uint32_t)(uintptr_t)b; // Segundo bloque de origen.
    lli_m2m_bloques[0].dstAddr = (uint32_t)(uintptr_t)&destino[cantidad_por_bloque]; // Continúa detrás de A.
    lli_m2m_bloques[0].nextLLI = (uint32_t)(uintptr_t)&lli_m2m_bloques[1]; // Luego procesa C.
    lli_m2m_bloques[0].control = control; // Sin IRQ intermedia.

    lli_m2m_bloques[1].srcAddr = (uint32_t)(uintptr_t)c; // Tercer bloque de origen.
    lli_m2m_bloques[1].dstAddr = (uint32_t)(uintptr_t)&destino[2u * cantidad_por_bloque]; // Continúa detrás de B.
    lli_m2m_bloques[1].nextLLI = 0u; // Fin de la cadena.
    lli_m2m_bloques[1].control = control | GPDMA_DMACCxControl_I; // Genera la IRQ final.

    GPDMA_Channel_CFG_T cfg;
    cfg.channelNum = GPDMA_CH_7; // Menor prioridad para M2M.
    cfg.transferSize = (uint32_t)cantidad_por_bloque; // Tamaño del primer bloque.
    cfg.type = GPDMA_M2M; // Memoria a memoria.
    cfg.srcMemAddr = (uint32_t)(uintptr_t)a; // A se carga en el canal.
    cfg.dstMemAddr = (uint32_t)(uintptr_t)destino; // Comienza al inicio del destino.
    cfg.srcConn = GPDMA_ADC; // Ignorado en M2M.
    cfg.dstConn = GPDMA_ADC; // Ignorado en M2M.
    cfg.src.width = GPDMA_WORD; // Lee el origen de a 32 bits.
    cfg.src.burst = GPDMA_BSIZE_32; // Agrupa hasta 32 lecturas.
    cfg.src.increment = ENABLE; // Recorre A.
    cfg.dst.width = GPDMA_WORD; // Escribe el destino de a 32 bits.
    cfg.dst.burst = GPDMA_BSIZE_32; // Agrupa hasta 32 escrituras.
    cfg.dst.increment = ENABLE; // Recorre el destino.
    cfg.intTC = DISABLE; // La LLI final genera TC.
    cfg.intErr = ENABLE; // Interrumpe ante error.
    cfg.linkedList = (uint32_t)(uintptr_t)&lli_m2m_bloques[0]; // Después de A procesa B.
    return GPDMA_SetupChannel(&cfg);
}

void DMA_IRQHandler(void)
{
    if (GPDMA_IntGetStatus(GPDMA_INTTC, GPDMA_CH_7) == SET) {
        GPDMA_ClearIntPending(GPDMA_CLR_INTTC, GPDMA_CH_7);
        dma_fin = true;
    }
    if (GPDMA_IntGetStatus(GPDMA_INTERR, GPDMA_CH_7) == SET) {
        GPDMA_ClearIntPending(GPDMA_CLR_INTERR, GPDMA_CH_7);
        dma_error = true;
    }
}

int main(void)
{
    // Prepara tres bloques con valores distintos.
    for (size_t i = 0u; i < CANTIDAD; ++i) {
        origen_a[i] = (uint32_t)i;
        origen_b[i] = (uint32_t)(100u + i);
        origen_c[i] = (uint32_t)(200u + i);
    }

    GPDMA_Init(); // Inicializa el controlador y sus ocho canales.
    // Configura una cadena que copia A, luego B y finalmente C.
    if (config_dma_m2m_tres_bloques() != SUCCESS) {
        while (1) {} // Se detiene si la configuración no es válida.
    }
    NVIC_EnableIRQ(DMA_IRQn); // Habilita la interrupción compartida del DMA.
    GPDMA_ChannelStart(GPDMA_CH_7); // Inicia la copia del primer bloque.
    while (!dma_fin && !dma_error) {} // Espera el final de la cadena o un error.
    while (1) {} // Permite revisar destino_demo desde el debugger.
}
