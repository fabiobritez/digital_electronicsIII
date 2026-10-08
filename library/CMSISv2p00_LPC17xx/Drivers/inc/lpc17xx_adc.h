/**
 * @file        lpc17xx_adc.h
 * @brief       Contains all macro definitions and function prototypes support for ADC firmware
 *              library on LPC17xx
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
/** @defgroup ADC ADC
 * @ingroup LPC1700CMSIS_FwLib_Drivers
 * @{
 */

#ifndef LPC17XX_ADC_H_
#define LPC17XX_ADC_H_

/* -------------------------------- Includes -------------------------------- */
#include <LPC17xx.h>
#include "lpc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------- Private Macros ----------------------------- */
/** @defgroup ADC_Private_Macros ADC Private Macros
 * @{
 */

/* ---------------------------- BIT DEFINITIONS ----------------------------- */
/**  Selects which of the AD0.0:7 pins is (are) to be sampled and converted */
#define ADC_CR_CH_SEL(n)           ((1UL << n))
/**  The APB clock (PCLK) is divided by (this value plus one) to produce the clock for the A/D */
#define ADC_CR_CLKDIV(n)           ((n << 8))
/**  Repeated conversions A/D enable bit */
#define ADC_CR_BURST               ((1UL << 16))
/**  ADC convert in power down mode */
#define ADC_CR_PDN                 ((1UL << 21))
/**  Start mask bits */
#define ADC_CR_START_MASK          ((7UL << 24))
/**  Select Start Mode */
#define ADC_CR_START_MODE_SEL(SEL) ((SEL << 24))
/**  Start conversion now */
#define ADC_CR_START_NOW           ((1UL << 24))
/**  Start conversion when the edge selected by bit 27 occurs on P2.10/EINT0 */
#define ADC_CR_START_EINT0         ((2UL << 24))
/** Start conversion when the edge selected by bit 27 occurs on P1.27/CAP0.1 */
#define ADC_CR_START_CAP01         ((3UL << 24))
/**  Start conversion when the edge selected by bit 27 occurs on MAT0.1 */
#define ADC_CR_START_MAT01         ((4UL << 24))
/**  Start conversion when the edge selected by bit 27 occurs on MAT0.3 */
#define ADC_CR_START_MAT03         ((5UL << 24))
/**  Start conversion when the edge selected by bit 27 occurs on MAT1.0 */
#define ADC_CR_START_MAT10         ((6UL << 24))
/**  Start conversion when the edge selected by bit 27 occurs on MAT1.1 */
#define ADC_CR_START_MAT11         ((7UL << 24))
/**  Start conversion on a falling edge on the selected CAP/MAT signal */
#define ADC_CR_EDGE                ((1UL << 27))

/* ----------------------- ADGDR REGISTER DEFINITIONS ----------------------- */
/** When DONE is 1, this field contains result value of ADC conversion */
#define ADC_GDR_RESULT(n)    (((n >> 4) & 0xFFF))
/** These bits contain the channel from which the LS bits were converted */
#define ADC_GDR_CH(n)        (((n >> 24) & 0x7))
/** This bit is 1 in burst mode if the results of one or more conversions was (were) lost */
#define ADC_GDR_OVERRUN_FLAG ((1UL << 30))
/** This bit is set to 1 when an A/D conversion completes */
#define ADC_GDR_DONE_FLAG    ((1UL << 31))

/** These bits are used to mask the channel selection bits in the GDR register */
#define ADC_GDR_CH_MASK ((7UL << 24))

/* ---------------------- ADINTEN REGISTER DEFINITIONS ---------------------- */
/** These bits allow control over which A/D channels generate interrupts for conversion completion
 */
#define ADC_INTEN_CH(n)  ((1UL << n))
/** When 1, enables the global DONE flag in ADDR to generate an interrupt */
#define ADC_INTEN_GLOBAL ((1UL << 8))

/* ----------------------- ADDRx REGISTER DEFINITIONS ----------------------- */
/** When DONE is 1, this field contains result value of ADC conversion */
#define ADC_DR_RESULT(n)    (((n >> 4) & 0xFFF))
/** These bits mirror the OVERRRUN status flags that appear in the
 * result register for each A/D channel */
