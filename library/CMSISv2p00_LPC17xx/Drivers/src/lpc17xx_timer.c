/**
 * @file        lpc17xx_timer.c
 * @brief       Contains all functions support for Timer firmware library on LPC17xx
 * @version     3.0
 * @date        18. June. 2010
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
 * Last update: 21/02/2026, Author: David Trujillo Medina
 */

/* ---------------------------- Peripheral group ---------------------------- */
/** @addtogroup TIM
 * @{
 */

/* -------------------------------- Includes -------------------------------- */
#include "lpc17xx_timer.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_pinsel.h"

/* If this source file built with example, the LPC17xx FW library configuration
 * file in each example directory ("lpc17xx_libcfg.h") must be included,
 * otherwise the default FW library configuration file must be included instead
 */
#ifdef __BUILD_WITH_EXAMPLE__
#include "lpc17xx_libcfg.h"
#else
#include "lpc17xx_libcfg_default.h"
#endif /* __BUILD_WITH_EXAMPLE__ */

#ifdef _TIM

/* ---------------------- Private Function Prototypes ----------------------- */
/**
 * @brief Returns the peripheral clock frequency for the specified timer.
 *
 * @param[in] timerNum Timer index (0 to 3).
 *
 * @return Peripheral clock frequency in Hz, or 0 if the timer number is invalid.
 */
static uint32_t getPClock(uint32_t timerNum);

/**
 * @brief Converts a time in microseconds to timer ticks for the specified timer.
 *
 * @param[in] timerNum Timer index (0 to 3).
 * @param[in] uSec Time duration in microseconds.
 * @return Number of timer ticks required for the given time, or 0 if input is invalid.
 */
static uint32_t converUSecToVal(uint32_t timerNum, uint32_t uSec);

/**
 * @brief      Converts a timer register pointer to its timer number.
 *
 * This function maps the given LPC_TIM_TypeDef pointer to its corresponding
 * timer number representation.
 * If the pointer does not match any known timer, it returns 0xFFFFFFFF.
 *
 * @param[in]  TIMx  Pointer to the timer peripheral (LPC_TIM0/1/2/3).
 *
 * @return     Timer number (0 to 3), or 0xFFFFFFFF if the pointer is invalid.
 *
 * @note
 * - Use this function to identify the timer index from its register pointer.
 * - The return value 0xFFFFFFFF indicates an invalid or unknown timer pointer.
 */
static uint32_t converPtrToTimeNum(LPC_TIM_TypeDef* TIMx);

/**
 * @brief      Initializes the specified Timer/Counter peripheral.
 *
 * This function enables power and clock for the given timer peripheral, resets
 * its counters, and clears all interrupt flags. It prepares the timer for
 * subsequent configuration and use.
 *
 * @param[in]  TIMx  Pointer to the timer peripheral (LPC_TIM0/1/2/3).
 *
 * @note
 * - Does not enable the timer; it only initializes the peripheral.
 * - After calling this function, the timer is ready for configuration.
 */
static void TIM_Init(LPC_TIM_TypeDef* TIMx);
/* ------------------- End of Private Function Prototypes ------------------- */

/* --------------------------- Private Functions ---------------------------- */
static uint32_t getPClock(uint32_t timerNum) {
    static const uint32_t clk_selectors[] = {CLKPWR_PCLKSEL_TIMER0, CLKPWR_PCLKSEL_TIMER1,
                                             CLKPWR_PCLKSEL_TIMER2, CLKPWR_PCLKSEL_TIMER3};
    if (timerNum > 3) {
        return 0;
    }
    return CLKPWR_GetPCLK(clk_selectors[timerNum]);
}

static uint32_t converUSecToVal(uint32_t timerNum, uint32_t uSec) {
    uint64_t pclk = getPClock(timerNum);
    if (uSec == 0) {
        return 0;
    }
    return (uint32_t)(pclk * uSec / 1000000);
}

static uint32_t converPtrToTimeNum(LPC_TIM_TypeDef* TIMx) {
    switch ((uintptr_t)TIMx) {
        case (uintptr_t)LPC_TIM0: return 0;
        case (uintptr_t)LPC_TIM1: return 1;
        case (uintptr_t)LPC_TIM2: return 2;
        case (uintptr_t)LPC_TIM3: return 3;
        default: break;
    }
    return 0xFFFFFFFF;
}

