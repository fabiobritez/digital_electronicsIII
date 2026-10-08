/**
 * @file        lpc17xx_exti.h
 * @brief       Contains all macro definitions and function prototypes
 *              support for External interrupt firmware library on LPC17xx
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
/** @defgroup EXTI EXTI
 * @ingroup LPC1700CMSIS_FwLib_Drivers
 * @{
 */

#ifndef LPC17XX_EXTI_H_
#define LPC17XX_EXTI_H_

/* -------------------------------- Includes -------------------------------- */
#include "LPC17xx.h"
#include "lpc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------- Private Macros ----------------------------- */
/** @defgroup EXTI_Private_Macros EXTI Private Macros
 * @{
 */

/* External interrupt pin offset definition. */
#define EINT_PIN_BASE_OFFSET (20)

/* ------------------------ MACROS MASKS DEFINITIONS ------------------------ */
/** All external interrupt lines mask. */
#define EINT_ALL_MASK ((0xF))

/**
 * @}
 */

/* ------------------------------ Public Types ------------------------------ */
/** @defgroup EXTI_Public_Types EXTI Public Types
 * @{
 */

/**
 * @brief EXTI external interrupt line option.
 */
typedef enum {
    EXTI_EINT0, /*!<  P2.10.*/
    EXTI_EINT1, /*!<  P2.11.*/
    EXTI_EINT2, /*!<  P2.12.*/
    EXTI_EINT3  /*!<  P2.13.*/
} EXTI_LINE;
/** Check EXTI line option parameter. */
#define PARAM_EXTI_LINE(LINE) ((LINE) >= EXTI_EINT0 && (LINE) <= EXTI_EINT3)

/**
 * @brief EXTI mode option.
 */
typedef enum {
    EXTI_LEVEL_SENSITIVE,
    EXTI_EDGE_SENSITIVE
} EXTI_MODE;
/** Check EXTI mode option parameter. */
#define PARAM_EXTI_MODE(MODE) ((MODE) == EXTI_LEVEL_SENSITIVE || (MODE) == EXTI_EDGE_SENSITIVE)

/**
 * @brief EXTI polarity option.
 */
typedef enum {
    EXTI_LOW_ACTIVE   = 0,
    EXTI_FALLING_EDGE = 0,
    EXTI_HIGH_ACTIVE  = 1,
    EXTI_RISING_EDGE  = 1
} EXTI_POLARITY;
/** Check EXTI polarity option parameter. */
#define PARAM_EXTI_POLARITY(POL) ((POL) == EXTI_LOW_ACTIVE || (POL) == EXTI_HIGH_ACTIVE)

/**
 * @brief EXTI resistor option.
 */
typedef enum {
    EXTI_PULLUP = 0,
    EXTI_PULLDOWN,
    EXTI_NOPULL
} EXTI_RESISTOR;
/** Check EXTI resistor option parameter. */
#define PARAM_EXTI_RESISTOR(RES) ((RES) >= EXTI_PULLUP && (RES) <= EXTI_NOPULL)

/**
 * @brief EXTI Initialize structure.
 */
typedef struct {
    EXTI_LINE line;         /*!< EXTI_EINTx [0...3]. */
    EXTI_MODE mode;         /*!< EXTI_LEVEL_SENSITIVE or EXTI_EDGE_SENSITIVE. */
    EXTI_POLARITY polarity; /*!< - EXTI_LOW_ACTIVE
                                 - EXTI_FALLING_EDGE
                                 - EXTI_HIGH_ACTIVE
                                 - EXTI_RISING_EDGE */
} EXTI_CFG_T;

/**
 * @}
 */

/* ---------------------------- Public Functions ---------------------------- */
/** @defgroup EXTI_Public_Functions EXTI Public Functions
 * @{
 */

/**
 * @brief Initializes the External Interrupt peripheral to a default state.
 *
 * Disables all four external interrupt lines (EINT0-EINT3) in the NVIC and resets the EXTMODE and
 * EXTPOLAR registers to their default Level-Sensitive and Active-Low configurations.
 */
void EXTI_Init(void);

/**
 * @brief Configures the physical pin and resistor mode for an EINT line.
 *
 * Assigns the EINT function to the corresponding hardware pin and establishes the electrical state
 * (Pull-up, Pull-down, or None) to ensure signal stability.
 *
 * @param line    The external interrupt line (EXTI_EINTx [0...3]).
 * @param resMode The resistor configuration (EXTI_PULLUP, EXTI_PULLDOWN, or EXTI_NOPULL).
 */
void EXTI_PinConfig(EXTI_LINE line, EXTI_RESISTOR resMode);

/**
 * @brief Configures the trigger mode and polarity for an external interrupt.
 *
 * Sets the EXTMODE (Level vs Edge) and EXTPOLAR (High/Low or Rising/Falling) registers. The
 * corresponding NVIC IRQ is disabled during the configuration process to prevent spurious interrupt
 * requests.
 *
 * @param extiCfg Pointer to an EXTI_CFG_T structure containing the settings.
 */
void EXTI_Config(const EXTI_CFG_T* extiCfg);

/**
 * @brief Configures and enables an external interrupt line in a single operation.
 *
 * Applies the mode and polarity settings through EXTI_Config and subsequently enables the
 * corresponding interrupt in the NVIC.
 *
 * @param extiCfg Pointer to an EXTI_CFG_T structure containing the settings.
 */
void EXTI_ConfigEnable(const EXTI_CFG_T* extiCfg);

/**
 * @brief Clears the pending flag for a specific external interrupt line.
 *
 * Writes a '1' to the corresponding bit in the EXTINT register. This action is required to
 * acknowledge the interrupt and allow subsequent triggers.
 *
 * @param line The external interrupt line (EXTI_EINTx [0...3]).
 */
void EXTI_ClearFlag(EXTI_LINE line);

/**
 * @brief Retrieves the current status of an external interrupt flag.
 *
 * Reads the EXTINT register to determine if a trigger event has occurred on the specified line.
 *
 * @param line The external interrupt line to check (EXTI_EINTx [0...3]).
 * @return SET if the interrupt is pending, RESET otherwise.
 */
FlagStatus EXTI_GetFlag(EXTI_LINE line);

/**
 * @brief Clears pending requests and enables the interrupt in the NVIC.
 *
 * Performs a safety clear of both the EXTI hardware flag and the NVIC pending state before enabling
 * the IRQ to prevent immediate triggering from stale events.
 *
 * @param line The external interrupt line to enable (EXTI_EINTx [0...3]).
 */
void EXTI_EnableIRQ(EXTI_LINE line);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  // LPC17XX_EXTI_H_

/**
 * @}
 */

/* ------------------------------ End Of File ------------------------------- */
