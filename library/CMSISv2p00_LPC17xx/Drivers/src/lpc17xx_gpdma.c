/**
 * @file    lpc17xx_gpdma.c
 * @brief   Contains all macro definitions and function prototypes
 *          support for GPDMA firmware library on LPC17xx
 * @version 2.0
 * @date    21. May. 2010
 * @author  NXP MCU SW Application Team
 *
 * Software that is described herein is for illustrative purposes only which provides customers with
 * programming information regarding the products. This software is supplied "AS IS" without any
 * warranties. NXP Semiconductors assumes no responsibility or liability for the use of the
 * software, conveys no license or title under any patent, copyright, or mask work right to the
 * product. NXP Semiconductors reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or warranty that such application
 * will be suitable for the specified use without further testing or modification.
 *
 * @par Refactor:
 * Date: 28/02/2026, Author: David Trujillo Medina
 */

/* ---------------------------- Peripheral group ---------------------------- */
/** @addtogroup GPDMA
 * @{
 */

/* -------------------------------- Includes -------------------------------- */
#include "lpc17xx_gpdma.h"
#include "lpc17xx_clkpwr.h"

/* If this source file built with example, the LPC17xx FW library configuration
 * file in each example directory ("lpc17xx_libcfg.h") must be included,
 * otherwise the default FW library configuration file must be included instead
 */
#ifdef __BUILD_WITH_EXAMPLE__
#include "lpc17xx_libcfg.h"
#else
#include "lpc17xx_libcfg_default.h"
#endif /* __BUILD_WITH_EXAMPLE__ */

#ifdef _GPDMA

/* --------------------------- Private Variables ---------------------------- */
/** @defgroup GPDMA_Private_Variables GPDMA Private Variables
 * @{
 */

/**
 * @brief Lookup Table of Connection Type matched with Peripheral Data (FIFO) register base address
 */
#ifdef __IAR_SYSTEMS_ICC__
volatile const void* GPDMA_LUTPerAddr[] = {
    (&LPC_SSP0->DR),                 // SSP0 Tx
    (&LPC_SSP0->DR),                 // SSP0 Rx
    (&LPC_SSP1->DR),                 // SSP1 Tx
    (&LPC_SSP1->DR),                 // SSP1 Rx
    (&LPC_ADC->ADGDR),               // ADC
    (&LPC_I2S->I2STXFIFO),           // I2S Tx
    (&LPC_I2S->I2SRXFIFO),           // I2S Rx
    (&LPC_DAC->DACR),                // DAC
    (&LPC_UART0->/*RBTHDLR.*/ THR),  // UART0 Tx
    (&LPC_UART0->/*RBTHDLR.*/ RBR),  // UART0 Rx
    (&LPC_UART1->/*RBTHDLR.*/ THR),  // UART1 Tx
    (&LPC_UART1->/*RBTHDLR.*/ RBR),  // UART1 Rx
    (&LPC_UART2->/*RBTHDLR.*/ THR),  // UART2 Tx
    (&LPC_UART2->/*RBTHDLR.*/ RBR),  // UART2 Rx
    (&LPC_UART3->/*RBTHDLR.*/ THR),  // UART3 Tx
    (&LPC_UART3->/*RBTHDLR.*/ RBR),  // UART3 Rx
    (&LPC_TIM0->MR0),                // MAT0.0
    (&LPC_TIM0->MR1),                // MAT0.1
    (&LPC_TIM1->MR0),                // MAT1.0
    (&LPC_TIM1->MR1),                // MAT1.1
    (&LPC_TIM2->MR0),                // MAT2.0
    (&LPC_TIM2->MR1),                // MAT2.1
    (&LPC_TIM3->MR0),                // MAT3.0
    (&LPC_TIM3->MR1)                 // MAT3.1
};
#else
const uint32_t GPDMA_LUTPerAddr[] = {
    ((uint32_t)&LPC_SSP0->DR),        // SSP0 Tx
    ((uint32_t)&LPC_SSP0->DR),        // SSP0 Rx
    ((uint32_t)&LPC_SSP1->DR),        // SSP1 Tx
    ((uint32_t)&LPC_SSP1->DR),        // SSP1 Rx
    ((uint32_t)&LPC_ADC->ADGDR),      // ADC
    ((uint32_t)&LPC_I2S->I2STXFIFO),  // I2S Tx
    ((uint32_t)&LPC_I2S->I2SRXFIFO),  // I2S Rx
    ((uint32_t)&LPC_DAC->DACR),       // DAC
    ((uint32_t)&LPC_UART0->THR),      // UART0 Tx
    ((uint32_t)&LPC_UART0->RBR),      // UART0 Rx
    ((uint32_t)&LPC_UART1->THR),      // UART1 Tx
    ((uint32_t)&LPC_UART1->RBR),      // UART1 Rx
    ((uint32_t)&LPC_UART2->THR),      // UART2 Tx
    ((uint32_t)&LPC_UART2->RBR),      // UART2 Rx
    ((uint32_t)&LPC_UART3->THR),      // UART3 Tx
    ((uint32_t)&LPC_UART3->RBR),      // UART3 Rx
    ((uint32_t)&LPC_TIM0->MR0),       // MAT0.0
    ((uint32_t)&LPC_TIM0->MR1),       // MAT0.1
    ((uint32_t)&LPC_TIM1->MR0),       // MAT1.0
    ((uint32_t)&LPC_TIM1->MR1),       // MAT1.1
    ((uint32_t)&LPC_TIM2->MR0),       // MAT2.0
    ((uint32_t)&LPC_TIM2->MR1),       // MAT2.1
    ((uint32_t)&LPC_TIM3->MR0),       // MAT3.0
    ((uint32_t)&LPC_TIM3->MR1)        // MAT3.1
};
#endif
/**
 * @brief Lookup Table of GPDMA Channel Number matched with GPDMA channel pointer
 */
