/**
 * @file        lpc17xx_dac.c
 * @brief       Contains all functions support for DAC firmware library on LPC17xx
 * @version     2.0
 * @date        21. May. 2010
 * @author      NXP MCU SW Application Team
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
 * Last update: 22/02/2026, Author: David Trujillo Medina
 */

/* ---------------------------- Peripheral group ---------------------------- */
/** @addtogroup DAC
 * @{
 */

/* -------------------------------- Includes -------------------------------- */
#include "lpc17xx_dac.h"
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

#ifdef _DAC

/* ---------------------------- Public Functions ---------------------------- */
/** @addtogroup DAC_Public_Functions
 * @{
 */

void DAC_Init(void) {
    LPC_PINCON->PINSEL1  = (LPC_PINCON->PINSEL1 & ~(0x3 << 20)) | (0x2 << 20);
    LPC_PINCON->PINMODE1 = (LPC_PINCON->PINMODE1 & ~(0x3 << 20)) | (0x2 << 20);

    CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_DAC, CLKPWR_PCLKSEL_CCLK_DIV_4);

    DAC_SetBias(DAC_700uA);
}

void DAC_UpdateValue(uint32_t newValue) {
    LPC_DAC->DACR = (LPC_DAC->DACR & ~(0x3FF << 6)) | ((newValue & 0x3FF) << 6);
}

void DAC_SetBias(DAC_MAX_CURRENT maxCurr) {
    CHECK_PARAM(PARAM_DAC_MAX_CURRENT(maxCurr));

    LPC_DAC->DACR = (LPC_DAC->DACR & ~DAC_BIAS_EN) | (maxCurr == DAC_350uA ? DAC_BIAS_EN : 0);
}

void DAC_ConfigDAConverterControl(const DAC_CONVERTER_CFG_T* dacCfg) {
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(dacCfg->doubleBuffer));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(dacCfg->dmaCounter));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(dacCfg->dmaRequest));

    const uint32_t ctrl = (dacCfg->doubleBuffer ? DAC_DBLBUF_ENA : 0) |
                          (dacCfg->dmaCounter ? DAC_CNT_ENA : 0) |
                          (dacCfg->dmaRequest ? DAC_DMA_ENA : 0);
    LPC_DAC->DACCTRL = ctrl;
}

void DAC_SetDMATimeOut(uint16_t timeOut) {
    LPC_DAC->DACCNTVAL = DAC_CCNT_VALUE(timeOut);
}

/**
 * @}
 */

#endif /* _DAC */

/**
 * @}
 */

/* ------------------------------ End Of File ------------------------------- */
