/**
 * @file        lpc17xx_dac.h
 * @brief       Contains all macro definitions and function prototypes
 *              support for DAC firmware library on LPC17xx
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
 * Last update: 22/02/2026, Author: David Trujillo Medina
 */

/* ---------------------------- Peripheral group ---------------------------- */
/** @defgroup DAC DAC
 * @ingroup LPC1700CMSIS_FwLib_Drivers
 * @{
 */

#ifndef LPC17XX_DAC_H_
#define LPC17XX_DAC_H_

/* -------------------------------- Includes -------------------------------- */
#include "LPC17xx.h"
#include "lpc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------- Private Macros ----------------------------- */
/** @defgroup DAC_Private_Macros DAC Private Macros
 * @{
 */

/** After the selected settling time after this field is written with a
new VALUE, the voltage on the AOUT pin (with respect to VSSA)
is VALUE/1024 × VREF */
#define DAC_VALUE(n)      ((uint32_t)((n & 0x3FF) << 6))
/** If this bit = 0: The settling time of the DAC is 1 microsecond max,
 * and the maximum current is 700 microAmpere
 * If this bit = 1: The settling time of the DAC is 2.5 microsecond
 * and the maximum current is 350 microAmpere */
#define DAC_BIAS_EN       ((uint32_t)(1 << 16))
/** Value to reload interrupt DMA counter */
#define DAC_CCNT_VALUE(n) ((uint32_t)(n & 0xffff))

/** DCAR double buffering */
#define DAC_DBLBUF_ENA   ((uint32_t)(1 << 1))
/** DCAR Time out count enable */
#define DAC_CNT_ENA      ((uint32_t)(1 << 2))
/** DCAR DMA access */
#define DAC_DMA_ENA      ((uint32_t)(1 << 3))
/** DCAR DACCTRL mask bit */
#define DAC_DACCTRL_MASK ((uint32_t)(0x0F))

/** Macro to determine if it is valid DAC peripheral */
#define PARAM_DACx(n) (((uint32_t*)n) == ((uint32_t*)LPC_DAC))

/**
 * @}
 */

/* ------------------------------ Public Types ------------------------------ */
/** @defgroup DAC_Public_Types DAC Public Types
 * @{
 */

/**
 * @brief DAC current options for bias configuration.
 */
typedef enum {
    DAC_700uA = 0, /**< Settling time: 1 us max, max current: 700 uA */
    DAC_350uA      /**< Settling time: 2.5 us max, max current: 350 uA */
} DAC_MAX_CURRENT;
/** Check DAC max current option parameter. */
#define PARAM_DAC_MAX_CURRENT(MAX) ((MAX == DAC_700uA) || (MAX == DAC_350uA))

/**
 * @brief DAC converter control configuration.
 *
 * Used to configure double buffering, timeout counter, and DMA access for the DAC.
 */
typedef struct {
    FunctionalState doubleBuffer; /**< ENABLE or DISABLE. */
    FunctionalState dmaCounter;   /**< ENABLE or DISABLE. */
    FunctionalState dmaRequest;   /**< ENABLE or DISABLE. */
} DAC_CONVERTER_CFG_T;

/**
 * @}
 */

/* ---------------------------- Public Functions ---------------------------- */
/**
 * @brief  Initializes the DAC peripheral and its associated pin.
 *
 * Configures P0.26 as AOUT (Function 10) and sets the pin mode to "Neither pull-up nor pull-down".
 * It establishes the peripheral clock (PCLK) at CCLK/4 and sets the initial settling time to the
 * 700 uA (1 MHz) mode and output value to 0.
 *
 * @note:
 * - The DAC pin is configured for analog output.
 * - The DAC is set to maximum current by default.
 * - Call this function before using other DAC functions.
 */
void DAC_Init(void);

/**
 * @brief  Updates the analog output value.
 *
 * Writes a 10-bit value to the VALUE field (bits 6 to 15) of the DACR register. The output voltage
 * is calculated as: Vout = (VALUE / 1024) * VREFP.
 *
 * @param  newValue  10-bit digital value (0 to 1023).
 */
void DAC_UpdateValue(uint32_t newValue);

/**
 * @brief  Sets the maximum update rate and settling time.
 *
 * Controls the BIAS bit in the DACR register to switch between two power modes:
 * - DAC_700uA: Maximum update rate of 1 MHz (Settling time < 1 us).
 * - DAC_350uA: Maximum update rate of 400 kHz (Settling time < 2.5 us).
 *
 * @param  maxCurr  Power mode selection (DAC_700uA or DAC_350uA).
 */
void DAC_SetBias(DAC_MAX_CURRENT maxCurr);

/**
 * @brief  Configures the DAC control register for advanced features.
 *
 * Sets the operational modes for the DAC converter, including double buffering, timeout counter
 * activation, and DMA request generation.
 *
 * @param  dacCfg  Pointer to a DAC_CONVERTER_CFG_T structure containing the settings.
 */
void DAC_ConfigDAConverterControl(const DAC_CONVERTER_CFG_T* dacCfg);

/**
 * @brief  Sets the reload value for the DAC timeout counter.
 *
 * Configures the 16-bit DACCNTVAL register. This value determines the time interval between DMA
 * requests or hardware updates when the counter is enabled in the control register.
 *
 * @param  timeOut  16-bit timeout value in PCLK cycles.
 *
 * @note:
 * - Use this function to configure DMA or interrupt timing.
 * - If timeOut is more than 16 bits, only the lower 16 bits are used.
 */
void DAC_SetDMATimeOut(uint16_t timeOut);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LPC17XX_DAC_H_ */

/**
 * @}
 */

/* ------------------------------ End Of File ------------------------------- */