LPC_GPDMACH_TypeDef* pGPDMACh[8] = {
    LPC_GPDMACH0,  // GPDMA Channel 0
    LPC_GPDMACH1,  // GPDMA Channel 1
    LPC_GPDMACH2,  // GPDMA Channel 2
    LPC_GPDMACH3,  // GPDMA Channel 3
    LPC_GPDMACH4,  // GPDMA Channel 4
    LPC_GPDMACH5,  // GPDMA Channel 5
    LPC_GPDMACH6,  // GPDMA Channel 6
    LPC_GPDMACH7   // GPDMA Channel 7
};

/**
 * @brief Optimized Peripheral Source and Destination burst size
 */
const uint8_t GPDMA_LUTPerBurst[] = {
    GPDMA_BSIZE_4,   // SSP0 Tx
    GPDMA_BSIZE_4,   // SSP0 Rx
    GPDMA_BSIZE_4,   // SSP1 Tx
    GPDMA_BSIZE_4,   // SSP1 Rx
    GPDMA_BSIZE_4,   // ADC
    GPDMA_BSIZE_32,  // I2S channel 0
    GPDMA_BSIZE_32,  // I2S channel 1
    GPDMA_BSIZE_1,   // DAC
    GPDMA_BSIZE_1,   // UART0 Tx
    GPDMA_BSIZE_1,   // UART0 Rx
    GPDMA_BSIZE_1,   // UART1 Tx
    GPDMA_BSIZE_1,   // UART1 Rx
    GPDMA_BSIZE_1,   // UART2 Tx
    GPDMA_BSIZE_1,   // UART2 Rx
    GPDMA_BSIZE_1,   // UART3 Tx
    GPDMA_BSIZE_1,   // UART3 Rx
    GPDMA_BSIZE_1,   // MAT0.0
    GPDMA_BSIZE_1,   // MAT0.1
    GPDMA_BSIZE_1,   // MAT1.0
    GPDMA_BSIZE_1,   // MAT1.1
    GPDMA_BSIZE_1,   // MAT2.0
    GPDMA_BSIZE_1,   // MAT2.1
    GPDMA_BSIZE_1,   // MAT3.0
    GPDMA_BSIZE_1    // MAT3.1
};