#define ADC_DR_OVERRUN_FLAG ((1UL << 30))
/** This bit is set to 1 when an A/D conversion completes. It is cleared
 * when this register is read */
#define ADC_DR_DONE_FLAG    ((1UL << 31))

/* ---------------------- ADSTAT REGISTER DEFINITIONS ----------------------- */
/** These bits mirror the DONE status flags that appear in the result
 * register for each A/D channel */
#define ADC_STAT_CH_DONE_FLAG(n)    ((n & 0xFF))
/** These bits mirror the OVERRRUN status flags that appear in the
 * result register for each A/D channel */
#define ADC_STAT_CH_OVERRUN_FLAG(n) (((n >> 8) & 0xFF))
/** This bit is the A/D interrupt flag */
#define ADC_STAT_INT_FLAG           ((1UL << 16))

/* ----------------------- ADTRM REGISTER DEFINITIONS ----------------------- */
/** Offset trim bits for ADC operation */
#define ADC_ADCOFFS(n) (((n & 0xF) << 4))
/** Written to boot code*/
#define ADC_TRIM(n)    (((n & 0xF) << 8))

/* ------------------------ CHECK PARAM DEFINITIONS ------------------------- */
/** Check ADC parameter */
#define PARAM_ADCx(n) (((uint32_t*)n) == ((uint32_t*)LPC_ADC))

/** Check ADC rate parameter */
#define PARAM_ADC_RATE(rate) ((rate > 0) && (rate <= 200000))

/**
 * @}
 */

/* ------------------------------ Public Types ------------------------------ */
/** @defgroup ADC_Public_Types ADC Public Types
 * @{
 */

/**
 * @brief ADC channel selection.
 */
typedef enum {
    ADC_CHANNEL_0 = 0,
    ADC_CHANNEL_1,
    ADC_CHANNEL_2,
    ADC_CHANNEL_3,
    ADC_CHANNEL_4,
    ADC_CHANNEL_5,
    ADC_CHANNEL_6,
    ADC_CHANNEL_7
} ADC_CHANNEL;
/** Check ADC channel selection parameter. */
#define PARAM_ADC_CHANNEL(SEL) ((SEL) >= ADC_CHANNEL_0 && (SEL) <= ADC_CHANNEL_7)

/**
 * @brief ADC interrupt source selection.
 */
typedef enum {
    ADC_INT_CH0 = 0,
    ADC_INT_CH1,
    ADC_INT_CH2,
    ADC_INT_CH3,
    ADC_INT_CH4,
    ADC_INT_CH5,
    ADC_INT_CH6,
    ADC_INT_CH7,
    ADC_INT_GLOBAL
} ADC_INT_SOURCE;
/** Check ADC interrupt source selection parameter. */
#define PARAM_ADC_INT_SOURCE(SEL) ((SEL) >= ADC_INT_CH0 && (SEL) <= ADC_INT_GLOBAL)

/**
 * @brief ADC start option.
 */
typedef enum {
    ADC_START_CONTINUOUS = 0,
    ADC_START_NOW,
    ADC_START_ON_EINT0,
    ADC_START_ON_CAP01,
    ADC_START_ON_MAT01,
    ADC_START_ON_MAT03
} ADC_START_MODE;
/** Check ADC start option parameter. */
#define PARAM_ADC_START_MODE(MODE) ((MODE) >= ADC_START_CONTINUOUS && (MODE) <= ADC_START_ON_MAT03)

/**
 * @brief ADC edge selection for start conversion.
 */
typedef enum {
    ADC_START_ON_RISING = 0,
    ADC_START_ON_FALLING
} ADC_START_ON_EDGE;
/** Check ADC state parameter */
#define PARAM_ADC_START_ON_EDGE(EDGE) \
    ((EDGE == ADC_START_ON_RISING) || (EDGE == ADC_START_ON_FALLING))

/**
 * @brief ADC data status.
 */
