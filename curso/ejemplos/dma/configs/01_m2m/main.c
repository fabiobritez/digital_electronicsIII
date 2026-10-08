#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lpc17xx_gpdma.h"

#define DMA_MAX_TRANSFERENCIAS 4095u
#define CANTIDAD 16u

static uint32_t origen_a[CANTIDAD];
static uint32_t origen_b[CANTIDAD];
static uint32_t origen_c[CANTIDAD];
static uint32_t destino_demo[3u * CANTIDAD];
static volatile bool dma_fin;
static volatile bool dma_error;

static uint32_t control_lli(size_t cantidad, bool irq)
{
    uint32_t control = 0u;
    control |= GPDMA_DMACCxControl_TransferSize(cantidad); // Cantidad de words del bloque.
    control |= GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_32); // Lee hasta 32 words por burst.
    control |= GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_32); // Escribe hasta 32 words por burst.
    control |= GPDMA_DMACCxControl_SWidth(GPDMA_WORD); // El origen se lee de a 32 bits.
    control |= GPDMA_DMACCxControl_DWidth(GPDMA_WORD); // El destino se escribe de a 32 bits.
    control |= GPDMA_DMACCxControl_SI; // Avanza la dirección de origen.
    control |= GPDMA_DMACCxControl_DI; // Avanza la dirección de destino.
    if (irq) {
        control |= GPDMA_DMACCxControl_I; // Interrumpe al terminar este descriptor.
    }
    return control;
}

// M2M arranca sin requests. Se usa canal 7, la menor prioridad.
Status config_dma_m2m_words(const uint32_t *origen, uint32_t *destino, size_t cantidad)
{
    if (cantidad == 0u || cantidad > DMA_MAX_TRANSFERENCIAS) { // simplemente valida que la cantidad de words este dentro de los limites
        return ERROR;
    }

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
Status config_dma_m2m_fill(const uint32_t *valor, uint32_t *destino, size_t cantidad)
{
    if (cantidad == 0u || cantidad > DMA_MAX_TRANSFERENCIAS) {
        return ERROR;
    }

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
static GPDMA_LLI_T m2m_lli[2];

Status config_dma_m2m_tres_bloques(const uint32_t *a, const uint32_t *b, const uint32_t *c, uint32_t *destino, size_t cantidad_por_bloque)
{
    if (cantidad_por_bloque == 0u || cantidad_por_bloque > DMA_MAX_TRANSFERENCIAS) { // simplemente valida que la cantidad de words sea válida
        return ERROR;
    }

    const uint32_t control_intermedio = control_lli(cantidad_por_bloque, false);
    const uint32_t control_final = control_intermedio | GPDMA_DMACCxControl_I;

    m2m_lli[0].srcAddr = (uint32_t)(uintptr_t)b; // Segundo bloque de origen.
    m2m_lli[0].dstAddr = (uint32_t)(uintptr_t)&destino[cantidad_por_bloque]; // Continúa detrás de A.
    m2m_lli[0].nextLLI = (uint32_t)(uintptr_t)&m2m_lli[1]; // Luego procesa C.
    m2m_lli[0].control = control_intermedio; // Sin IRQ intermedia.

    m2m_lli[1].srcAddr = (uint32_t)(uintptr_t)c; // Tercer bloque de origen.
    m2m_lli[1].dstAddr = (uint32_t)(uintptr_t)&destino[2u * cantidad_por_bloque]; // Continúa detrás de B.
    m2m_lli[1].nextLLI = 0u; // Fin de la cadena.
    m2m_lli[1].control = control_final; // Genera la IRQ final.

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
    cfg.linkedList = (uint32_t)(uintptr_t)&m2m_lli[0]; // Después de A procesa B.
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
    if (config_dma_m2m_tres_bloques(origen_a, origen_b, origen_c, destino_demo, CANTIDAD) != SUCCESS) {
        while (1) {} // Se detiene si la configuración no es válida.
    }
    NVIC_EnableIRQ(DMA_IRQn); // Habilita la interrupción compartida del DMA.
    GPDMA_ChannelStart(GPDMA_CH_7); // Inicia la copia del primer bloque.
    while (!dma_fin && !dma_error) {} // Espera el final de la cadena o un error.
    while (1) {} // Permite revisar destino_demo desde el debugger.
}
