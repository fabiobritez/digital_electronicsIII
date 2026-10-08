/**
 * @file        lpc17xx_pwm.c
 * @brief       Contains all functions support for PWM firmware library on LPC17xx
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
 * Last update: 21/02/2026, Author: David Trujillo Medina
 */

/* ---------------------------- Peripheral group ---------------------------- */
/** @addtogroup PWM
 * @{
 */

/* -------------------------------- Includes -------------------------------- */
#include "lpc17xx_pwm.h"
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

#ifdef _PWM

static uint32_t converUSecToVal(uint32_t uSec);

/**
 * @brief Converts a time in microseconds to timer ticks for the specified timer.
 *
 * @param[in] uSec Time duration in microseconds.
 * @return Number of timer ticks required for the given time, or 0 if input is invalid.
 */
static uint32_t converUSecToVal(uint32_t uSec) {
    const uint64_t pclk = CLKPWR_GetPCLK(CLKPWR_PCLKSEL_PWM1);
    if (uSec == 0)
        return 0;
    return (uint32_t)(pclk * uSec / 1000000);
}

static void PWM_Init(void) {
    CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCPWM1, ENABLE);
    CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_PWM1, CLKPWR_PCLKSEL_CCLK_DIV_4);

    LPC_PWM1->TCR = 0x00;
    PWM_ResetCounter();
    LPC_PWM1->MCR  = 0x00;
    LPC_PWM1->CCR  = 0x00;
    LPC_PWM1->PCR  = 0x00;
    LPC_PWM1->LER  = 0x00;
    LPC_PWM1->CTCR = 0x00;
    LPC_PWM1->IR   = PWM_IR_BITMASK;
}

/* ---------------------------- Public Functions ---------------------------- */
/** @addtogroup PWM_Public_Functions
 * @{
 */

void PWM_InitTimer(const PWM_TIMERCFG_T* timerCfg) {
    CHECK_PARAM(PARAM_PWM_PRESCALE(timerCfg->prescaleOpt));

    PWM_Init();

    if (timerCfg->prescaleOpt == PWM_TICK) {
        if (timerCfg->prescaleValue > 0) {
            LPC_PWM1->PR = timerCfg->prescaleValue - 1;
        } else {
            LPC_PWM1->PR = 0;
        }
    } else {
        const uint32_t prVal = converUSecToVal(timerCfg->prescaleValue);

        LPC_PWM1->PR = prVal > 0 ? prVal - 1 : 0;
    }
}

void PWM_InitCounter(const PWM_COUNTERCFG_T* counterCfg) {
    CHECK_PARAM(PARAM_PWM_CAPTURE(counterCfg->input));
    CHECK_PARAM(PARAM_PWM_CTR_EDGE(counterCfg->edge));

    PWM_Init();

    LPC_PWM1->CTCR = counterCfg->edge | (counterCfg->input << 2);
}

void PWM_DeInit(void) {
    LPC_PWM1->TCR = 0x00;
    CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCPWM1, DISABLE);
}

void PWM_PinConfig(PWM_PIN_OPTION option) {
    CHECK_PARAM(PARAM_PWM_PIN_OPTION(option));

    static const PINSEL_CFG_T PinCfg[14] = {{PORT_1, PIN_18, PINSEL_FUNC_10, PINSEL_TRISTATE},
                                            {PORT_2, PIN_0, PINSEL_FUNC_01, PINSEL_TRISTATE},
                                            {PORT_1, PIN_20, PINSEL_FUNC_10, PINSEL_TRISTATE},
                                            {PORT_2, PIN_1, PINSEL_FUNC_01, PINSEL_TRISTATE},
                                            {PORT_3, PIN_25, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_1, PIN_21, PINSEL_FUNC_10, PINSEL_TRISTATE},
                                            {PORT_2, PIN_2, PINSEL_FUNC_01, PINSEL_TRISTATE},
                                            {PORT_3, PIN_26, PINSEL_FUNC_11, PINSEL_TRISTATE},
                                            {PORT_1, PIN_23, PINSEL_FUNC_10, PINSEL_TRISTATE},
                                            {PORT_2, PIN_3, PINSEL_FUNC_01, PINSEL_TRISTATE},
                                            {PORT_1, PIN_24, PINSEL_FUNC_10, PINSEL_TRISTATE},
                                            {PORT_2, PIN_4, PINSEL_FUNC_01, PINSEL_TRISTATE},
                                            {PORT_1, PIN_26, PINSEL_FUNC_10, PINSEL_TRISTATE},
                                            {PORT_2, PIN_5, PINSEL_FUNC_01, PINSEL_TRISTATE}};

    PINSEL_ConfigPin(&PinCfg[option]);
}

void PWM_ChannelConfig(PWM_CHANNEL channel, PWM_CHANNEL_EDGE edgeMode) {
    CHECK_PARAM(PARAM_PWM_CHANNEL(channel));
    CHECK_PARAM(PARAM_PWM_CHANNEL_EDGE(edgeMode));

    if (channel == PWM_CHANNEL_1) {
        return;
    }

    if (edgeMode == PWM_SINGLE_EDGE) {
        LPC_PWM1->PCR &= ~PWM_PCR_PWMSELn(channel);
    } else {
        LPC_PWM1->PCR |= PWM_PCR_PWMSELn(channel);
    }
}

