/**
 * @file        lpc17xx_systick.c
 * @brief       Contains all functions support for SYSTICK firmware library on LPC17xx
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
 * Date: 09/07/2025, Author: David Trujillo Medina
 */

/* ---------------------------- Peripheral group ---------------------------- */
/** @addtogroup SYSTICK
 * @{
 */

/* -------------------------------- Includes -------------------------------- */
#include "lpc17xx_systick.h"
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

#ifdef _SYSTICK
/* ---------------------- Private Function Prototypes ----------------------- */
/**
 * @brief       Configures the pin for external SysTick clock input.
 *
 * Sets the appropriate function and mode for the pin used as the external
 * clock input for the SysTick timer. The pin is set to the correct function
 * in `PINSEL7` and configured in tristate (no pull-up/pull-down) in `PINMODE7`.
 */
static void SYSTICK_PinConfig(void);
/* ------------------- End of Private Function Prototypes ------------------- */

/* --------------------------- Private Functions ---------------------------- */
static void SYSTICK_PinConfig(void) {
    LPC_PINCON->PINSEL7 &= ~(0x3 << ST_PIN_PCB_POS);
    LPC_PINCON->PINSEL7 |= (0x1 << ST_PIN_PCB_POS);

    LPC_PINCON->PINMODE7 &= ~(0x3 << ST_PIN_PCB_POS);
    LPC_PINCON->PINMODE7 |= (0x2 << ST_PIN_PCB_POS);
}
/* ------------------------ End of Private Functions ------------------------ */

/* ---------------------------- Public Functions ---------------------------- */
/** @addtogroup SYSTICK_Public_Functions
 * @{
 */

void SYSTICK_InternalInit(uint32_t time) {
    const uint32_t ticksPerMs = SystemCoreClock / 1000;

    if (time > (ST_MAX_LOAD / ticksPerMs)) {
        SysTick->LOAD = ST_MAX_LOAD;
    } else {
        SysTick->LOAD = (ticksPerMs * time) - 1;
    }

    SysTick->VAL = 0;
    SysTick->CTRL |= ST_CTRL_CLKSOURCE;
}

void SYSTICK_ExternalInit(uint32_t extFreq, uint32_t time) {
    SYSTICK_PinConfig();

    const uint32_t ticksPerMs = extFreq / 1000;

    if (ticksPerMs == 0) {
        return;  // Avoid division by zero if extFreq is less than 1000 Hz
    }

    if (time > (ST_MAX_LOAD / ticksPerMs)) {
        SysTick->LOAD = ST_MAX_LOAD;
    } else {
        SysTick->LOAD = (ticksPerMs * time) - 1;
    }

    SysTick->VAL = 0;
    SysTick->CTRL &= ~ST_CTRL_CLKSOURCE;
}

void SYSTICK_Cmd(FunctionalState newState) {
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(newState));

    if (newState == ENABLE) {
        SysTick->CTRL |= ST_CTRL_ENABLE;
    } else {
        SysTick->CTRL &= ~ST_CTRL_ENABLE;
    }
}

void SYSTICK_IntCmd(FunctionalState newState) {
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(newState));

    if (newState == ENABLE) {
        SysTick->CTRL |= ST_CTRL_TICKINT;
    } else {
        SysTick->CTRL &= ~ST_CTRL_TICKINT;
    }
}

/**
 * @}
 */

#endif /* _SYSTICK */

/**
 * @}
 */

/* ------------------------------ End Of File ------------------------------- */
