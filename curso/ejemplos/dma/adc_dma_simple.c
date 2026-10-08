#include "lpc17xx_adc.h"
#include "lpc17xx_nvic.h"
#include "lpc17xx_gpdma.h"



/**
 * Habilita el canal DMA
 * Inicia conversión ADC
 * Espera que DMA complete la transferencia
 * El valor está disponible en adc_value
 * Retraso y reinicio del ciclo
 */


// Tamaño de transferencia DMA
#define DMA_SIZE		1

volatile uint32_t Channel0_TC;

// Bandera de error para Canal 0
volatile uint32_t Channel0_Err;

// Destino de la transferencia DMA (el DMA escribe acá; debe ser global/estático)
volatile uint32_t adc_value;

// Configuración del canal GPDMA (se reutiliza en cada vuelta del bucle)
GPDMA_Channel_CFG_T GPDMACfg;


void DMA_IRQHandler (void)
{
	// Verificar interrupción GPDMA en canal 0
	if (GPDMA_IntGetStatus(GPDMA_INT, GPDMA_CH_0)){
		// Verificar estado de contador terminal
		if(GPDMA_IntGetStatus(GPDMA_INTTC, GPDMA_CH_0)){
			// Limpiar interrupción pendiente de contador terminal
			GPDMA_ClearIntPending(GPDMA_CLR_INTTC, GPDMA_CH_0);
			Channel0_TC++;
		}
		// Verificar estado de error terminal
		if (GPDMA_IntGetStatus(GPDMA_INTERR, GPDMA_CH_0)){
			// Limpiar interrupción pendiente de error
			GPDMA_ClearIntPending(GPDMA_CLR_INTERR, GPDMA_CH_0);
			Channel0_Err++;
		}
	}
}

/*-------------------------FUNCIÓN PRINCIPAL------------------------------*/

void configADC(){
	// Configuración de ADC:
	// - Canal ADC 2
	// - Tasa de conversión = 200KHz
	ADC_Init(200000);
	ADC_PinConfig(ADC_CHANNEL_2);
	ADC_IntEnable(ADC_INT_CH2); /* DONE genera la request DMA, no ADC_IRQn. */
	ADC_ChannelEnable(ADC_CHANNEL_2);

}


void configDMA()
{
	// Deshabilitar interrupción GPDMA
		NVIC_DisableIRQ(DMA_IRQn);
		// Prioridad: preemption = 1, sub-priority = 1
		NVIC_SetPriority(DMA_IRQn, ((0x01<<3)|0x01));

		// Inicializar controlador GPDMA
		GPDMA_Init();

		// Configurar canal GPDMA --------------------------------
		GPDMACfg.channelNum = GPDMA_CH_0;
		GPDMACfg.srcMemAddr = 0;                 // Ignorado en P2M
		GPDMACfg.dstMemAddr = (uint32_t)&adc_value;
		GPDMACfg.transferSize = DMA_SIZE;        // Una transferencia de 32 bits
		GPDMACfg.type = GPDMA_P2M;
		GPDMACfg.srcConn = GPDMA_ADC;
		GPDMACfg.dstConn = GPDMA_ADC;            // Ignorado en P2M
		GPDMACfg.src = (GPDMA_Endpoint_T){GPDMA_WORD, GPDMA_BSIZE_1, DISABLE};
		GPDMACfg.dst = (GPDMA_Endpoint_T){GPDMA_WORD, GPDMA_BSIZE_1, ENABLE};
		GPDMACfg.intTC = ENABLE;
		GPDMACfg.intErr = ENABLE;
		GPDMACfg.linkedList = 0;
		GPDMA_SetupChannel(&GPDMACfg);


		// Habilitar interrupción GPDMA
			NVIC_EnableIRQ(DMA_IRQn);



}

int main (void)
{
		configADC();
		 configDMA();

		// Resetear contador terminal
		Channel0_TC = 0;
		// Resetear contador de errores
		Channel0_Err = 0;



		uint32_t tmp;

		while (1) {
			/* Reconfigurar el canal: al terminar cada transferencia el canal se
			 * deshabilita solo y sus registros (direcciones, tamaño) quedaron
			 * avanzados; re-habilitarlo sin reconfigurar tiene efectos
			 * impredecibles (manual, bit E de DMACCxConfig). */
			GPDMA_SetupChannel(&GPDMACfg);

			// Habilitar canal GPDMA 0
			GPDMA_ChannelStart(GPDMA_CH_0);

			// Iniciar conversión ADC
			ADC_StartCmd(ADC_START_NOW);

			// Esperar que se complete el procesamiento GPDMA
			while ((Channel0_TC == 0));

			// Deshabilitar canal GPDMA 0
			GPDMA_ChannelStop(GPDMA_CH_0);

			// Acá podés usar el valor ADC almacenado en adc_value
			// Extraer resultado: ADC_DR_RESULT(adc_value)

			// Esperar un tiempo
			for(tmp = 0; tmp < 1000000; tmp++);

			// Resetear contador terminal
			Channel0_TC = 0;
			// Resetear contador de errores
			Channel0_Err = 0;
		}

		ADC_DeInit();
		return 1;
}