void PWM_Enable() {
    LPC_PWM1->TCR |= PWM_TCR_PWM_ENABLE;
}

void PWM_Disable() {
    LPC_PWM1->TCR &= ~PWM_TCR_PWM_ENABLE;
}

void PWM_ChannelEnable(PWM_CHANNEL channel) {
    CHECK_PARAM(PARAM_PWM_CHANNEL(channel));

    LPC_PWM1->PCR |= PWM_PCR_PWMENAn(channel);
}

void PWM_ChannelDisable(PWM_CHANNEL channel) {
    CHECK_PARAM(PARAM_PWM_CHANNEL(channel));

    LPC_PWM1->PCR &= ~PWM_PCR_PWMENAn(channel);
}

void PWM_CounterEnable() {
    LPC_PWM1->TCR |= PWM_TCR_COUNTER_ENABLE;
}

void PWM_CounterDisable() {
    LPC_PWM1->TCR &= ~PWM_TCR_COUNTER_ENABLE;
}

void PWM_ResetCounter(void) {
    LPC_PWM1->TCR |= PWM_TCR_COUNTER_RESET;
    LPC_PWM1->TCR &= ~PWM_TCR_COUNTER_RESET;
}

void PWM_ConfigMatch(const PWM_MATCHCFG_T* pwmMatchCfg) {
    CHECK_PARAM(PARAM_PWM_MATCH_OPT(pwmMatchCfg->channel));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(pwmMatchCfg->intEn));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(pwmMatchCfg->resetEn));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(pwmMatchCfg->stopEn));

    LPC_PWM1->MCR &= ~PWM_MCR_CHANNEL_MASKBIT(pwmMatchCfg->channel);

    const uint32_t mcrMask =
        pwmMatchCfg->intEn | pwmMatchCfg->resetEn << 1 | pwmMatchCfg->stopEn << 2;
    LPC_PWM1->MCR &= ~PWM_MCR_CHANNEL_MASKBIT(pwmMatchCfg->channel);
    LPC_PWM1->MCR |= mcrMask << (pwmMatchCfg->channel * 3);

    PWM_MatchUpdateSingle(pwmMatchCfg->channel, pwmMatchCfg->matchValue);
}

void PWM_MatchUpdateSingle(PWM_MATCH_OPT match, uint32_t matchValue) {
    CHECK_PARAM(PARAM_PWM_MATCH_OPT(match));

    volatile uint32_t* MR[] = {&LPC_PWM1->MR0, &LPC_PWM1->MR1, &LPC_PWM1->MR2, &LPC_PWM1->MR3,
                               &LPC_PWM1->MR4, &LPC_PWM1->MR5, &LPC_PWM1->MR6};

    *MR[match] = matchValue;
    LPC_PWM1->LER |= _BIT(match);
}

void PWM_MatchUpdateDouble(PWM_MATCH_OPT matchA, uint32_t newValueA, PWM_MATCH_OPT matchB,
                           uint32_t newValueB) {
    CHECK_PARAM(PARAM_PWM_MATCH_OPT(matchA));
    CHECK_PARAM(PARAM_PWM_MATCH_OPT(matchB));

    volatile uint32_t* MR[] = {&LPC_PWM1->MR0, &LPC_PWM1->MR1, &LPC_PWM1->MR2, &LPC_PWM1->MR3,
                               &LPC_PWM1->MR4, &LPC_PWM1->MR5, &LPC_PWM1->MR6};

    *MR[matchA] = newValueA;
    *MR[matchB] = newValueB;
    LPC_PWM1->LER |= _BIT(matchA) | _BIT(matchB);
}

void PWM_ClearIntPending(PWM_INT_TYPE intFlag) {
    CHECK_PARAM(PARAM_PWM_INT_TYPE(intFlag));

    LPC_PWM1->IR = _BIT(intFlag);
}

FlagStatus PWM_GetIntStatus(PWM_INT_TYPE intFlag) {
    CHECK_PARAM(PARAM_PWM_INT_TYPE(intFlag));

    return LPC_PWM1->IR & _BIT(intFlag) ? SET : RESET;
}

void PWM_ConfigCapture(const PWM_CAPTURECFG_T* capCfg) {
    CHECK_PARAM(PARAM_PWM_CAPTURE(capCfg->channel));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(capCfg->risingEn));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(capCfg->fallingEn));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(capCfg->intEn));

    LPC_PWM1->CCR &= ~PWM_CCR_CHANNEL_MASKBIT(capCfg->channel);

    const uint32_t capMask = capCfg->risingEn | capCfg->fallingEn << 1 | capCfg->intEn << 2;
    LPC_PWM1->CCR &= ~PWM_CCR_CHANNEL_MASKBIT(capCfg->channel);
    LPC_PWM1->CCR |= capMask << (capCfg->channel * 3);
}

uint32_t PWM_GetCaptureValue(PWM_CAPTURE capChannel) {
    CHECK_PARAM(PARAM_PWM_CAPTURE(capChannel));

    switch (capChannel) {
        case PWM_CAPTURE_0: return LPC_PWM1->CR0;

        case PWM_CAPTURE_1: return LPC_PWM1->CR1;

        default: return 0;
    }
}

/**
 * @}
 */

#endif /* _PWM */

/**
 * @}
 */

/* ------------------------------ End Of File ------------------------------- */
