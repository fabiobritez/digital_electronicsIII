/*
 * Transfiere 256 bytes (0x100) de datos de una ubicación de memoria RAM a otra
 * Usa el módulo GPDMA (General Purpose DMA) en modo Memoria a Memoria
 * Utiliza interrupciones para detectar cuando la transferencia termina
 * Verifica que los datos se copiaron correctamente comparando origen y destino
 * Si hay error en la verificación, entra en un bucle infinito

*/

#include "lpc17xx_gpdma.h"

/* Tamaño de transferencia DMA, en bytes */
#define DMA_SIZE		0x100UL

/* Dirección de origen: RAM AHB banco 0 (16 kB en 0x2007 C000, libre si no se usa Ethernet/USB) */
#define DMA_SRC			0x2007C000UL

/* Dirección de destino: RAM AHB banco 1 (16 kB en 0x2008 0000) */
#define DMA_DST			0x20080000UL


/************************** VARIABLES PRIVADAS *************************/
/* Bandera de Terminal Counter para Canal 0 */
volatile uint32_t Canal0_TC;

/* Bandera de Error Counter para Canal 0 */
volatile uint32_t Canal0_Err;


/************************** FUNCIONES PRIVADAS *************************/
void inicializarBuffer(void);
void verificarBuffer(void);


void DMA_IRQHandler(void)
{
	/* Verificar interrupción GPDMA en canal 0 */
	if (GPDMA_IntGetStatus(GPDMA_INT, GPDMA_CH_7)) {
		/* Verificar estado de terminal counter */
		if(GPDMA_IntGetStatus(GPDMA_INTTC, GPDMA_CH_7)) {
			/* Limpiar interrupción pendiente de terminate counter */
			GPDMA_ClearIntPending(GPDMA_CLR_INTTC, GPDMA_CH_7);
			Canal0_TC++;
		}
		/* Verificar estado de error */
		if (GPDMA_IntGetStatus(GPDMA_INTERR, GPDMA_CH_7)) {
			/* Limpiar interrupción pendiente de error counter */
			GPDMA_ClearIntPending(GPDMA_CLR_INTERR, GPDMA_CH_7);
			Canal0_Err++;
		}
	}
}


void inicializarBuffer(void)
{
	uint8_t i;
	uint32_t *dir_origen = (uint32_t *)DMA_SRC;
	uint32_t *dir_destino = (uint32_t *)DMA_DST;

	for (i = 0; i < DMA_SIZE/4; i++) {
		*dir_origen++ = i;
		*dir_destino++ = 0;
	}
}


void verificarBuffer(void)
{
	uint8_t i;
	uint32_t *dir_origen = (uint32_t *)DMA_SRC;
	uint32_t *dir_destino = (uint32_t *)DMA_DST;

	for (i = 0; i < DMA_SIZE/4; i++) {
		if (*dir_origen++ != *dir_destino++) {
		  /* Llamar bucle de error */
		  while(1){};
    }
	}
}




int main(void)
{
	GPDMA_Channel_CFG_T ConfigGPDMA = {0};

	/* Inicializar buffer */
	inicializarBuffer();

	/* Deshabilitar interrupción GPDMA */
	NVIC_DisableIRQ(DMA_IRQn);

	/* Configurar prioridad: preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(DMA_IRQn, ((0x01<<3)|0x01));

	/* Inicializar controlador GPDMA */
	GPDMA_Init();

	/* Configurar canal GPDMA -------------------------------- */
	/* Canal 7: el manual recomienda prioridad baja para M2M. */
	ConfigGPDMA.channelNum = GPDMA_CH_7;
	/* Dirección de memoria origen */
	ConfigGPDMA.srcMemAddr = DMA_SRC;
	/* Dirección de memoria destino */
	ConfigGPDMA.dstMemAddr = DMA_DST;
	/* Tamaño de transferencia: en ELEMENTOS (words de 4 bytes), no en bytes */
	ConfigGPDMA.transferSize = DMA_SIZE/4;
	/* Tipo de transferencia: Memoria a Memoria */
	ConfigGPDMA.type = GPDMA_M2M;
	/* Conexión de origen - no usado en M2M */
	ConfigGPDMA.srcConn = GPDMA_ADC;
	/* Conexión de destino - no usado en M2M */
	ConfigGPDMA.dstConn = GPDMA_ADC;
	/* Lista enlazada - no usada */
	ConfigGPDMA.src = (GPDMA_Endpoint_T){GPDMA_WORD, GPDMA_BSIZE_32, ENABLE};
	ConfigGPDMA.dst = (GPDMA_Endpoint_T){GPDMA_WORD, GPDMA_BSIZE_32, ENABLE};
	ConfigGPDMA.intTC = ENABLE;
	ConfigGPDMA.intErr = ENABLE;
	ConfigGPDMA.linkedList = 0;

	/* Configurar canal con los parámetros dados */
	GPDMA_SetupChannel(&ConfigGPDMA);

	/* Resetear contador terminal */
	Canal0_TC = 0;
	/* Resetear contador de errores */
	Canal0_Err = 0;

	/* Habilitar canal GPDMA 7 */
	GPDMA_ChannelStart(GPDMA_CH_7);

	/* Habilitar interrupción GPDMA */
	NVIC_EnableIRQ(DMA_IRQn);

	/* Esperar a que el procesamiento GPDMA se complete */
	while ((Canal0_TC == 0) && (Canal0_Err == 0));

	/* Verificar buffer */
	verificarBuffer();

	/* Si llega aquí, la transferencia fue exitosa */
 
	while(1){};

	return 1;
}
