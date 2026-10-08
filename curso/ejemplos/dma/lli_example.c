/**********************************************************************
* @file		link_list.c
* @brief	Ejemplo de uso de función Link List en GPDMA
* @version	1.0
* @date		16. Junio. 2010
* @author	NXP MCU SW Application Team
**********************************************************************/
#include "lpc17xx_gpdma.h"

/** Tamaño de transferencia DMA */
#define DMA_SIZE		32
// Buffer de destino
uint32_t DMADest_Buffer[DMA_SIZE];

// Primer buffer fuente (16 elementos)
uint32_t DMASrc_Buffer1[DMA_SIZE/2] =
{
	0x01020304,0x05060708,0x090A0B0C,0x0D0E0F10,
	0x11121314,0x15161718,0x191A1B1C,0x1D1E1F20,
	0x21222324,0x25262728,0x292A2B2C,0x2D2E2F30,
	0x31323334,0x35363738,0x393A3B3C,0x3D3E3F40
};

// Segundo buffer fuente (16 elementos)
uint32_t DMASrc_Buffer2[DMA_SIZE/2] =
{
	0x41424344,0x45464748,0x494A4B4C,0x4D4E4F50,
	0x51525354,0x55565758,0x595A5B5C,0x5D5E5F60,
	0x61626364,0x65666768,0x696A6B6C,0x6D6E6F70,
	0x71727374,0x75767778,0x797A7B7C,0x7D7E7F80
};

// Bandera de Terminal Counter para Canal 0
volatile uint32_t Canal0_TC;

// Bandera de Error para Canal 0
volatile uint32_t Canal0_Error;


void DMA_IRQHandler (void)
{
	// Verificar interrupción GPDMA en canal 0
	if (GPDMA_IntGetStatus(GPDMA_INT, GPDMA_CH_7)) {
		// Verificar estado de terminal counter
		if(GPDMA_IntGetStatus(GPDMA_INTTC, GPDMA_CH_7)) {
			// Limpiar interrupción pendiente de terminal counter
			GPDMA_ClearIntPending(GPDMA_CLR_INTTC, GPDMA_CH_7);
			Canal0_TC++;
		}
		// Verificar estado de error
		if (GPDMA_IntGetStatus(GPDMA_INTERR, GPDMA_CH_7)) {
			// Limpiar interrupción pendiente de error
			GPDMA_ClearIntPending(GPDMA_CLR_INTERR, GPDMA_CH_7);
			Canal0_Error++;
		}
	}
}


void verificarBuffer(void)
{
	uint8_t i;
	uint32_t *dir_fuente = (uint32_t *)DMASrc_Buffer1;
	uint32_t *dir_destino = (uint32_t *)DMADest_Buffer;

	// Verificar primer bloque de datos
	for (i = 0; i < DMA_SIZE/2; i++) {
		if (*dir_fuente++ != *dir_destino++) {
			// Error: bucle infinito
			while(1);
		}
	}

	// Verificar segundo bloque de datos
	dir_fuente = (uint32_t *)DMASrc_Buffer2;
	for (i = 0; i < DMA_SIZE/2; i++) {
		if (*dir_fuente++ != *dir_destino++) {
			// Error: bucle infinito
			while(1);
		}
	}
}



int main(void) {
	GPDMA_Channel_CFG_T configGPDMA = {0};
	GPDMA_LLI_T struct_LLI;

	// Deshabilitar interrupción GPDMA
	NVIC_DisableIRQ(DMA_IRQn);
	// Prioridad: preemption = 1, sub-priority = 1
	NVIC_SetPriority(DMA_IRQn, ((0x01<<3)|0x01));

	// Inicializar controlador GPDMA
	GPDMA_Init();

	/* Inicializar lista enlazada GPDMA.
	 * El primer tramo (Buffer1 -> primera mitad del destino) NO necesita descriptor
	 * en RAM: lo describe configGPDMA y GPDMA_SetupChannel lo carga en los registros del
	 * canal. El descriptor de abajo es el SEGUNDO tramo, al que el canal salta
	 * cuando termina el primero. */
	struct_LLI.srcAddr = (uint32_t)&DMASrc_Buffer2;
	struct_LLI.dstAddr = ((uint32_t)&DMADest_Buffer) + (DMA_SIZE/2)*4;
	struct_LLI.nextLLI = 0; // Último elemento
	struct_LLI.control = (DMA_SIZE/2)
								| (2<<18) // Ancho fuente 32 bits
								| (2<<21) // Ancho destino 32 bits
								| (1<<26) // Incremento fuente
								| (1<<27) // Incremento destino
								| (1UL<<31) // IRQ de terminal count al acabar este tramo
								;

	// Configurar canal GPDMA (esto describe el PRIMER tramo)
	configGPDMA.channelNum = GPDMA_CH_7;                 // M2M: prioridad baja
	configGPDMA.srcMemAddr = (uint32_t)DMASrc_Buffer1;
	configGPDMA.dstMemAddr = (uint32_t)DMADest_Buffer;
	configGPDMA.transferSize = DMA_SIZE/2;
	configGPDMA.type = GPDMA_M2M;
	configGPDMA.srcConn = GPDMA_ADC;                     // ignorado
	configGPDMA.dstConn = GPDMA_ADC;                     // ignorado
	configGPDMA.src = (GPDMA_Endpoint_T){GPDMA_WORD, GPDMA_BSIZE_32, ENABLE};
	configGPDMA.dst = (GPDMA_Endpoint_T){GPDMA_WORD, GPDMA_BSIZE_32, ENABLE};
	configGPDMA.intTC = DISABLE;                         // solo interrumpe la LLI final
	configGPDMA.intErr = ENABLE;
	configGPDMA.linkedList = (uint32_t)&struct_LLI;

	// Configurar canal con los parámetros dados
	GPDMA_SetupChannel(&configGPDMA);

	// Resetear contadores
	Canal0_TC = 0;
	Canal0_Error = 0;

	// Habilitar canal 7 de GPDMA
	GPDMA_ChannelStart(GPDMA_CH_7);

	// Habilitar interrupción GPDMA
	NVIC_EnableIRQ(DMA_IRQn);

	/* Solo el descriptor final tiene I=1. */
	while ((Canal0_TC == 0) && (Canal0_Error == 0));

	// Verificar buffer
	verificarBuffer();

	// Bucle infinito
	while(1)
	{

	}
	return 1;
}
