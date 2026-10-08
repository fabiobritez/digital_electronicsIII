/**
 * @file        lpc17xx_pwm.h
 * @brief       Contains all macro definitions and function prototypes
 *              support for PWM firmware library on LPC17xx
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
/** @defgroup PWM PWM
 * @ingroup LPC1700CMSIS_FwLib_Drivers
 * @{
 */

#ifndef LPC17XX_PWM_H_
#define LPC17XX_PWM_H_

/* -------------------------------- Includes -------------------------------- */
#include "LPC17xx.h"
#include "lpc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------- Private Macros ----------------------------- */
/** @defgroup PWM_Private_Macros PWM Private Macros
 * @{
 */

/* ------------------------ MACROS MASKS DEFINITIONS ------------------------ */

/** CTCR register mode mask. */
#define PWM_CTCR_MODE_MASK  ((0x3))
/** CTCR register count input select mask. */
#define PWM_CTCR_INPUT_MASK ((0xC))

/* ------------------------- MACROS BIT DEFINITIONS ------------------------- */
/** CTCR register mode mask. */
#define PWM_CTCR_MODE_MASK          ((0x3))
/** PWM Capture input select */
#define PWM_CTCR_SELECT_INPUT(n)    ((uint32_t)((n) << 2))
/** IR register mask */
#define PWM_IR_BITMASK              ((uint32_t)(0x0000073F))
/** MCR interrupt on MATCHx bit. */
#define PWM_MCR_INT(MRx)            ((uint32_t)(1 << (3 * (MRx))))
/** MCR reset on MATCHx bit. */
#define PWM_MCR_RESET(MRx)          ((uint32_t)(1 << ((3 * (MRx)) + 1)))
/** MCR stop on MATCHx bit. */
#define PWM_MCR_STOP(MRx)           ((uint32_t)(1 << ((3 * (MRx)) + 2)))
/** MCR register channel mask bit. */
#define PWM_MCR_CHANNEL_MASKBIT(CH) ((uint32_t)(7 << (CH * 3)))
/** TCR counter enable bit. */
#define PWM_TCR_COUNTER_ENABLE      ((uint32_t)(1 << 0))
/** TCR counter reset bit. */
#define PWM_TCR_COUNTER_RESET       ((uint32_t)(1 << 1))
/** TCR PWM enable bit. */
#define PWM_TCR_PWM_ENABLE          ((uint32_t)(1 << 3))
/** PCR edge select bit. */
#define PWM_PCR_PWMSELn(CH)         ((uint32_t)(_BIT(CH)))
/** PCR PWM channel output enable bit. */
#define PWM_PCR_PWMENAn(CH)         ((uint32_t)((1) << ((CH) + 8)))
/** CCR register channel mask bit. */
#define PWM_CCR_CHANNEL_MASKBIT(CH) ((uint32_t)(7 << (CH * 3)))
/** CCR rising edge sensitive channel bit. */
#define PWM_CCR_CAP_RISING(CAPx)    ((uint32_t)(1 << (((CAPx & 0x2) << 1) + (CAPx & 0x1))))
/** CCR falling edge sensitive channel bit.*/
#define PWM_CCR_CAP_FALLING(CAPx)   ((uint32_t)(1 << (((CAPx & 0x2) << 1) + (CAPx & 0x1) + 1)))
/** CCR interrupt on event channel bit. */
#define PWM_CCR_INT_ON_CAP(CAPx)    ((uint32_t)(1 << (((CAPx & 0x2) << 1) + (CAPx & 0x1) + 2)))

/**
 * @}
 */

/* ------------------------------ Public Types ------------------------------ */
/** @defgroup PWM_Public_Types PWM Public Types
 * @{
 */

/**
 * @brief Timer/counter operating mode.
 */
typedef enum {
    PWM_TIMER_MODE = 0,
    PWM_COUNTER_MODE
} PWM_MODE;
/** Check PWM mode option parameter. */
#define PARAM_PWM_TIM_MODE(n) ((n >= PWM_TIMER_MODE) && (n <= PWM_COUNTER_MODE))

/**
 * @brief Counter mode edge selection.
 */
typedef enum {
    PWM_CTR_RISING = 1,
    PWM_CTR_FALLING,
    PWM_CTR_ANY
} PWM_CTR_EDGE;
/** Check PWM counter mode edge selection parameter. */
#define PARAM_PWM_CTR_EDGE(EDGE) ((EDGE) >= PWM_CTR_RISING && (EDGE) <= PWM_CTR_ANY)

/**
 * @brief Timer/Counter prescale option.
 */