/**
 * @brief Optimized Peripheral Source and Destination transfer width
 */
const uint8_t GPDMA_LUTPerWid[] = {
    GPDMA_BYTE,  // SSP0 Tx
    GPDMA_BYTE,  // SSP0 Rx
    GPDMA_BYTE,  // SSP1 Tx
    GPDMA_BYTE,  // SSP1 Rx
    GPDMA_WORD,  // ADC
    GPDMA_WORD,  // I2S channel 0
    GPDMA_WORD,  // I2S channel 1
    GPDMA_WORD,  // DAC: DACR usa VALUE en bits 15:6; el acceso debe ser de 32 bits
    GPDMA_BYTE,  // UART0 Tx
    GPDMA_BYTE,  // UART0 Rx
    GPDMA_BYTE,  // UART1 Tx
    GPDMA_BYTE,  // UART1 Rx
    GPDMA_BYTE,  // UART2 Tx
    GPDMA_BYTE,  // UART2 Rx
    GPDMA_BYTE,  // UART3 Tx
    GPDMA_BYTE,  // UART3 Rx
    GPDMA_WORD,  // MAT0.0
    GPDMA_WORD,  // MAT0.1
    GPDMA_WORD,  // MAT1.0
    GPDMA_WORD,  // MAT1.1
    GPDMA_WORD,  // MAT2.0
    GPDMA_WORD,  // MAT2.1
    GPDMA_WORD,  // MAT3.0
    GPDMA_WORD   // MAT3.1
};

/**
 * @}
 */

/* ---------------------- Private Function Prototypes ----------------------- */
/**
 * @brief Validates if the specified DMA channel is available (not in use).
 * @param[in] channelNum DMA channel to validate.
 * @return SUCCESS if available, ERROR if busy.
 */
static Status GPDMA_ValidateChannel(GPDMA_CH channelNum);

/**
 * @brief Resets the specified DMA channel control and configuration registers,
 * and clears interrupt flags.
 * @param[in] ch Pointer to the DMA channel registers.
 * @param[in] channelNum DMA channel number.
 */
static void GPDMA_ResetChannel(LPC_GPDMACH_TypeDef* ch, GPDMA_CH channelNum);

/**
 * @brief Configures DMA channel registers according to the configuration structure.
 * @param[in] ch Pointer to the DMA channel registers.
 * @param[in] cfg Pointer to the DMA channel configuration structure.
 */
static void GPDMA_ConfigChannelRegisters(LPC_GPDMACH_TypeDef* ch, const GPDMA_Channel_CFG_T* cfg);

/**
 * @brief Configures the DMAREQSEL register for peripheral connections.
 * @param[in] cfg Pointer to the DMA channel configuration structure.
 */
static void GPDMA_ConfigDMAReqSel(const GPDMA_Channel_CFG_T* cfg);
/* ------------------- End of Private Function Prototypes ------------------- */

/* --------------------------- Private Functions ---------------------------- */
static Status GPDMA_ValidateChannel(GPDMA_CH channelNum) {
    if (LPC_GPDMA->DMACEnbldChns & GPDMA_ChannelBit(channelNum)) {
        return ERROR;
    }
    return SUCCESS;
}

static void GPDMA_ResetChannel(LPC_GPDMACH_TypeDef* ch, GPDMA_CH channelNum) {
    LPC_GPDMA->DMACIntTCClear = GPDMA_ChannelBit(channelNum);
    LPC_GPDMA->DMACIntErrClr  = GPDMA_ChannelBit(channelNum);
    ch->DMACCControl          = 0;
    ch->DMACCConfig           = 0;
}

