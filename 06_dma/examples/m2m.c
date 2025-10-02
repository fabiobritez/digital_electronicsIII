/*
 * Transfiere 256 bytes (0x100) de datos de una ubicación de memoria RAM a otra
 * Usa el módulo GPDMA (General Purpose DMA) en modo Memoria a Memoria
 * Utiliza interrupciones para detectar cuando la transferencia termina
 * Verifica que los datos se copiaron correctamente comparando origen y destino
 * Si hay error en la verificación, entra en un bucle infinito

*/

#include "lpc17xx_gpdma.h"

/* Tamaño de transferencia DMA */
#define DMA_SIZE		0x100UL

/* Dirección de origen DMA GPIO*/
#define DMA_SRC			LPC_GPIO0_BASE

/* Dirección de destino DMA */
#define DMA_DST			(DMA_SRC+DMA_SIZE)


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
	if (GPDMA_IntGetStatus(GPDMA_STAT_INT, 0)) {
		/* Verificar estado de terminal counter */
		if(GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0)) {
			/* Limpiar interrupción pendiente de terminate counter */
			GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, 0);
			Canal0_TC++;
		}
		/* Verificar estado de error */
		if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0)) {
			/* Limpiar interrupción pendiente de error counter */
			GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, 0);
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
	GPDMA_Channel_CFG_Type ConfigGPDMA;

	/* Inicializar buffer */
	inicializarBuffer();

	/* Deshabilitar interrupción GPDMA */
	NVIC_DisableIRQ(DMA_IRQn);

	/* Configurar prioridad: preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(DMA_IRQn, ((0x01<<3)|0x01));

	/* Inicializar controlador GPDMA */
	GPDMA_Init();

	/* Configurar canal GPDMA -------------------------------- */
	/* Canal 0 */
	ConfigGPDMA.ChannelNum = 0;
	/* Dirección de memoria origen */
	ConfigGPDMA.SrcMemAddr = DMA_SRC;
	/* Dirección de memoria destino */
	ConfigGPDMA.DstMemAddr = DMA_DST;
	/* Tamaño de transferencia */
	ConfigGPDMA.TransferSize = DMA_SIZE;
	/* Ancho de transferencia: palabra (32 bits) */
	ConfigGPDMA.TransferWidth = GPDMA_WIDTH_WORD;
	/* Tipo de transferencia: Memoria a Memoria */
	ConfigGPDMA.TransferType = GPDMA_TRANSFERTYPE_M2M;
	/* Conexión de origen - no usado en M2M */
	ConfigGPDMA.SrcConn = 0;
	/* Conexión de destino - no usado en M2M */
	ConfigGPDMA.DstConn = 0;
	/* Lista enlazada - no usada */
	ConfigGPDMA.DMALLI = 0;

	/* Configurar canal con los parámetros dados */
	GPDMA_Setup(&ConfigGPDMA);

	/* Resetear contador terminal */
	Canal0_TC = 0;
	/* Resetear contador de errores */
	Canal0_Err = 0;

	/* Habilitar canal GPDMA 0 */
	GPDMA_ChannelCmd(0, ENABLE);

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

