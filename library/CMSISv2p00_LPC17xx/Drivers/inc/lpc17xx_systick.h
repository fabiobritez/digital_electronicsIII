/**
 * @file        lpc17xx_systick.h
 * @brief       Contains all macro definitions and function prototypes
 *              support for SYSTICK firmware library on LPC17xx
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
/** @defgroup SYSTICK SYSTICK
 * @ingroup LPC1700CMSIS_FwLib_Drivers
 * @{
 */

#ifndef LPC17XX_SYSTICK_H_
#define LPC17XX_SYSTICK_H_

/* -------------------------------- Includes -------------------------------- */
#include "LPC17xx.h"
#include "lpc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------- Private Macros ----------------------------- */
/** @defgroup SYSTICK_Private_Macros SYSTICK Private Macros
 * @{
 */

/** Enable SysTick Counter bit. */
#define ST_CTRL_ENABLE    ((uint32_t)(1 << 0))
/** Enable SysTick Interrupt bit. */
#define ST_CTRL_TICKINT   ((uint32_t)(1 << 1))
/** Clock Source selection bit. */
#define ST_CTRL_CLKSOURCE ((uint32_t)(1 << 2))
/** Count Flag bit. */
#define ST_CTRL_COUNTFLAG ((uint32_t)(1 << 16))
/** Maximum reload value for SysTick timer (24 bits). */
#define ST_MAX_LOAD       ((uint32_t)(0x00FFFFFF))
/** PCB bit position in PINSEL7 register. */
#define ST_PIN_PCB_POS    ((uint32_t)(20))

/**
 * @}
 */

/* ---------------------------- Public Functions ---------------------------- */
/** @defgroup SYSTICK_Public_Functions SYSTICK Public Functions
 * @{
 */

/**
 * @brief Initializes the SysTick timer using the internal processor clock.
 *
 * Calculates the required number of ticks based on the SystemCoreClock to
 * achieve a specific millisecond interval. It configures the LOAD register,
 * resets the current value, and selects the internal clock as the source.
 *
 * @param time Desired interval in milliseconds.
 *
 * @note        If the requested time exceeds the maximum possible interval for the SysTick timer,
 *              the LOAD register is set to its maximum value ST_MAX_LOAD, resulting in the
 *              longest possible timer interval.
 */
void SYSTICK_InternalInit(uint32_t time);

/**
 * @brief Initializes the SysTick timer using an external clock source.
 *
 * Configures the STCLK pin via PINSEL and calculates the LOAD value based on the provided external
 * frequency. This mode is used when a precise external reference is required or to save power by
 * bypassing the internal PLL.
 *
 * @param extFreq Frequency of the external clock source in Hz.
 * @param time    Desired interval in milliseconds.
 */
void SYSTICK_ExternalInit(uint32_t extFreq, uint32_t time);

/**
 * @brief Enables or disables the SysTick counter.
 *
 * Controls the ENABLE bit in the SysTick Control and Status Register (STCSR). When enabled, the
 * counter starts decrementing from the LOAD value.
 *
 * @param newState ENABLE to start the counter, DISABLE to stop it.
 */
void SYSTICK_Cmd(FunctionalState newState);

/**
 * @brief Enables or disables the SysTick exception request.
 *
 * Controls the TICKINT bit in the STCSR register. If enabled, the SysTick exception is generated
 * when the counter reaches zero.
 *
 * @param newState ENABLE to activate interrupts, DISABLE to deactivate them.
 */
void SYSTICK_IntCmd(FunctionalState newState);

/**
 * @brief Retrieves the current value of the SysTick counter.
 *
 * Reads the VAL register, which contains the current count of the
 * 24-bit down-counter.
 *
 * @return Current value of the 24-bit counter.
 */
static __INLINE uint32_t SYSTICK_GetCurrentValue(void) {
    return (SysTick->VAL);
}

/**
 * @brief Clears the SysTick Count Flag.
 *
 * Performs a dummy read of the CTRL register. According to the ARM Cortex-M3 architecture, reading
 * the SysTick Control and Status Register clears the COUNTFLAG bit to 0.
 */
static __INLINE void SYSTICK_ClearCounterFlag(void) {
    (void)SysTick->CTRL;
}

/**
 * @brief Retrieves the current reload (LOAD) value.
 *
 * Returns the 24-bit value currently programmed in the LOAD register, masked to ensure only valid
 * bits are returned.
 *
 * @return The 24-bit reload value.
 */
static __INLINE uint32_t SYSTICK_GetReloadValue(void) {
    return (SysTick->LOAD & ST_MAX_LOAD);
}

/**
 * @brief Sets a new reload value for the counter.
 *
 * Updates the LOAD register with a new 24-bit value. The new value takes effect the next time the
 * counter reaches zero or if the VAL register is manually cleared.
 *
 * @param reloadTicks The new 24-bit reload value (Max 0xFFFFFF).
 */
static __INLINE void SYSTICK_SetReloadValue(uint32_t reloadTicks) {
    SysTick->LOAD = (reloadTicks & ST_MAX_LOAD);
}

/**
 * @brief Checks if the SysTick counter is currently enabled.
 *
 * Verifies the status of the ENABLE bit in the CTRL register.
 *
 * @return SET if the counter is enabled, RESET otherwise.
 */
static __INLINE FlagStatus SYSTICK_IsActive(void) {
    return (SysTick->CTRL & ST_CTRL_ENABLE) ? SET : RESET;
}

/**
 * @brief Checks if the SysTick timer has counted to zero since the last read.
 *
 * Reads the COUNTFLAG bit in the CTRL register. This bit is set to 1 when the counter transitions
 * from 1 to 0.
 *
 * @return SET if the timer has fired, RESET otherwise.
 */
static __INLINE FlagStatus SYSTICK_HasFired(void) {
    return (SysTick->CTRL & ST_CTRL_COUNTFLAG) ? SET : RESET;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  // LPC17XX_SYSTICK_H_

/**
 * @}
 */

/* ------------------------------ End Of File ------------------------------- */