static void GPDMA_ConfigChannelRegisters(LPC_GPDMACH_TypeDef* ch, const GPDMA_Channel_CFG_T* cfg) {
    uint32_t ctrl = GPDMA_DMACCxControl_TransferSize(cfg->transferSize);

    uint32_t srcAddr = cfg->srcMemAddr;
    uint32_t dstAddr = cfg->dstMemAddr;
    if (cfg->type == GPDMA_P2M || cfg->type == GPDMA_P2P) {
        srcAddr = GPDMA_LUTPerAddr[cfg->srcConn];
    }
    if (cfg->type == GPDMA_M2P || cfg->type == GPDMA_P2P) {
        dstAddr = GPDMA_LUTPerAddr[cfg->dstConn];
    }
    ch->DMACCSrcAddr  = srcAddr;
    ch->DMACCDestAddr = dstAddr;

    const uint32_t autoSrcWidth =
        (cfg->type == GPDMA_P2M || cfg->type == GPDMA_P2P) ? GPDMA_LUTPerWid[cfg->srcConn]
        : (cfg->type == GPDMA_M2P)                         ? GPDMA_LUTPerWid[cfg->dstConn]
                                                          : GPDMA_WORD;
    const uint32_t autoDstWidth =
        (cfg->type == GPDMA_M2P || cfg->type == GPDMA_P2P) ? GPDMA_LUTPerWid[cfg->dstConn]
        : (cfg->type == GPDMA_P2M)                         ? GPDMA_LUTPerWid[cfg->srcConn]
                                                          : GPDMA_WORD;
    const uint32_t sWidth =
        (cfg->src.width != GPDMA_WIDTH_AUTO) ? cfg->src.width : autoSrcWidth;
    const uint32_t dWidth =
        (cfg->dst.width != GPDMA_WIDTH_AUTO) ? cfg->dst.width : autoDstWidth;
    ctrl |= GPDMA_DMACCxControl_SWidth(sWidth) | GPDMA_DMACCxControl_DWidth(dWidth);

    const uint32_t autoSrcBurst =
        (cfg->type == GPDMA_P2M || cfg->type == GPDMA_P2P) ? GPDMA_LUTPerBurst[cfg->srcConn]
        : (cfg->type == GPDMA_M2P)                         ? GPDMA_LUTPerBurst[cfg->dstConn]
                                                          : GPDMA_BSIZE_32;
    const uint32_t autoDstBurst =
        (cfg->type == GPDMA_M2P || cfg->type == GPDMA_P2P) ? GPDMA_LUTPerBurst[cfg->dstConn]
        : (cfg->type == GPDMA_P2M)                         ? GPDMA_LUTPerBurst[cfg->srcConn]
                                                          : GPDMA_BSIZE_32;
    const uint32_t sBurst =
        (cfg->src.burst != GPDMA_BURST_AUTO) ? cfg->src.burst : autoSrcBurst;
    const uint32_t dBurst =
        (cfg->dst.burst != GPDMA_BURST_AUTO) ? cfg->dst.burst : autoDstBurst;
    ctrl |= GPDMA_DMACCxControl_SBSize(sBurst) | GPDMA_DMACCxControl_DBSize(dBurst);

    if (cfg->src.increment == ENABLE) {
        ctrl |= GPDMA_DMACCxControl_SI;
    }
    if (cfg->dst.increment == ENABLE) {
        ctrl |= GPDMA_DMACCxControl_DI;
    }

    ch->DMACCControl = ctrl | (cfg->intTC ? GPDMA_DMACCxControl_I : 0);
}

static void GPDMA_ConfigConnectionRequest(GPDMA_CONNECTION connection) {
    if (connection >= GPDMA_MAT0_0) {
        LPC_SC->DMAREQSEL |= (1UL << (connection - GPDMA_MAT0_0));
    } else if (connection >= GPDMA_UART0_Tx) {
        LPC_SC->DMAREQSEL &= ~(1UL << (connection - GPDMA_UART0_Tx));
    }
}

