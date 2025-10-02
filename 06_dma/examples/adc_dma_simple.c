#include "lpc17xx_adc.h"
#include "lpc17xx_pinsel.h"
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


void DMA_IRQHandler (void)
{
	// Verificar interrupción GPDMA en canal 0
	if (GPDMA_IntGetStatus(GPDMA_STAT_INT, 0)){
		// Verificar estado de contador terminal
		if(GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0)){
			// Limpiar interrupción pendiente de contador terminal
			GPDMA_ClearIntPending (GPDMA_STATCLR_INTTC, 0);
			Channel0_TC++;
		}
		// Verificar estado de error terminal
		if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0)){
			// Limpiar interrupción pendiente de error
			GPDMA_ClearIntPending (GPDMA_STATCLR_INTERR, 0);
			Channel0_Err++;
		}
	}
}

/*-------------------------FUNCIÓN PRINCIPAL------------------------------*/

void configADC(){
	PINSEL_CFG_Type PinCfg;
	GPDMA_Channel_CFG_Type GPDMACfg;
	// Configurar P0.25 como AD0.2
	PinCfg.Funcnum = 1;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Pinnum = 25;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);

	// Configuración de ADC:
	// - Canal ADC 2
	// - Tasa de conversión = 200KHz
	ADC_Init(LPC_ADC, 200000);
	ADC_IntConfig(LPC_ADC, ADC_ADINTEN2, SET);
	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_2, SET);

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
		GPDMACfg.ChannelNum = 0;              // Canal 0
		GPDMACfg.SrcMemAddr = 0;              // Memoria fuente - sin usar
		GPDMACfg.DstMemAddr = (uint32_t) &adc_value;  // Memoria destino
		GPDMACfg.TransferSize = DMA_SIZE;     // Tamaño de transferencia
		GPDMACfg.TransferWidth = 0;           // Ancho de transferencia - sin usar
		GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_P2M;  // Tipo: Periférico a Memoria
		GPDMACfg.SrcConn = GPDMA_CONN_ADC;    // Conexión fuente: ADC
		GPDMACfg.DstConn = 0;                 // Conexión destino - sin usar
		GPDMACfg.DMALLI = 0;                  // Lista enlazada - sin usar
		GPDMA_Setup(&GPDMACfg);


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



		uint32_t adc_value, tmp;

		while (1) {
			// Habilitar canal GPDMA 0
			GPDMA_ChannelCmd(0, ENABLE);

			// Iniciar conversión ADC
			ADC_StartCmd(LPC_ADC, ADC_START_NOW);

			// Esperar que se complete el procesamiento GPDMA
			while ((Channel0_TC == 0));

			// Deshabilitar canal GPDMA 0
			GPDMA_ChannelCmd(0, DISABLE);

			// Aquí puedes usar el valor ADC almacenado en adc_value
			// Extraer resultado: ADC_DR_RESULT(adc_value)

			// Esperar un tiempo
			for(tmp = 0; tmp < 1000000; tmp++);

			// Resetear contador terminal
			Channel0_TC = 0;
			// Resetear contador de errores
			Channel0_Err = 0;
		}

		ADC_DeInit(LPC_ADC);
		return 1;
}

