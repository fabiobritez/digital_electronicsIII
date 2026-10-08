/**
 * @file        lpc17xx_exti.c
 * @brief       Contains all functions support for External interrupt firmware library on LPC17xx
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
 * Last update: 20/02/2026, Author: David Trujillo Medina
 */

/* ---------------------------- Peripheral group ---------------------------- */
/** @addtogroup EXTI
 * @{
 */

/* -------------------------------- Includes -------------------------------- */
#include "lpc17xx_exti.h"

/* If this source file built with example, the LPC17xx FW library configuration
 * file in each example directory ("lpc17xx_libcfg.h") must be included,
 * otherwise the default FW library configuration file must be included instead
 */
#ifdef __BUILD_WITH_EXAMPLE__
#include "lpc17xx_libcfg.h"
#else
#include "lpc17xx_libcfg_default.h"
#endif /* __BUILD_WITH_EXAMPLE__ */

#ifdef _EXTI

/* ---------------------- Private Function Prototypes ----------------------- */
/**
 * @brief       Sets the mode (level or edge sensitivity) for a specific EXTI line.
 *
 * @param[in]   line    EXTI_EINTx [0...3].
 * @param[in]   mode    Mode selection, must be:
 *                      - EXTI_LEVEL_SENSITIVE
 *                      - EXTI_EDGE_SENSITIVE
 * @note        If the mode value is invalid, the function does nothing.
 */
static void EXTI_SetMode(EXTI_LINE line, EXTI_MODE mode);

/**
 * @brief       Sets the polarity (active level or edge) for a specific EXTI line.
 *
 * @param[in]   line        EXTI_EINTx [0...3].
 * @param[in]   polarity    Polarity selection, should be:
 *                          - EXTI_LOW_ACTIVE or EXTI_FALLING_EDGE (equivalent)
 *                          - EXTI_HIGH_ACTIVE or EXTI_RISING_EDGE (equivalent)
 * @note        If the polarity value is invalid, the function does nothing.
 */
static void EXTI_SetPolarity(EXTI_LINE line, EXTI_POLARITY polarity);
/* ------------------- End of Private Function Prototypes ------------------- */

/* --------------------------- Private Functions ---------------------------- */
static void EXTI_SetMode(EXTI_LINE line, EXTI_MODE mode) {
    if (mode == EXTI_EDGE_SENSITIVE) {
        LPC_SC->EXTMODE |= (1UL << line);
    } else {
        LPC_SC->EXTMODE &= ~(1UL << line);
    }
}

static void EXTI_SetPolarity(EXTI_LINE line, EXTI_POLARITY polarity) {
    if (polarity == EXTI_HIGH_ACTIVE) {
        LPC_SC->EXTPOLAR |= (1UL << line);
    } else {
        LPC_SC->EXTPOLAR &= ~(1UL << line);
    }
}
/* ------------------------ End of Private Functions ------------------------ */

/* ---------------------------- Public Functions ---------------------------- */
/** @addtogroup EXTI_Public_Functions
 * @{
 */

void EXTI_Init(void) {
    NVIC_DisableIRQ(EINT0_IRQn);
    NVIC_DisableIRQ(EINT1_IRQn);
    NVIC_DisableIRQ(EINT2_IRQn);
    NVIC_DisableIRQ(EINT3_IRQn);

    LPC_SC->EXTMODE  = 0x0;
    LPC_SC->EXTPOLAR = 0x0;
}

void EXTI_PinConfig(EXTI_LINE line, EXTI_RESISTOR resMode) {
    CHECK_PARAM(PARAM_EXTI_LINE(line));
    CHECK_PARAM(PARAM_EXTI_RESISTOR(resMode));

    const uint8_t bitPos = (uint8_t)(EINT_PIN_BASE_OFFSET + (line * 2));

    LPC_PINCON->PINSEL4 &= ~(0x03UL << bitPos);
    LPC_PINCON->PINSEL4 |= (0x01UL << bitPos);

    LPC_PINCON->PINMODE4 &= ~(0x03UL << bitPos);

    if (resMode == EXTI_PULLDOWN) {
        LPC_PINCON->PINMODE4 |= (0x3UL << bitPos);
    } else if (resMode == EXTI_NOPULL) {
        LPC_PINCON->PINMODE4 |= (0x2UL << bitPos);
    }
}

void EXTI_Config(const EXTI_CFG_T* extiCfg) {
    CHECK_PARAM(PARAM_EXTI_LINE(extiCfg->line));
    CHECK_PARAM(PARAM_EXTI_MODE(extiCfg->mode));
    CHECK_PARAM(PARAM_EXTI_POLARITY(extiCfg->polarity));

    NVIC_DisableIRQ((IRQn_Type)(EINT0_IRQn + extiCfg->line));

    EXTI_SetMode(extiCfg->line, extiCfg->mode);
    EXTI_SetPolarity(extiCfg->line, extiCfg->polarity);
}

void EXTI_ConfigEnable(const EXTI_CFG_T* extiCfg) {
    CHECK_PARAM(PARAM_EXTI_LINE(extiCfg->line));
    CHECK_PARAM(PARAM_EXTI_MODE(extiCfg->mode));
    CHECK_PARAM(PARAM_EXTI_POLARITY(extiCfg->polarity));

    EXTI_Config(extiCfg);
    EXTI_EnableIRQ(extiCfg->line);
}

void EXTI_ClearFlag(EXTI_LINE line) {
    CHECK_PARAM(PARAM_EXTI_LINE(line));

    LPC_SC->EXTINT = (1 << line);
}

FlagStatus EXTI_GetFlag(EXTI_LINE line) {
    CHECK_PARAM(PARAM_EXTI_LINE(line));

    return (LPC_SC->EXTINT & (1 << line)) ? SET : RESET;
}

void EXTI_EnableIRQ(EXTI_LINE line) {
    CHECK_PARAM(PARAM_EXTI_LINE(line));

    EXTI_ClearFlag(line);
    NVIC_ClearPendingIRQ((IRQn_Type)(EINT0_IRQn + line));
    NVIC_EnableIRQ((IRQn_Type)(EINT0_IRQn + line));
}

/**
 * @}
 */

#endif /* _EXTI */

/**
 * @}
 */

/* ------------------------------ End Of File ------------------------------- */