typedef enum {
    ADC_DATA_OVERRUN = 0,
    ADC_DATA_DONE
} ADC_DATA_STATUS;
/** Check ADC state parameter */
#define PARAM_ADC_DATA_STATUS(OPT) ((OPT == ADC_DATA_OVERRUN) || (OPT == ADC_DATA_DONE))

/**
 * @}
 */

/* ---------------------------- Public Functions ---------------------------- */
/** @defgroup ADC_Public_Functions ADC Public Functions
 * @{
 */

/**
 * @brief  Initializes the ADC peripheral with a specific sampling rate.
 *
 * This function enables the ADC clock in the PCONP register, calculates the required clock divider
 * based on the peripheral clock (PCLK) to achieve the target rate, and powers up the converter.
 *
 * @note   A full conversion requires 65 ADC clock cycles. The maximum supported conversion rate for
 * the LPC176x is 200 kHz.
 *
 * @param  rate  Desired sampling rate in Hz (e.g., 100000 for 100kHz).
 */
void ADC_Init(uint32_t rate);

/**
 * @brief  De-initializes the ADC peripheral.
 *
 * Powers down the ADC by clearing the PDN bit in the ADCR register and disables the peripheral
 * clock to minimize power consumption.
 */
void ADC_DeInit(void);

/**
 * @brief  Configures the physical pin associated with an ADC channel.
 *
 * Sets the corresponding PINSEL register to enable the analog input function for the selected
 * channel and configures the PINMODE register to "Neither pull-up nor pull-down" (Tristate) to
 * prevent measurement interference.
 *
 * @param  channel  ADC channel to be configured (ADC_CHANNEL_x [0...7]).
 */
void ADC_PinConfig(ADC_CHANNEL channel);

/**
 * @brief  Enables the ADC Burst conversion mode.
 *
 * In Burst mode, the ADC performs repeated conversions on all selected channels. It is highly
 * recommended for applications requiring continuous monitoring of multiple analog signals without
 * manual software triggering.
 *
 * @note:
 * - START bits must be 000 before enabling burst mode.
 * - ADGINTEN bit must be disabled for burst mode operation.
 * - Burst mode starts conversions immediately.
 */
void ADC_BurstEnable(void);

/**
 * @brief  Disables the ADC Burst conversion mode.
 *
 * Switches the ADC back to software-controlled mode. Conversions will only occur when explicitly
 * triggered via a start command or external trigger.
 *
 * @note:
 * - Disabling burst mode stops conversions.
 */
void ADC_BurstDisable(void);

/**
 * @brief  Powers up the ADC converter.
 *
 * Sets the PDN bit in the ADCR register, bringing the ADC out of power-down mode to its operational
 * state.
 *
 * @note:
 * - Enable only after enabling the ADC clock.
 * - Does not enable the ADC clock.
 */
void ADC_PowerUp(void);

/**
 * @brief  Powers down the ADC converter.
 *
 * Clears the PDN bit in the ADCR register. This reduces the power consumption of the peripheral.
 *
 * @note:
 * - Disable before disabling the ADC clock.
 * - Does not disable the ADC clock.
 */
void ADC_PowerDown(void);

/**
 * @brief  Configures the start mode or trigger for the ADC conversion.
 *
 * This function clears the previous start bits and sets the new trigger mode. It supports starting
 * the conversion immediately (Software Start) or  synchronizing it with external hardware signals
 * such as EINT0, CAP, or MAT.
 *
 * @param  mode  Selected trigger source:
 *               - ADC_START_CONTINUOUS (burst mode)
 *               - ADC_START_NOW (manual start)
 *               - ADC_START_ON_EINT0
 *               - ADC_START_ON_CAP01
 *               - ADC_START_ON_MAT01
 *               - ADC_START_ON_MAT03
 *               - ADC_START_ON_MAT10
 *               - ADC_START_ON_MAT11
 */
void ADC_StartCmd(ADC_START_MODE mode);

/**
 * @brief  Selects an ADC channel for the next conversion sequence.
 *
 * Adds the specified channel to the bitmask of active channels in the ADCR register. Multiple
 * channels can be enabled simultaneously, especially useful in Burst mode.
 *
 * @param  channel  The ADC channel to enable (ADC_CHANNEL_x [0...7]).
 */
