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
	if (GPDMA_IntGetStatus(GPDMA_STAT_INT, 0)) {
		// Verificar estado de terminal counter
		if(GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0)) {
			// Limpiar interrupción pendiente de terminal counter
			GPDMA_ClearIntPending (GPDMA_STATCLR_INTTC, 0);
			Canal0_TC++;
		}
		// Verificar estado de error
		if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0)) {
			// Limpiar interrupción pendiente de error
			GPDMA_ClearIntPending (GPDMA_STATCLR_INTERR, 0);
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
	GPDMA_Channel_CFG_Type configGPDMA;
	GPDMA_LLI_Type struct_LLI[2];

	// Deshabilitar interrupción GPDMA
	NVIC_DisableIRQ(DMA_IRQn);
	// Prioridad: preemption = 1, sub-priority = 1
	NVIC_SetPriority(DMA_IRQn, ((0x01<<3)|0x01));

	// Inicializar controlador GPDMA
	GPDMA_Init();

	/* Inicializar lista enlazada GPDMA */
	// Primera estructura de lista enlazada
	struct_LLI[0].SrcAddr = (uint32_t)&DMASrc_Buffer1;
	struct_LLI[0].DstAddr = (uint32_t)&DMADest_Buffer;
	struct_LLI[0].NextLLI = (uint32_t)&struct_LLI[1];
	struct_LLI[0].Control = (DMA_SIZE/2)
								| (2<<18) // Ancho fuente 32 bits
								| (2<<21) // Ancho destino 32 bits
								| (1<<26) // Incremento fuente
								| (1<<27) // Incremento destino
								;

	// Segunda estructura de lista enlazada
	struct_LLI[1].SrcAddr = (uint32_t)&DMASrc_Buffer2;
	struct_LLI[1].DstAddr = ((uint32_t)&DMADest_Buffer) + (DMA_SIZE/2)*4;
	struct_LLI[1].NextLLI = 0; // Último elemento
	struct_LLI[1].Control = (DMA_SIZE/2)
								| (2<<18) // Ancho fuente 32 bits
								| (2<<21) // Ancho destino 32 bits
								| (1<<26) // Incremento fuente
								| (1<<27) // Incremento destino
								;

	// Configurar canal GPDMA
	configGPDMA.ChannelNum = 0;                          // Canal 0
	configGPDMA.SrcMemAddr = (uint32_t)DMASrc_Buffer1;   // Dirección memoria fuente
	configGPDMA.DstMemAddr = (uint32_t)DMADest_Buffer;   // Dirección memoria destino
	configGPDMA.TransferSize = DMA_SIZE;                 // Tamaño de transferencia
	configGPDMA.TransferWidth = GPDMA_WIDTH_WORD;        // Ancho de transferencia
	configGPDMA.TransferType = GPDMA_TRANSFERTYPE_M2M;   // Tipo: Memoria a Memoria
	configGPDMA.SrcConn = 0;                             // Conexión fuente - no usada
	configGPDMA.DstConn = 0;                             // Conexión destino - no usada
	configGPDMA.DMALLI = (uint32_t)&struct_LLI[0]; // Lista enlazada

	// Configurar canal con los parámetros dados
	GPDMA_Setup(&configGPDMA);

	// Resetear contadores
	Canal0_TC = 0;
	Canal0_Error = 0;

	// Habilitar canal 0 de GPDMA
	GPDMA_ChannelCmd(0, ENABLE);

	// Habilitar interrupción GPDMA
	NVIC_EnableIRQ(DMA_IRQn);

	// Esperar a que complete el procesamiento GPDMA
	while ((Canal0_TC == 0) && (Canal0_Error == 0));

	// Verificar buffer
	verificarBuffer();

	// Bucle infinito
	while(1)
	{

	}
	return 1;
}

