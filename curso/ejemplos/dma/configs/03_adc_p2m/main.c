#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lpc17xx_adc.h"
#include "lpc17xx_gpdma.h"

#define MUESTRAS_POR_BUFFER 32u

static uint32_t buffer_a_demo[MUESTRAS_POR_BUFFER];
static uint32_t buffer_b_demo[MUESTRAS_POR_BUFFER];
static volatile uint32_t bloques_completos;
static volatile bool dma_error;

static uint32_t control_lli_adc(void)
{
    const size_t cantidad = MUESTRAS_POR_BUFFER;
    uint32_t control = 0u;
    control |= GPDMA_DMACCxControl_TransferSize(cantidad); // Muestras por buffer.
    control |= GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_1); // Lee una muestra por request.
    control |= GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_1); // Escribe una muestra por request.
    control |= GPDMA_DMACCxControl_SWidth(GPDMA_WORD); // Lee ADGDR de a 32 bits.
    control |= GPDMA_DMACCxControl_DWidth(GPDMA_WORD); // Guarda cada muestra en una word.
    control |= GPDMA_DMACCxControl_DI; // Avanza por el buffer.
    control |= GPDMA_DMACCxControl_I; // Interrumpe al completar el buffer.
    return control;
}

static void config_adc0(void)
{
    const uint32_t muestras_por_segundo = 10000u;
    ADC_Init(muestras_por_segundo); // Define la frecuencia de muestreo.
    ADC_PinConfig(ADC_CHANNEL_0); // P0.23 como entrada AD0.0.
    ADC_ChannelEnable(ADC_CHANNEL_0); // Convierte solamente el canal 0.
    // DONE alimenta la request DMA. No se habilita ADC_IRQn en el NVIC.
    ADC_IntEnable(ADC_INT_CH0);
}

Status config_dma_adc_bloque(void)
{
    uint32_t *muestras = buffer_a_demo;
    const size_t cantidad = MUESTRAS_POR_BUFFER;
    config_adc0();

    GPDMA_Channel_CFG_T cfg;
    cfg.channelNum = GPDMA_CH_0; // Máxima prioridad para evitar overrun.
    cfg.transferSize = (uint32_t)cantidad; // Cantidad de muestras.
    cfg.type = GPDMA_P2M; // Periférico a memoria.
    cfg.srcMemAddr = 0u; // El driver obtiene ADGDR.
    cfg.dstMemAddr = (uint32_t)(uintptr_t)muestras; // Inicio del buffer.
    cfg.srcConn = GPDMA_ADC; // DONE del ADC genera la request.
    cfg.dstConn = 0; // Ignorado en P2M.
    cfg.src.width = GPDMA_WORD; // ADGDR se lee de a 32 bits.
    cfg.src.burst = GPDMA_BSIZE_1; // Una muestra por request.
    cfg.src.increment = DISABLE; // ADGDR queda fijo.
    cfg.dst.width = GPDMA_WORD; // Guarda cada resultado completo.
    cfg.dst.burst = GPDMA_BSIZE_1; // Una escritura por request.
    cfg.dst.increment = ENABLE; // Avanza por el buffer.
    cfg.intTC = ENABLE; // El bloque genera TC y habilita su IRQ.
    cfg.intErr = ENABLE; // Interrumpe ante error.
    cfg.linkedList = 0u; // Transferencia única.
    return GPDMA_SetupChannel(&cfg);
}

static GPDMA_LLI_T lli_adc_ping_pong[2];