static void GPDMA_ConfigDMAReqSel(const GPDMA_Channel_CFG_T* cfg) {
    if (cfg->type == GPDMA_P2M || cfg->type == GPDMA_P2P) {
        GPDMA_ConfigConnectionRequest(cfg->srcConn);
    }
    if (cfg->type == GPDMA_M2P || cfg->type == GPDMA_P2P) {
        GPDMA_ConfigConnectionRequest(cfg->dstConn);
    }
}
/* ------------------------ End of Private Functions ------------------------ */

/* ---------------------------- Public Functions ---------------------------- */
/** @addtogroup GPDMA_Public_Functions
 * @{
 */

void GPDMA_Init(void) {
    CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCGPDMA, ENABLE);

    for (int i = 0; i < GPDMA_NUM_CHANNELS; i++) {
        pGPDMACh[i]->DMACCConfig  = 0;
        pGPDMACh[i]->DMACCControl = 0;
    }

    LPC_GPDMA->DMACIntTCClear = GPDMA_DMACIntStat_ALL;
    LPC_GPDMA->DMACIntErrClr  = GPDMA_DMACIntStat_ALL;

    LPC_GPDMA->DMACConfig = GPDMA_DMACConfig_E;
    while (!(LPC_GPDMA->DMACConfig & GPDMA_DMACConfig_E)) {}
}

void GPDMA_DeInit(void) {
    LPC_GPDMA->DMACConfig &= ~GPDMA_DMACConfig_E;
    while (LPC_GPDMA->DMACConfig & GPDMA_DMACConfig_E) {}

    CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCGPDMA, DISABLE);
}

Status GPDMA_SetupChannel(const GPDMA_Channel_CFG_T* dmaCfg) {
    if (dmaCfg == NULL) {
        return ERROR;
    }

    CHECK_PARAM(PARAM_GPDMA_CHANNEL(dmaCfg->channelNum));
    CHECK_PARAM(dmaCfg->transferSize > 0 && dmaCfg->transferSize <= 4095);
    CHECK_PARAM(PARAM_GPDMA_TRANSFER_WIDTH(dmaCfg->src.width));
    CHECK_PARAM(PARAM_GPDMA_TRANSFER_WIDTH(dmaCfg->dst.width));
    CHECK_PARAM(PARAM_GPDMA_BURST_SIZE(dmaCfg->src.burst));
    CHECK_PARAM(PARAM_GPDMA_BURST_SIZE(dmaCfg->dst.burst));
    CHECK_PARAM(PARAM_GPDMA_TRANSFER_TYPE(dmaCfg->type));
    CHECK_PARAM(PARAM_GPDMA_CONNECTION(dmaCfg->srcConn));
    CHECK_PARAM(PARAM_GPDMA_CONNECTION(dmaCfg->dstConn));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(dmaCfg->intTC));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(dmaCfg->intErr));

    if (GPDMA_ValidateChannel(dmaCfg->channelNum) == ERROR) {
        return ERROR;
    }

    LPC_GPDMACH_TypeDef* pDMAch = pGPDMACh[dmaCfg->channelNum];

    GPDMA_ResetChannel(pDMAch, dmaCfg->channelNum);

    pDMAch->DMACCLLI = dmaCfg->linkedList;

    GPDMA_ConfigChannelRegisters(pDMAch, dmaCfg);

    GPDMA_ConfigDMAReqSel(dmaCfg);

    const uint32_t src = (dmaCfg->srcConn > 15) ? (dmaCfg->srcConn - 8) : dmaCfg->srcConn;
    const uint32_t dst = (dmaCfg->dstConn > 15) ? (dmaCfg->dstConn - 8) : dmaCfg->dstConn;

    pDMAch->DMACCConfig = (dmaCfg->intTC ? GPDMA_DMACCxConfig_ITC : 0) |
                          (dmaCfg->intErr ? GPDMA_DMACCxConfig_IE : 0) |
                          GPDMA_DMACCxConfig_TransferType(dmaCfg->type) |
                          GPDMA_DMACCxConfig_SrcPeripheral(src) |
                          GPDMA_DMACCxConfig_DestPeripheral(dst);

    return SUCCESS;
}

