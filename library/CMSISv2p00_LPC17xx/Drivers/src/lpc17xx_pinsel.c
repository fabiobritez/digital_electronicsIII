/**
 * @file        lpc17xx_pinsel.c
 * @brief       Contains all functions support for Pin connect block firmware
 *              library on LPC17xx
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
 * Last update: 20/02/2026, Author: David Trujillo Medina
 */

/* ---------------------------- Peripheral group ---------------------------- */
/** @addtogroup PINSEL
 * @{
 */

/* -------------------------------- Includes -------------------------------- */
#include "lpc17xx_pinsel.h"
#include "lpc17xx_libcfg_default.h"

/* ---------------------- Private Function Prototypes ----------------------- */
/**
 * @brief       Sets up the pin selection function.
 *
 * @param[in]   port  PORT_x, where x is in the range [0...4].
 * @param[in]   pin   PIN_x, where x is in the range [0...31].
 * @param[in]   func  PINSEL_FUNC_x, where x is in the range [00...11].
 */
static void PINSEL_setPinFunc(uint8_t port, uint8_t pin, uint8_t func);

/**
 * @brief       Configures the resistor mode for a pin.
 *
 * @param[in]   port  PORT_x, where x is in the range [0...4].
 * @param[in]   pin   PIN_x, where x is in the range [0...31].
 * @param[in]   mode: Must be one of:
 *                    - PINSEL_PULLUP    : Internal pull-up resistor.
 *                    - PINSEL_REPEATER  : Repeater mode.
 *                    - PINSEL_TRISTATE  : Tri-state.
 *                    - PINSEL_PULLDOWN  : Internal pull-down resistor.
 */
static void PINSEL_setResistorMode(uint8_t port, uint8_t pin, uint8_t mode);

/**
 * @brief       Configures the open-drain mode for a pin.
 *
 * @param[in]   port    PORT_x, where x is in the range [0...4].
 * @param[in]   pin     PIN_x, where x is in the range [0...31].
 * @param[in]   state   Must be one of:
 *                      - ENABLE : Open-drain mode enabled.
 *                      - DISABLE : Open-drain mode disabled (normal mode).
 */
static void PINSEL_setOpenDrainMode(uint8_t port, uint8_t pin, FunctionalState state);
/* ------------------- End of Private Function Prototypes ------------------- */

/* --------------------------- Private Functions ---------------------------- */
static void PINSEL_setPinFunc(uint8_t port, uint8_t pin, uint8_t func) {
    const uint32_t regIDx = 2 * port + pin / 16;
    const uint32_t pinPos = (pin % 16) * 2;

    __IO uint32_t* pPinCon = &LPC_PINCON->PINSEL0;

    pPinCon[regIDx] &= ~(PINSEL_FUNC_MASK << pinPos);
    pPinCon[regIDx] |= (uint32_t)func << pinPos;
}

static void PINSEL_setResistorMode(uint8_t port, uint8_t pin, uint8_t mode) {
    const uint32_t regIDx = (2 * port) + (pin / 16);
    const uint32_t pinPos = (pin % 16) * 2;

    __IO uint32_t* pModeCon = &LPC_PINCON->PINMODE0;

    pModeCon[regIDx] &= ~(PINSEL_RES_MODE_MASK << pinPos);
    pModeCon[regIDx] |= (uint32_t)mode << pinPos;
}

static void PINSEL_setOpenDrainMode(uint8_t port, uint8_t pin, FunctionalState state) {
    __IO uint32_t* pODCon = &LPC_PINCON->PINMODE_OD0;

    if (state == ENABLE) {
        pODCon[port] |= (PINSEL_OD_MASK << pin);
    } else {
        pODCon[port] &= ~(PINSEL_OD_MASK << pin);
    }
}
/* ------------------------ End of Private Functions ------------------------ */

/* ---------------------------- Public Functions ---------------------------- */
/** @addtogroup PINSEL_Public_Functions
 * @{
 */

void PINSEL_ConfigPin(const PINSEL_CFG_T* pinCfg) {
    CHECK_PARAM(PARAM_LPC_PORT(pinCfg->port));
    CHECK_PARAM(PARAM_LPC_PIN(pinCfg->pin));
    CHECK_PARAM(PARAM_PINSEL_FUNC(pinCfg->func));
    CHECK_PARAM(PARAM_PINSEL_MODE(pinCfg->mode));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(pinCfg->openDrain));

    PINSEL_setPinFunc(pinCfg->port, pinCfg->pin, pinCfg->func);
    PINSEL_setResistorMode(pinCfg->port, pinCfg->pin, pinCfg->mode);
    PINSEL_setOpenDrainMode(pinCfg->port, pinCfg->pin, pinCfg->openDrain);
}

void PINSEL_ConfigMultiplePins(const PINSEL_CFG_T* pinCfg, uint32_t pinMask) {
    CHECK_PARAM(PARAM_LPC_PORT(pinCfg->port));
    CHECK_PARAM(PARAM_PINSEL_FUNC(pinCfg->func));

    PINSEL_CFG_T tempCfg = *pinCfg;
    for (uint8_t pin = 0; pin < 32; pin++) {
        if (pinMask & 1U << pin) {
            tempCfg.pin = pin;
            PINSEL_ConfigPin(&tempCfg);
        }
    }
}

void PINSEL_ConfigTraceFunc(FunctionalState newState) {
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(newState));

    if (newState == ENABLE) {
        LPC_PINCON->PINSEL10 |= (PINSEL_PIN_MASK << PINSEL_TRACE_POS);
    } else {
        LPC_PINCON->PINSEL10 &= ~(PINSEL_PIN_MASK << PINSEL_TRACE_POS);
    }
}

void PINSEL_SetI2CPins(PINSEL_I2C_MODE driveMode, FunctionalState filterSlewRate) {
    CHECK_PARAM(PARAM_PINSEL_I2C_MODE(driveMode));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(filterSlewRate));

    uint32_t regVal = 0;

    if (driveMode == PINSEL_I2C_FAST) {
        regVal = PINSEL_I2CPADCFG_SCLDRV0 | PINSEL_I2CPADCFG_SDADRV0;
    }

    if (filterSlewRate == DISABLE) {
        regVal = PINSEL_I2CPADCFG_SCLI2C0 | PINSEL_I2CPADCFG_SDAI2C0;
    }

    LPC_PINCON->I2CPADCFG = regVal;
}

/**
 * @}
 */

/**
 * @}
 */

/* ------------------------------ End Of File ------------------------------- */