// A -> B -> A: IRQ por cada buffer completo.
Status config_dma_adc_ping_pong(void)
{
    uint32_t *buffer_a = buffer_a_demo;
    uint32_t *buffer_b = buffer_b_demo;
    const size_t cantidad_por_buffer = MUESTRAS_POR_BUFFER;
    config_adc0();

    const uint32_t control = control_lli_adc();

    lli_adc_ping_pong[0].srcAddr = (uint32_t)(uintptr_t)&LPC_ADC->ADGDR; // Registro de resultado fijo.
    lli_adc_ping_pong[0].dstAddr = (uint32_t)(uintptr_t)buffer_a; // Primer buffer.
    lli_adc_ping_pong[0].nextLLI = (uint32_t)(uintptr_t)&lli_adc_ping_pong[1]; // Luego llena B.
    lli_adc_ping_pong[0].control = control; // Una IRQ al completar A.

    lli_adc_ping_pong[1].srcAddr = (uint32_t)(uintptr_t)&LPC_ADC->ADGDR; // Mismo registro de origen.
    lli_adc_ping_pong[1].dstAddr = (uint32_t)(uintptr_t)buffer_b; // Segundo buffer.
    lli_adc_ping_pong[1].nextLLI = (uint32_t)(uintptr_t)&lli_adc_ping_pong[0]; // Vuelve a llenar A.
    lli_adc_ping_pong[1].control = control; // Una IRQ al completar B.

    GPDMA_Channel_CFG_T cfg;
    cfg.channelNum = GPDMA_CH_0; // Máxima prioridad para el ADC.
    cfg.transferSize = (uint32_t)cantidad_por_buffer; // Muestras por buffer.
    cfg.type = GPDMA_P2M; // ADC a memoria.
    cfg.srcMemAddr = 0u; // El driver obtiene ADGDR.
    cfg.dstMemAddr = (uint32_t)(uintptr_t)buffer_a; // El primer bloque llena A.
    cfg.srcConn = GPDMA_ADC; // DONE genera cada request.
    cfg.dstConn = 0; // Ignorado en P2M.
    cfg.src.width = GPDMA_WORD; // Lee ADGDR completo.
    cfg.src.burst = GPDMA_BSIZE_1; // Una muestra por request.
    cfg.src.increment = DISABLE; // El registro permanece fijo.
    cfg.dst.width = GPDMA_WORD; // Guarda resultados de 32 bits.
    cfg.dst.burst = GPDMA_BSIZE_1; // Una escritura por request.
    cfg.dst.increment = ENABLE; // Avanza dentro de cada buffer.
    cfg.intTC = ENABLE; // A genera TC y habilita la IRQ del canal.
    cfg.intErr = ENABLE; // Interrumpe ante error.
    cfg.linkedList = (uint32_t)(uintptr_t)&lli_adc_ping_pong[1]; // Después de A carga B.
    return GPDMA_SetupChannel(&cfg);
}

void DMA_IRQHandler(void)
{
    if (GPDMA_IntGetStatus(GPDMA_INTTC, GPDMA_CH_0) == SET) {
        GPDMA_ClearIntPending(GPDMA_CLR_INTTC, GPDMA_CH_0);
        ++bloques_completos;
    }
    if (GPDMA_IntGetStatus(GPDMA_INTERR, GPDMA_CH_0) == SET) {
        GPDMA_ClearIntPending(GPDMA_CLR_INTERR, GPDMA_CH_0);
        dma_error = true;
    }
}

int main(void)
{
    GPDMA_Init(); // Inicializa el controlador DMA.
    // Configura dos buffers que se llenan de forma alternada a 10 ksample/s.
    if (config_dma_adc_ping_pong() != SUCCESS) {
        while (1) {} // Se detiene si la configuración no es válida.
    }
    NVIC_EnableIRQ(DMA_IRQn); // Cuenta cada buffer completo y detecta errores.
    GPDMA_ChannelStart(GPDMA_CH_0); // Escucha las requests del ADC.
    ADC_BurstEnable(); // Inicia las conversiones periódicas.
    while (!dma_error) {} // La adquisición continúa mientras no haya errores.
    GPDMA_ChannelGracefulStop(GPDMA_CH_0); // Drena el FIFO antes de detenerse.
    while (1) {} // Conserva los buffers para revisarlos en el debugger.
}
