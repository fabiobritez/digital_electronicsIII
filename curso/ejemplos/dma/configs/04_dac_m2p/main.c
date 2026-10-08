#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lpc17xx_dac.h"
#include "lpc17xx_gpdma.h"

#define DMA_MAX_TRANSFERENCIAS 4095u
#define CANTIDAD_MUESTRAS 32u

static uint32_t tabla_dac[CANTIDAD_MUESTRAS];

static uint32_t control_lli_dac(size_t cantidad)
{
    uint32_t control = 0u;
    control |= GPDMA_DMACCxControl_TransferSize(cantidad); // Muestras por vuelta.
    control |= GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_1); // Lee una muestra por request.
    control |= GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_1); // Escribe una muestra por request.
    control |= GPDMA_DMACCxControl_SWidth(GPDMA_WORD); // Lee la tabla de a 32 bits.
    control |= GPDMA_DMACCxControl_DWidth(GPDMA_WORD); // Escribe DACR de a 32 bits.
    control |= GPDMA_DMACCxControl_SI; // Avanza por la tabla.
    return control;
}

static void config_dac_sin_request(uint16_t ticks_por_muestra)
{
    DAC_CONVERTER_CFG_T ctrl;
    ctrl.doubleBuffer = ENABLE; // Actualiza la salida sin transitorios.
    ctrl.dmaCounter = ENABLE; // Usa el timeout como período.
    ctrl.dmaRequest = DISABLE; // Evita requests antes de configurar DMA.
    DAC_Init(); // Configura P0.26 como AOUT.
    DAC_SetDMATimeOut(ticks_por_muestra); // Fija el período de muestra.
    DAC_ConfigDAConverterControl(&ctrl);
}

static void habilitar_request_dac(void)
{
    DAC_CONVERTER_CFG_T ctrl;
    ctrl.doubleBuffer = ENABLE; // Conserva el doble buffer.
    ctrl.dmaCounter = ENABLE; // Mantiene activo el contador.
    ctrl.dmaRequest = ENABLE; // Habilita las requests al DMA.
    DAC_ConfigDAConverterControl(&ctrl);
}

Status config_dma_dac_bloque(const uint32_t *muestras_dacr, size_t cantidad, uint16_t ticks_por_muestra)
{
    if (cantidad == 0u || cantidad > DMA_MAX_TRANSFERENCIAS) {
        return ERROR;
    }
    config_dac_sin_request(ticks_por_muestra);

    GPDMA_Channel_CFG_T cfg;
    cfg.channelNum = GPDMA_CH_1; // Prioridad alta para sostener la salida.
    cfg.transferSize = (uint32_t)cantidad; // Cantidad de muestras.
    cfg.type = GPDMA_M2P; // Memoria a periférico.
    cfg.srcMemAddr = (uint32_t)(uintptr_t)muestras_dacr; // Inicio de la tabla.
    cfg.dstMemAddr = 0u; // El driver obtiene DACR.
    cfg.srcConn = GPDMA_ADC; // Ignorado en M2P.
    cfg.dstConn = GPDMA_DAC; // El timeout del DAC genera requests.
    cfg.src.width = GPDMA_WORD; // Lee una muestra completa.
    cfg.src.burst = GPDMA_BSIZE_1; // Una muestra por request.
    cfg.src.increment = ENABLE; // Recorre la tabla.
    cfg.dst.width = GPDMA_WORD; // Escribe DACR completo.
    cfg.dst.burst = GPDMA_BSIZE_1; // Una escritura por request.
    cfg.dst.increment = DISABLE; // DACR queda fijo.
    cfg.intTC = ENABLE; // Interrumpe al terminar la tabla.
    cfg.intErr = ENABLE; // Interrumpe ante error.
    cfg.linkedList = 0u; // Reproducción única.
    return GPDMA_SetupChannel(&cfg);
}

static GPDMA_LLI_T dac_anillo;

Status config_dma_dac_anillo(const uint32_t *tabla_dacr, size_t cantidad, uint16_t ticks_por_muestra)
{
    if (cantidad == 0u || cantidad > DMA_MAX_TRANSFERENCIAS) {
        return ERROR;
    }
    config_dac_sin_request(ticks_por_muestra);

    const uint32_t control = control_lli_dac(cantidad);
    dac_anillo.srcAddr = (uint32_t)(uintptr_t)tabla_dacr; // Reinicia la tabla.
    dac_anillo.dstAddr = (uint32_t)(uintptr_t)&LPC_DAC->DACR; // Registro de salida fijo.
    dac_anillo.nextLLI = (uint32_t)(uintptr_t)&dac_anillo; // Repite indefinidamente.
    dac_anillo.control = control; // Sin IRQ por vuelta.

    GPDMA_Channel_CFG_T cfg;
    cfg.channelNum = GPDMA_CH_1; // Prioridad alta para sostener la salida.
    cfg.transferSize = (uint32_t)cantidad; // Muestras por vuelta.
    cfg.type = GPDMA_M2P; // Memoria a DAC.
    cfg.srcMemAddr = (uint32_t)(uintptr_t)tabla_dacr; // Primera vuelta desde la tabla.
    cfg.dstMemAddr = 0u; // El driver obtiene DACR.
    cfg.srcConn = GPDMA_ADC; // Ignorado en M2P.
    cfg.dstConn = GPDMA_DAC; // El DAC marca el ritmo.
    cfg.src.width = GPDMA_WORD; // Lee una muestra completa.
    cfg.src.burst = GPDMA_BSIZE_1; // Una muestra por request.
    cfg.src.increment = ENABLE; // Recorre la tabla.
    cfg.dst.width = GPDMA_WORD; // Escribe DACR completo.
    cfg.dst.burst = GPDMA_BSIZE_1; // Una escritura por request.
    cfg.dst.increment = DISABLE; // DACR queda fijo.
    cfg.intTC = DISABLE; // No interrumpe en cada vuelta.
    cfg.intErr = ENABLE; // Interrumpe ante error.
    cfg.linkedList = (uint32_t)(uintptr_t)&dac_anillo; // Cierra el anillo.
    return GPDMA_SetupChannel(&cfg);
}

void iniciar_dma_dac(void)
{
    GPDMA_ChannelStart(GPDMA_CH_1); // Escucha antes de habilitar requests.
    habilitar_request_dac();
}

int main(void)
{
    // Construye una tabla triangular y la adapta al formato del registro DACR.
    for (size_t i = 0u; i < CANTIDAD_MUESTRAS; ++i) {
        const uint32_t valor = (i < CANTIDAD_MUESTRAS / 2u) ? (uint32_t)(i * 64u) : (uint32_t)((CANTIDAD_MUESTRAS - 1u - i) * 64u);
        tabla_dac[i] = DAC_VALUE(valor);
    }

    GPDMA_Init(); // Inicializa el controlador DMA.
    // Configura una LLI circular que repite la tabla cada 32 muestras.
    if (config_dma_dac_anillo(tabla_dac, CANTIDAD_MUESTRAS, 781u) != SUCCESS) {
        while (1) {} // Se detiene si la configuración no es válida.
    }
    iniciar_dma_dac(); // Inicia el canal antes de habilitar requests del DAC.
    while (1) {} // El DMA mantiene la señal sin intervención del CPU.
}