static void TIM_Init(LPC_TIM_TypeDef* TIMx) {
    if (TIMx == LPC_TIM0) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM0, ENABLE);
        CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_TIMER0, CLKPWR_PCLKSEL_CCLK_DIV_4);
    } else if (TIMx == LPC_TIM1) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM1, ENABLE);
        CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_TIMER1, CLKPWR_PCLKSEL_CCLK_DIV_4);
    } else if (TIMx == LPC_TIM2) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM2, ENABLE);
        CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_TIMER2, CLKPWR_PCLKSEL_CCLK_DIV_4);
    } else if (TIMx == LPC_TIM3) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM3, ENABLE);
        CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_TIMER3, CLKPWR_PCLKSEL_CCLK_DIV_4);
    }
    TIMx->TCR = 0;
    TIM_ResetCounter(TIMx);
    TIMx->IR = 0x3F;
}
/* ------------------------ End of Private Functions ------------------------ */

/* ---------------------------- Public Functions ---------------------------- */
/** @addtogroup TIM_Public_Functions
 * @{
 */
void TIM_InitTimer(LPC_TIM_TypeDef* TIMx, const TIM_TIMERCFG_T* timerCfg) {
    CHECK_PARAM(PARAM_TIMx(TIMx));
    CHECK_PARAM(PARAM_TIM_PRESCALE(timerCfg->prescaleOpt));

    TIM_Init(TIMx);

    if (timerCfg->prescaleOpt == TIM_TICK) {
        if (timerCfg->prescaleValue > 0) {
            TIMx->PR = timerCfg->prescaleValue - 1;
        } else {
            TIMx->PR = 0;
        }
    } else {
        const uint32_t prVal = converUSecToVal(converPtrToTimeNum(TIMx), timerCfg->prescaleValue);

        TIMx->PR = (prVal > 0) ? (prVal - 1) : 0;
    }
}

void TIM_InitCounter(LPC_TIM_TypeDef* TIMx, const TIM_COUNTERCFG_T* counterCfg) {
    CHECK_PARAM(PARAM_TIMx(TIMx));
    CHECK_PARAM(PARAM_TIM_CAPTURE_CH(counterCfg->input));
    CHECK_PARAM(PARAM_TIM_CTR_EDGE(counterCfg->edge));

    TIM_Init(TIMx);

    TIMx->CTCR = counterCfg->edge | (counterCfg->input << 2);
}

void TIM_DeInit(LPC_TIM_TypeDef* TIMx) {
    CHECK_PARAM(PARAM_TIMx(TIMx));

    TIMx->TCR = 0x00;

    if (TIMx == LPC_TIM0) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM0, DISABLE);
    } else if (TIMx == LPC_TIM1) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM1, DISABLE);
    } else if (TIMx == LPC_TIM2) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM2, DISABLE);
    } else if (TIMx == LPC_TIM3) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM3, DISABLE);
    }
}

void TIM_Enable(LPC_TIM_TypeDef* TIMx) {
    CHECK_PARAM(PARAM_TIMx(TIMx));

    TIMx->TCR |= 1UL;
}

void TIM_Disable(LPC_TIM_TypeDef* TIMx) {
    CHECK_PARAM(PARAM_TIMx(TIMx));

    TIMx->TCR &= ~1UL;
}

uint32_t TIM_ReadTimer(LPC_TIM_TypeDef* TIMx) {
    CHECK_PARAM(PARAM_TIMx(TIMx));

    return TIMx->TC;
}

uint32_t TIM_ReadPrescale(LPC_TIM_TypeDef* TIMx) {
    CHECK_PARAM(PARAM_TIMx(TIMx));

    return TIMx->PC;
}

void TIM_ResetCounter(LPC_TIM_TypeDef* TIMx) {
    CHECK_PARAM(PARAM_TIMx(TIMx));

    TIMx->TCR |= TIM_RESET;
    TIMx->TCR &= ~TIM_RESET;
}

void TIM_ConfigMatch(LPC_TIM_TypeDef* TIMx, const TIM_MATCHCFG_T* matchCfg) {
    CHECK_PARAM(PARAM_TIMx(TIMx));
    CHECK_PARAM(PARAM_TIM_MATCH_CH(matchCfg->channel));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(matchCfg->intEn));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(matchCfg->stopEn));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(matchCfg->resetEn));
    CHECK_PARAM(PARAM_TIM_EXTMATCH_OPT(matchCfg->extOpt));

    TIM_UpdateMatchValue(TIMx, matchCfg->channel, matchCfg->matchValue);

    TIMx->MCR &= ~TIM_MCR_CHANNEL_MASKBIT(matchCfg->channel);

    const uint32_t mcrMask = matchCfg->intEn | matchCfg->resetEn << 1 | matchCfg->stopEn << 2;
    TIMx->MCR &= ~TIM_MCR_CHANNEL_MASKBIT(matchCfg->channel);
    TIMx->MCR |= mcrMask << (matchCfg->channel * 3);

    TIM_SetMatchExt(TIMx, matchCfg->channel, matchCfg->extOpt);
}