void ADC_ChannelEnable(ADC_CHANNEL channel);

/**
 * @brief  De-selects an ADC channel from the conversion sequence.
 *
 * Removes the specified channel from the active bitmask. If no channels remain selected, the ADC
 * will select the first channel (AD0.0) by default.
 *
 * @param  channel  The ADC channel to disable (ADC_CHANNEL_x [0...7]).
 */
void ADC_ChannelDisable(ADC_CHANNEL channel);

/**
 * @brief  Configures the trigger edge for external hardware starts.
 *
 * Determines whether the ADC conversion starts on the rising or falling edge of the selected
 * hardware trigger (e.g., CAP or MAT signals). This setting is only relevant when the start mode is
 * not set to "Now".
 *
 * @param  edge  The desired edge (ADC_START_ON_RISING or ADC_START_ON_FALLING).
 *
 * @note:
 * - This option is only effective when the start bits are set to [010...111]
 */
void ADC_EdgeStartConfig(ADC_START_ON_EDGE edge);

/**
 * @brief  Enables an interrupt source for the ADC.
 *
 * Configures the ADINTEN register to generate an interrupt when a specific channel completes its
 * conversion or when the global DONE flag is set.
 *
 * @param  source  The interrupt source to enable (ADC_INT_CHx [0...7] or ADC_INT_GLOBAL).
 *
 * @note:
 * - If ADC_INT_GLOBAL is selected, only the global DONE flag is enabled to generate an interrupt.
 * - Do not enable ADC_INT_GLOBAL if burst mode is enabled.
 */
void ADC_IntEnable(ADC_INT_SOURCE source);

/**
 * @brief  Disables an interrupt source for the ADC.
 *
 * Clears the corresponding bit in the ADINTEN register, preventing the peripheral from generating
 * interrupt requests for the specified source.
 *
 * @param  source  The interrupt source to disable (ADC_INT_CHx [0...7] or ADC_INT_GLOBAL).
 */
void ADC_IntDisable(ADC_INT_SOURCE source);

/**
 * @brief  Reads the global status of the most recent ADC conversion.
 *
 * Checks the Global Data Register (ADGDR) to determine if a conversion is complete or if an overrun
 * condition has occurred across any channel.
 *
 * @param  type  Status type to check (ADC_DATA_DONE or ADC_DATA_OVERRUN).
 * @return SET if the flag is active, RESET otherwise.
 */
FlagStatus ADC_GlobalGetStatus(ADC_DATA_STATUS type);

/**
 * @brief  Reads the status of a specific ADC channel.
 *
 * Accesses the individual channel data register (ADDR0-ADDR7) to check the completion or overrun
 * status for that specific input.
 *
 * @param  channel  The ADC channel to check (ADC_CHANNEL_x [0...7]).
 * @param  type     Status type (DONE or OVERRUN).
 * @return SET if the flag is active, RESET otherwise.
 */
FlagStatus ADC_ChannelGetStatus(ADC_CHANNEL channel, ADC_DATA_STATUS type);

/**
 * @brief  Retrieves the result of the last conversion from any channel.
 *
 * Returns the 12-bit digital value stored in the Global Data Register.
 *
 * @return The 12-bit conversion result (0 to 4095).
 *
 * @note:
 * - The returned value is right-aligned to bits [11:0].
 * - Reading this register automatically clears the DONE flag for that specific channel.
 */
uint16_t ADC_GlobalGetData(void);

/**
 * @brief  Retrieves the conversion result for a specific ADC channel.
 *
 * Returns the 12-bit digital value from the dedicated register of the selected channel.
 *
 * @param  channel  The ADC channel to read (ADC_CHANNEL_x [0...7]).
 * @return The 12-bit conversion result (0 to 4095).
 *
 * @note:
 * - The returned value is right-aligned to bits [11:0].
 * - Reading this register automatically clears the DONE flag for that specific channel.
 */
uint16_t ADC_ChannelGetData(ADC_CHANNEL channel);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LPC17XX_ADC_H_ */

/**
 * @}
 */

/* ------------------------------ End Of File ------------------------------- */
