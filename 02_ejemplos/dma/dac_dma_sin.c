/**********************************************************************
* @brief	Este ejemplo describe cómo usar el DAC para generar una onda
* 			senoidal usando DMA para transferir datos
**********************************************************************/
#include "lpc17xx_dac.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpdma.h"

/** Tamaño de transferencia DMA */
#define DMA_SIZE		60
#define NUM_MUESTRAS_SENO	60
#define FREQ_SENO_EN_HZ	60
#define PCLK_DAC_EN_MHZ	25 // CCLK dividido por 4



configDAC()
{
	PINSEL_CFG_Type configPin;
		/*
		 * Inicializar conexión del pin DAC
		 * AOUT en P0.26
		 */
		configPin.Funcnum = 2;
		configPin.OpenDrain = 0;
		configPin.Pinmode = 0;
		configPin.Pinnum = 26;
		configPin.Portnum = 0;
		PINSEL_configPin(&configPin);


		DAC_CONVERTER_CFG_Type configDAC;
		// Configurar el convertidor DAC
			configDAC.CNT_ENA = SET;
			configDAC.DMA_ENA = SET;
			DAC_Init(LPC_DAC);

			// Establecer tiempo de espera para el DAC
			tmp = (PCLK_DAC_EN_MHZ * 1000000) / (FREQ_SENO_EN_HZ * NUM_MUESTRAS_SENO);
			DAC_SetDMATimeOut(LPC_DAC, tmp);
			DAC_ConfigDAConverterControl(LPC_DAC, &configDAC);
}
GPDMA_Channel_CFG_Type configDMA;
void configDMA(){

	GPDMA_LLI_Type structLLI;

	uint32_t tmp;
	uint32_t i;

	// Tabla de valores de seno de 0 a 90 grados (16 muestras)
	uint32_t seno_0_a_90_16_muestras[16] = {
		0, 1045, 2079, 3090, 4067,
		5000, 5877, 6691, 7431, 8090,
		8660, 9135, 9510, 9781, 9945, 10000
	};

	uint32_t tabla_seno_dac[NUM_MUESTRAS_SENO];



	// Preparar tabla de búsqueda de onda senoidal para el DAC
		for(i = 0; i < NUM_MUESTRAS_SENO; i++)
		{
			if(i <= 15)
			{
				// Primer cuadrante (0° a 90°)
				tabla_seno_dac[i] = 512 + 512 * seno_0_a_90_16_muestras[i] / 10000;
				if(i == 15) tabla_seno_dac[i] = 1023;
			}
			else if(i <= 30)
			{
				// Segundo cuadrante (90° a 180°)
				tabla_seno_dac[i] = 512 + 512 * seno_0_a_90_16_muestras[30 - i] / 10000;
			}
			else if(i <= 45)
			{
				// Tercer cuadrante (180° a 270°)
				tabla_seno_dac[i] = 512 - 512 * seno_0_a_90_16_muestras[i - 30] / 10000;
			}
			else
			{
				// Cuarto cuadrante (270° a 360°)
				tabla_seno_dac[i] = 512 - 512 * seno_0_a_90_16_muestras[60 - i] / 10000;
			}
			// Desplazar 6 bits a la izquierda (formato del registro DACR)
			tabla_seno_dac[i] = (tabla_seno_dac[i] << 6);
		}

		// Preparar estructura de lista enlazada DMA
		structLLI.SrcAddr = (uint32_t)tabla_seno_dac;
		structLLI.DstAddr = (uint32_t)&(LPC_DAC->DACR);
		structLLI.NextLLI = (uint32_t)&structLLI;
		structLLI.Control = DMA_SIZE
								| (2 << 18) // ancho de origen 32 bits
								| (2 << 21) // ancho de destino 32 bits
								| (1 << 26) // incremento de origen
								;

		/* Sección de bloque GPDMA -------------------------------------------- */
		// Inicializar controlador GPDMA
		GPDMA_Init();

		// Configurar canal GPDMA - canal 0
		configDMA.ChannelNum = 0;
		// Dirección de memoria origen
		configDMA.SrcMemAddr = (uint32_t)(tabla_seno_dac);
		// Dirección de memoria destino - no usada
		configDMA.DstMemAddr = 0;
		// Tamaño de transferencia
		configDMA.TransferSize = DMA_SIZE;
		// Ancho de transferencia - no usado
		configDMA.TransferWidth = 0;
		// Tipo de transferencia: memoria a periférico
		configDMA.TransferType = GPDMA_TRANSFERTYPE_M2P;
		// Conexión origen - no usada
		configDMA.SrcConn = 0;
		// Conexión destino: DAC
		configDMA.DstConn = GPDMA_CONN_DAC;
		// Lista enlazada
		configDMA.DMALLI = (uint32_t)&structLLI;
		// Configurar canal con los parámetros dados
		GPDMA_Setup(&configDMA);


		// Habilitar canal GPDMA 0
		GPDMA_ChannelCmd(0, ENABLE);
}

int main()
{

	configDAC();
	configDMA();


		// Bucle infinito
		while(1);

		return 1;
	}