typedef enum {
    PWM_TICK = 0,
    PWM_US
} PWM_PRESCALE;
/** Check PWM prescale option parameter. */
#define PARAM_PWM_PRESCALE(OPT) ((OPT == PWM_TICK) || (OPT == PWM_US))

/**
 * @brief Capture channel enum and parameter macro
 */
typedef enum {
    PWM_CAPTURE_0 = 0,
    PWM_CAPTURE_1
} PWM_CAPTURE;
/** Check PWM capture channel parameter. */
#define PARAM_PWM_CAPTURE(CH) (((CH) >= PWM_CAPTURE_0) && ((CH) <= PWM_CAPTURE_1))

/**
 *@brief PWM channel enum and parameter macro
 */
typedef enum {
    PWM_CHANNEL_1 = 1,
    PWM_CHANNEL_2,
    PWM_CHANNEL_3,
    PWM_CHANNEL_4,
    PWM_CHANNEL_5,
    PWM_CHANNEL_6
} PWM_CHANNEL;
/** Check PWM channel parameter. */
#define PARAM_PWM_CHANNEL(CH) ((CH) >= PWM_CHANNEL_1 && (CH) <= PWM_CHANNEL_6)

/**
 *@brief Match register enum and parameter macro
 */
typedef enum {
    PWM_MATCH_0 = 0,
    PWM_MATCH_1,
    PWM_MATCH_2,
    PWM_MATCH_3,
    PWM_MATCH_4,
    PWM_MATCH_5,
    PWM_MATCH_6
} PWM_MATCH_OPT;
/** Check PWM match register parameter. */
#define PARAM_PWM_MATCH_OPT(CH) ((CH) >= PWM_MATCH_0 && (CH) <= PWM_MATCH_6)

/**
 * @brief PWM operating mode options.
 */
typedef enum {
    PWM_SINGLE_EDGE = 0,
    PWM_DUAL_EDGE
} PWM_CHANNEL_EDGE;
/** Check PWM channel edge mode parameter. */
#define PARAM_PWM_CHANNEL_EDGE(n) ((n == PWM_SINGLE_EDGE) || (n == PWM_DUAL_EDGE))

/**
 * @brief PWM Interrupt status type.
 */
typedef enum {
    PWM_MR0_INT = 0,
    PWM_MR1_INT,
    PWM_MR2_INT,
    PWM_MR3_INT,
    PWM_CR0_INT,
    PWM_CR1_INT,
    PWM_MR4_INT = 8,
    PWM_MR5_INT,
    PWM_MR6_INT
} PWM_INT_TYPE;
/** Check PWM interrupt type parameter. */
#define PARAM_PWM_INT_TYPE(TYPE)                         \
    (((TYPE) >= PWM_MR0_INT && (TYPE) <= PWM_CR1_INT) || \
     ((TYPE) >= PWM_MR4_INT && (TYPE) <= PWM_MR6_INT))

/**
 * @brief PWM pin selection options.
 */
typedef enum {
    PWM1_P1_18,
    PWM1_P2_0,
    PWM2_P1_20,
    PWM2_P2_1,
    PWM2_P3_25,
    PWM3_P1_21,
    PWM3_P2_2,
    PWM3_P3_26,
    PWM4_P1_23,
    PWM4_P2_3,
    PWM5_P1_24,
    PWM5_P2_4,
    PWM6_P1_26,
    PWM6_P2_5,
} PWM_PIN_OPTION;
/** Check PWM pin option parameter. */
#define PARAM_PWM_PIN_OPTION(OPT) ((OPT >= PWM1_P1_18) && (OPT <= PWM6_P2_5))

/**
 * @brief PWM configuration structure for TIMER mode.
 */
typedef struct {
    PWM_PRESCALE prescaleOpt; /**< Should be:
                                - PWM_TICK: Absolute value.
                                - PWM_US  : Value in microseconds. */
    uint32_t prescaleValue;   /**< Prescale max value. */
} PWM_TIMERCFG_T;

/**
 * @brief PWM configuration structure for COUNTER mode.
 */
typedef struct {
    PWM_CAPTURE input; /**< Should be:
                        - PWM_CAPTURE_0 : CAPn.0 input pin for PWM timer.
                        - PWM_CAPTURE_1 : CAPn.1 input pin for PWM timer. */
    PWM_CTR_EDGE edge; /**< Should be:
                        - PWM_CTR_RISING  : Count rising edges on the selected capture input.
                        - PWM_CTR_FALLING : Count falling edges on the selected capture input.
                        - PWM_CTR_ANY     : Count both rising and falling edges on the selected
                        capture input. */
} PWM_COUNTERCFG_T;

/**
 * @brief Capture input configuration structure.
 */
typedef struct {
    PWM_CAPTURE channel;       /**< PWM_CAPTURE_x [0...1]. */
    FunctionalState risingEn;  /**< Should be:
                                - ENABLE  : Enable capture on rising edge.
                                - DISABLE : Disable capture on rising edge. */
    FunctionalState fallingEn; /**< Should be:
                                - ENABLE  : Enable capture on falling edge.
                                - DISABLE : Disable capture on falling edge. */
    FunctionalState intEn;     /**< Should be:
                                - ENABLE  : Enable interrupt on capture event.
                                - DISABLE : Disable interrupt on capture event. */
} PWM_CAPTURECFG_T;

/**
 * @brief Match channel configuration structure.
 */
typedef struct {
    PWM_MATCH_OPT channel;   /**< PWM_MATCH_x [0...6]. */
    FunctionalState intEn;   /**< Should be:
                                - ENABLE  : Enable interrupt on match.
                                - DISABLE : Disable interrupt on match. */
    FunctionalState stopEn;  /**< Should be:
                                - ENABLE  : Stop timer on match.
                                - DISABLE : Do not stop timer on match. */
    FunctionalState resetEn; /**< Should be:
                                - ENABLE  : Reset timer on match.
                                - DISABLE : Do not reset timer on match. */
    uint32_t matchValue;     /**< Match value to compare with timer counter. */
} PWM_MATCHCFG_T;

/**
 * @}
 */

/* ---------------------------- Public Functions ---------------------------- */
/** @defgroup PWM_Public_Functions PWM Public Functions
 * @{
 */

/**
 * @brief Initializes the PWM time-base in Timer Mode.
 *
 * Powers up the PWM1 peripheral and configures the Prescale Register (PR). It supports setting the
 * prescaler directly in clock ticks or by calculating the required ticks for a specific microsecond
 * interval.
 *
 * @param timerCfg Pointer to a PWM_TIMERCFG_T structure containing the prescale options and values.
 */
void PWM_InitTimer(const PWM_TIMERCFG_T* timerCfg);

/**
 * @brief Initializes the PWM in Counter Mode using an external input.
 *
 * Configures the Count Control Register (CTCR) to allow the PWM to increment based on edges from an
 * external capture input (CAP) rather than the internal peripheral clock.
 *
 * @param counterCfg Pointer to a PWM_COUNTERCFG_T structure specifying the input source and the
 * edge type.
 */
void PWM_InitCounter(const PWM_COUNTERCFG_T* counterCfg);

/**
 * @brief De-initializes the PWM1 peripheral.
 *
 * Disables the timer counter and the PWM mode by clearing the TCR register, then cuts the power to
 * the peripheral via the PCONP register.
 */
void PWM_DeInit(void);

/**
 * @brief Configures the physical pin for a specific PWM output.
 *
 * Maps one of the internal PWM1 channels to a physical pin on Port 1, Port 2, or Port 3. This
 * function sets the required function and resistor mode (Tristate) via the Pin Connect Block to
 * allow the PWM signal to reach the external hardware.
 *
 * @param option Selection from the available pin-to-channel mappings.
 */
void PWM_PinConfig(PWM_PIN_OPTION option);

/**
 * @brief Sets the output mode for a specific PWM channel.
 *
 * Configures the PWM Control Register (PCR) to select between Single-Edge or Double-Edge PWM mode.
 * In Single-Edge mode, the pulse always starts at the beginning of the period. In Double-Edge mode,
 * both the start and end of the pulse can be controlled using two match registers.
 *
 * @param channel  The PWM channel (PWM_CHANNEL_x [1...6]).
 * @param edgeMode The desired mode (PWM_SINGLE_EDGE or PWM_DOUBLE_EDGE).
 *
 * @note   Hardware fixes Channel 1 to Single-Edge mode; attempting
 * to change its edge mode will have no effect.
 */
void PWM_ChannelConfig(PWM_CHANNEL channel, PWM_CHANNEL_EDGE edgeMode);

/**
 * @brief Enables the master PWM mode.
 *
 * Sets the PWM Enable bit in the Timer Control Register (TCR). This allows the match registers to
 * control the PWM output pins according to the configuration in the PCR register.
 */
void PWM_Enable();

/**
 * @brief Disables the master PWM mode.
 *
 * Clears the PWM Enable bit in the TCR register. When disabled, the PWM match logic is inactive,
 * and the pins will not toggle based on match events.
 */
void PWM_Disable();

/**
 * @brief Activates the output for a specific PWM channel.
 *
 * Enables the specified PWM output pin by setting the corresponding bit in the PWM Control Register
 * (PCR). The pin will begin to output the signal defined by its match registers.
 *
 * @param channel The PWM channel to enable (PWM_CHANNEL_x [1...6]).
 */
void PWM_ChannelEnable(PWM_CHANNEL channel);

/**
 * @brief Deactivates the output for a specific PWM channel.
 *
 * Disables the specified PWM output pin. This function clears the corresponding bit in the PCR,
 * preventing the pin from toggling based on match events, while allowing the PWM logic to continue
 * running for other channels if enabled.
 *
 * @param channel The PWM channel to disable (PWM_CHANNEL_x [1...6]).
 */
void PWM_ChannelDisable(PWM_CHANNEL channel);

/**
 * @brief Enables the internal Timer Counter.
 *
 * Starts the counting logic for the PWM peripheral. The Timer Counter (TC) will begin incrementing
 * based on the peripheral clock and prescaler.
 */
void PWM_CounterEnable();

/**
 * @brief Disables the internal Timer Counter.
 *
 * Pauses the counting logic. The Timer Counter (TC) will hold its current value.
 */
void PWM_CounterDisable();

/**
 * @brief Resets the Timer Counter and Prescale Counter.
 *
 * Toggles the Reset bit in the TCR register to clear the TC and PC values to zero on the next
 * positive edge of the PCLK.
 */
void PWM_ResetCounter(void);

/**
 * @brief Configures the behavior of a specific Match Register.
 *
 * Sets the Match Control Register (MCR) to determine if a match event triggers an interrupt, resets
 * the Timer Counter, or stops the timer. It also performs an initial update of the match value.
 *
 * @param pwmMatchCfg Pointer to a PWM_MATCHCFG_T structure containing configuration and initial
 * match value.
 */
void PWM_ConfigMatch(const PWM_MATCHCFG_T* pwmMatchCfg);

/**
 *
 * Writes a new value to the specified Match Register (MR0-MR6) and sets the corresponding bit in
 * the Latch Enable Register (LER). The new value will take effect at the next PWM period start
 * (when TC resets).
 *
 * @param match         The Match Register to update (PWM_MATCH_x [0...6]).
 * @param matchValue The new 32-bit value for the Match Register.
 */
void PWM_MatchUpdateSingle(PWM_MATCH_OPT match, uint32_t matchValue);

/**
 * @brief Updates two Match Register values simultaneously for double-edge mode.
 *
 * Writes new values to two Match Registers and enables their latches in a single operation. This is
 * required for double-edge PWM to ensure both the leading and trailing edges are updated
 * synchronously.
 *
 * @param matchA    The first Match Register (PWM_MATCH_x [0...6]).
 * @param newValueA The new value for matchA.
 * @param matchB    The second Match Register (PWM_MATCH_x [0...6]).
 * @param newValueB The new value for matchB.
 */
void PWM_MatchUpdateDouble(PWM_MATCH_OPT matchA, uint32_t newValueA, PWM_MATCH_OPT matchB,
                           uint32_t newValueB);

/**
 * @brief Clears a pending interrupt flag in the PWM peripheral.
 *
 * Writes a '1' to the corresponding bit in the Interrupt Register (IR). This acknowledges the event and allows the generation of subsequent interrupt requests.
 *
 * @param intFlag The interrupt source to clear (PWM_xxx_INT).
 */
void PWM_ClearIntPending(PWM_INT_TYPE intFlag);

/**
 * @brief Retrieves the current status of a PWM interrupt flag.
 *
 * Checks the Interrupt Register (IR) to determine if a specific match or capture event has triggered an interrupt request.
 *
 * @param intFlag The interrupt source to check (PWM_xxx_INT).
 * @return SET if the interrupt is pending, RESET otherwise.
 */
FlagStatus PWM_GetIntStatus(PWM_INT_TYPE intFlag);

/**
 * @brief Configures the capture logic for external signal timing.
 *
 * Sets the Capture Control Register (CCR) to define which edges (rising, falling, or both) on the selected CAP input will load the Timer Counter (TC) value into a capture register and whether an interrupt is generated.
 *
 * @param capCfg Pointer to a PWM_CAPTURECFG_T structure with capture settings.
 */
void PWM_ConfigCapture(const PWM_CAPTURECFG_T* capCfg);

/**
 * @brief Reads the captured value from a specific capture register.
 *
 * Returns the 32-bit Timer Counter value that was latched into the
 * Capture Register (CR0 or CR1) during the last valid edge event.
 *
 * @param capChannel The capture channel to read (PWM_CAPTURE_0 or PWM_CAPTURE_1).
 * @return The captured 32-bit Timer Counter value.
 */
uint32_t PWM_GetCaptureValue(PWM_CAPTURE capChannel);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LPC17XX_PWM_H_ */

/**
 * @}
 */

/* ------------------------------ End Of File ------------------------------- */