void TIM_UpdateMatchValue(LPC_TIM_TypeDef* TIMx, TIM_MATCH_CH channel, uint32_t matchValue) {
    CHECK_PARAM(PARAM_TIMx(TIMx));
    CHECK_PARAM(PARAM_TIM_MATCH_CH(channel));

    TIMx->IR = TIM_IR_CLR(channel);

    volatile uint32_t* mReg = &TIMx->MR0;
    mReg[channel]           = matchValue;
}

void TIM_SetMatchExt(LPC_TIM_TypeDef* TIMx, TIM_MATCH_CH channel, TIM_EXTMATCH_OPT type) {
    CHECK_PARAM(PARAM_TIMx(TIMx));
    CHECK_PARAM(PARAM_TIM_MATCH_CH(channel));
    CHECK_PARAM(PARAM_TIM_EXTMATCH_OPT(type));

    TIMx->EMR &= ~TIM_EMR_MASK(channel);
    TIMx->EMR |= TIM_EM_SET(channel, type);
}

void TIM_ConfigCapture(LPC_TIM_TypeDef* TIMx, const TIM_CAPTURECFG_T* capCfg) {
    CHECK_PARAM(PARAM_TIMx(TIMx));
    CHECK_PARAM(PARAM_TIM_CAPTURE_CH(capCfg->channel));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(capCfg->risingEn));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(capCfg->fallingEn));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(capCfg->intEn));

    TIMx->CCR &= ~TIM_CCR_CHANNEL_MASKBIT(capCfg->channel);

    const uint32_t capMask = capCfg->risingEn | capCfg->fallingEn << 1 | capCfg->intEn << 2;
    TIMx->CCR &= ~TIM_CCR_CHANNEL_MASKBIT(capCfg->channel);
    TIMx->CCR |= capMask << (capCfg->channel * 3);
}

uint32_t TIM_GetCaptureValue(LPC_TIM_TypeDef* TIMx, TIM_CAPTURE_CH channel) {
    CHECK_PARAM(PARAM_TIMx(TIMx));
    CHECK_PARAM(PARAM_TIM_CAPTURE_CH(channel));

    volatile const uint32_t* pCR = &TIMx->CR0;
    return pCR[channel];
}

void TIM_ClearIntPending(LPC_TIM_TypeDef* TIMx, TIM_INT intFlag) {
    CHECK_PARAM(PARAM_TIMx(TIMx));
    CHECK_PARAM(PARAM_TIM_INT(intFlag));

    TIMx->IR = TIM_IR_CLR(intFlag);
}

FlagStatus TIM_GetIntStatus(LPC_TIM_TypeDef* TIMx, TIM_INT intFlag) {
    CHECK_PARAM(PARAM_TIMx(TIMx));
    CHECK_PARAM(PARAM_TIM_INT(intFlag));

    return ((TIMx->IR & TIM_IR_CLR(intFlag)) ? SET : RESET);
}

void TIM_PinConfig(TIM_PIN_OPTION option) {
    CHECK_PARAM(PARAM_TIM_PIN_OPTION(option));

    static const PINSEL_CFG_T PinCfg[22] = {{PORT_1, PIN_26, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_1, PIN_27, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_1, PIN_28, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_3, PIN_25, PINSEL_FUNC_10, PINSEL_TRISTATE},
                                            {PORT_1, PIN_29, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_3, PIN_26, PINSEL_FUNC_10, PINSEL_TRISTATE},
                                            {PORT_1, PIN_18, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_1, PIN_19, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_1, PIN_22, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_1, PIN_25, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_0, PIN_4, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_0, PIN_5, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_0, PIN_6, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_4, PIN_28, PINSEL_FUNC_10, PINSEL_TRISTATE},
                                            {PORT_0, PIN_7, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_4, PIN_29, PINSEL_FUNC_10, PINSEL_TRISTATE},
                                            {PORT_0, PIN_8, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_0, PIN_9, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_0, PIN_23, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_0, PIN_24, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_0, PIN_10, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_0, PIN_11, PINSEL_FUNC_11, PINSEL_TRISTATE}};

    PINSEL_ConfigPin(&PinCfg[option]);
}

/**
 * @}
 */

#endif /* _TIM */

/**
 * @}
 */

/* ------------------------------ End Of File ------------------------------- */