void GPDMA_ChannelStart(GPDMA_CH channel) {
    CHECK_PARAM(PARAM_GPDMA_CHANNEL(channel));
    pGPDMACh[channel]->DMACCConfig |= GPDMA_DMACCxConfig_E;
}

void GPDMA_ChannelStop(GPDMA_CH channel) {
    CHECK_PARAM(PARAM_GPDMA_CHANNEL(channel));
    pGPDMACh[channel]->DMACCConfig &= ~GPDMA_DMACCxConfig_E;
}

void GPDMA_ChannelGracefulStop(GPDMA_CH channel) {
    CHECK_PARAM(PARAM_GPDMA_CHANNEL(channel));
    LPC_GPDMACH_TypeDef* pDMAch = pGPDMACh[channel];
    pDMAch->DMACCConfig |= GPDMA_DMACCxConfig_H;
    while (pDMAch->DMACCConfig & GPDMA_DMACCxConfig_A) {}
    pDMAch->DMACCConfig &= ~GPDMA_DMACCxConfig_E;
}

void GPDMA_ChannelPause(GPDMA_CH channel) {
    CHECK_PARAM(PARAM_GPDMA_CHANNEL(channel));
    pGPDMACh[channel]->DMACCConfig |= GPDMA_DMACCxConfig_H;
}

void GPDMA_ChannelResume(GPDMA_CH channel) {
    CHECK_PARAM(PARAM_GPDMA_CHANNEL(channel));
    pGPDMACh[channel]->DMACCConfig &= ~GPDMA_DMACCxConfig_H;
}

IntStatus GPDMA_IntGetStatus(GPDMA_STATUS_TYPE type, GPDMA_CH channel) {
    CHECK_PARAM(PARAM_GPDMA_STAT(type));
    CHECK_PARAM(PARAM_GPDMA_CHANNEL(channel));

    uint32_t status_reg = 0;

    const uint32_t channel_mask = GPDMA_ChannelBit(channel);

    switch (type) {
        case GPDMA_INT: status_reg = LPC_GPDMA->DMACIntStat; break;
        case GPDMA_INTTC: status_reg = LPC_GPDMA->DMACIntTCStat; break;
        case GPDMA_INTERR: status_reg = LPC_GPDMA->DMACIntErrStat; break;
        case GPDMA_RAW_INTTC: status_reg = LPC_GPDMA->DMACRawIntTCStat; break;
        case GPDMA_RAW_INTERR: status_reg = LPC_GPDMA->DMACRawIntErrStat; break;
        case GPDMA_ENABLED_CH: status_reg = LPC_GPDMA->DMACEnbldChns; break;
        default: return RESET;
    }

    return (status_reg & channel_mask) ? SET : RESET;
}

void GPDMA_ClearIntPending(GPDMA_CLEAR_INT type, GPDMA_CH channel) {
    CHECK_PARAM(PARAM_GPDMA_CLEAR_INT(type));
    CHECK_PARAM(PARAM_GPDMA_CHANNEL(channel));

    if (type == GPDMA_CLR_INTTC) {
        LPC_GPDMA->DMACIntTCClear = GPDMA_ChannelBit(channel);
    }
    if (type == GPDMA_CLR_INTERR) {
        LPC_GPDMA->DMACIntErrClr = GPDMA_ChannelBit(channel);
    }
}

/**
 * @}
 */

#endif /* _GPDMA */

/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */
