/**
 * @file        lpc17xx_timer.h
 * @brief       Contains all functions support for Timer firmware library on LPC17xx
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
/** @defgroup TIM TIM
 * @ingroup LPC1700CMSIS_FwLib_Drivers
 * @{
 */

#ifndef LPC17XX_TIMER_H_
#define LPC17XX_TIMER_H_

/* -------------------------------- Includes -------------------------------- */
#include "LPC17xx.h"
#include "lpc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------- Private Macros ----------------------------- */
/** @defgroup TIM_Private_Macros TIM Private Macros
 * @{
 */
/* ------------------------ MACROS MASKS DEFINITIONS ------------------------ */
/** TCR register mask. */
#define TIM_TCR_MASKBIT            ((uint32_t)(3))
/** MCR register mask. */
#define TIM_MCR_MASKBIT            ((uint32_t)(0x0FFF))
/** MCR register channel mask bit. */
#define TIM_MCR_CHANNEL_MASKBIT(n) ((uint32_t)(7UL << (n * 3)))
/** CCR register mask. */
#define TIM_CCR_MASKBIT            ((uint32_t)(0x3F))
/** CCR register channel mask bit. */
#define TIM_CCR_CHANNEL_MASKBIT(n) ((uint32_t)(7 << (n * 3)))
/** EMR register mask. */
#define TIM_EMR_MASKBIT            ((0x0FFF))
/** EMR register channel external control mask bit. */
#define TIM_EMR_MASK(n)            (_SBF(((n << 1) + 4), 0x03))
/** CTCR register mask. */
#define TIM_CTCR_MASKBIT           ((0xF))
/** CTCR register mode mask. */
#define TIM_CTCR_MODE_MASK         ((0x3))
/** CTCR register count input select mask. */
#define TIM_CTCR_INPUT_MASK        ((0xC))

/* ------------------------- MACROS BIT DEFINITIONS ------------------------- */
/** Timer/counter enable bit. */
#define TIM_ENABLE            ((uint32_t)(1 << 0))
/** Timer/counter reset bit. */
#define TIM_RESET             ((uint32_t)(1 << 1))
/** Interrupt flag bit. */
#define TIM_IR_CLR(n)         (_BIT(n))
/** Interrupt flag bit for match register n. */
#define TIM_MATCH_INT(n)      (_BIT(n & 0x0F))
/** Interrupt flag bit for capture register n. */
#define TIM_CAP_INT(n)        (_BIT(((n & 0x0F) + 4)))
/** Interrupt on match for match register n. */
#define TIM_INT_ON_MATCH(n)   (_BIT((n * 3)))
/** Reset on match for match register n. */
#define TIM_RESET_ON_MATCH(n) (_BIT(((n * 3) + 1)))
/** Stop on match for match register n. */
#define TIM_STOP_ON_MATCH(n)  (_BIT(((n * 3) + 2)))
/** Capture channel n on CRx rising edge. */
#define TIM_CAP_RISING(n)     (_BIT((n * 3)))
/** Capture channel n on CRx falling edge. */
#define TIM_CAP_FALLING(n)    (_BIT(((n * 3) + 1)))
/** Interrupt on capture channel n. */
#define TIM_INT_ON_CAP(n)     (_BIT(((n * 3) + 2)))
/** Mask bit for rising and falling edge bit. */
#define TIM_EDGE_MASK(n)      (_SBF((n * 3), 0x03))
/** Mask bit for match channel n. */
#define TIM_EM(n)             (_BIT(n))
/** Output state change of MAT.n when external match happens: no change. */
#define TIM_EM_NOTHING        ((uint8_t)(0x0))
/** Output state change of MAT.n when external match happens: low. */
#define TIM_EM_LOW            ((uint8_t)(0x1))
/** Output state change of MAT.n when external match happens: high. */
#define TIM_EM_HIGH           ((uint8_t)(0x2))
/** Output state change of MAT.n when external match happens: toggle. */
#define TIM_EM_TOGGLE         ((uint8_t)(0x3))
/** Macro for setting for the MAT.n change state bits. */
#define TIM_EM_SET(n, s)      (_SBF(((n << 1) + 4), (s & 0x03)))

/* ---------------------- CHECK PARAMETER DEFINITIONS ----------------------- */
/** Check TIMER parameter. */
#define PARAM_TIMx(n)                                                                      \
    (((uintptr_t)(n) == (uintptr_t)LPC_TIM0) || ((uintptr_t)(n) == (uintptr_t)LPC_TIM1) || \
     ((uintptr_t)(n) == (uintptr_t)LPC_TIM2) || ((uintptr_t)(n) == (uintptr_t)LPC_TIM3))

/**
 * @}
 */

/* ------------------------------ Public Types ------------------------------ */
/** @defgroup TIM_Public_Types TIM Public Types
 * @{
 */

/**
 * @brief interrupt type.
 */

/**
 * @brief Timer/counter operating mode.
 */
typedef enum {
    TIM_TIMER_MODE = 0,
    TIM_COUNTER_MODE
} TIM_MODE;
/** Check TIMER mode option parameter. */
#define PARAM_TIM_MODE(MODE) ((MODE) >= TIM_TIMER_MODE && (MODE) <= TIM_COUNTER_MODE)

/**
 * @brief Counter mode edge selection.
 */
typedef enum {
    TIM_CTR_RISING = 1,
    TIM_CTR_FALLING,
    TIM_CTR_ANY
} TIM_CTR_EDGE;
/** Check TIMER counter mode edge selection parameter. */
#define PARAM_TIM_CTR_EDGE(EDGE) ((EDGE) >= TIM_CTR_RISING && (EDGE) <= TIM_CTR_ANY)

/**
 * @brief Timer/Counter prescale option.
 */
typedef enum {
    TIM_TICK = 0,
    TIM_US
} TIM_PRESCALE;
/** Check TIMER prescale option parameter. */
#define PARAM_TIM_PRESCALE(PS) ((PS == TIM_TICK) || (PS == TIM_US))

/**
 * @brief Capture channel enum and parameter macro
 */
typedef enum {
    TIM_CAPTURE_0 = 0,
    TIM_CAPTURE_1
} TIM_CAPTURE_CH;
/** Check TIMER capture channel option parameter. */
#define PARAM_TIM_CAPTURE_CH(CH) (((CH) >= TIM_CAPTURE_0) && ((CH) <= TIM_CAPTURE_1))

/**
 * @brief Timer/counter capture mode options.
 */
typedef enum {
    TIM_CAP_NONE = 0,
    TIM_CAP_RISING,
    TIM_CAP_FALLING,
    TIM_CAP_ANY
} TIM_CAP_MODE;
/** Check TIMER capture mode option parameter. */
#define PARAM_TIM_CAP_MODE(MODE) ((MODE) >= TIM_CAP_NONE && (MODE) <= TIM_CAP_ANY)

/**
 *@brief Match channel enum and parameter macro
 */
typedef enum {
    TIM_MATCH_0 = 0,
    TIM_MATCH_1,
    TIM_MATCH_2,
    TIM_MATCH_3
} TIM_MATCH_CH;
/** Check TIMER match channel option parameter. */
#define PARAM_TIM_MATCH_CH(CH) ((CH) >= TIM_MATCH_0 && (CH) <= TIM_MATCH_3)

/**
 * @brief Timer/Counter external match option.
 */
typedef enum {
    TIM_NOTHING = 0,
    TIM_LOW,
    TIM_HIGH,
    TIM_TOGGLE
} TIM_EXTMATCH_OPT;
/** Check TIMER external match option parameter. */
#define PARAM_TIM_EXTMATCH_OPT(EM) ((EM) >= TIM_NOTHING && (EM) <= TIM_TOGGLE)

typedef enum {
    TIM_MR0_INT = 0,
    TIM_MR1_INT,
    TIM_MR2_INT,
    TIM_MR3_INT,
    TIM_CR0_INT,
    TIM_CR1_INT
} TIM_INT;
/** Check TIMER interrupt type parameter. */
#define PARAM_TIM_INT(INT) ((INT) >= TIM_MR0_INT && (INT) <= TIM_CR1_INT)

/**
 * @brief Timer pin selection options.
 */
typedef enum {
    TIM_CAP0_0_P1_26 = 0,
    TIM_CAP0_1_P1_27,
    TIM_MAT0_0_P1_28,
    TIM_MAT0_0_P3_25,
    TIM_MAT0_1_P1_29,
    TIM_MAT0_1_P3_26,
    TIM_CAP1_0_P1_18,
    TIM_CAP1_1_P1_19,
    TIM_MAT1_0_P1_22,
    TIM_MAT1_1_P1_25,
    TIM_CAP2_0_P0_4,
    TIM_CAP2_1_P0_5,
    TIM_MAT2_0_P0_6,
    TIM_MAT2_0_P4_28,
    TIM_MAT2_1_P0_7,
    TIM_MAT2_1_P4_29,
    TIM_MAT2_2_P0_8,
    TIM_MAT2_3_P0_9,
    TIM_CAP3_0_P0_23,
    TIM_CAP3_1_P0_24,
    TIM_MAT3_0_P0_10,
    TIM_MAT3_1_P0_11
} TIM_PIN_OPTION;
/** Check TIMER pin option parameter. */
#define PARAM_TIM_PIN_OPTION(OPT) ((OPT >= TIM_CAP0_0_P1_26) && (OPT <= TIM_MAT3_1_P0_11))

/**
 * @brief Timer configuration structure for TIMER mode.
 */
typedef struct {
    TIM_PRESCALE prescaleOpt; /**< Should be:
                                - TIM_TICK: Absolute value.
                                - TIM_US  : Value in microseconds. */
    uint32_t prescaleValue;   /**< Prescale max value. */
} TIM_TIMERCFG_T;

/**
 * @brief Timer configuration structure for COUNTER mode.
 */
typedef struct {
    TIM_CAPTURE_CH input; /**< Should be:
                            - TIM_CAPTURE_0 : CAPn.0 input pin for TIMERn.
                            - TIM_CAPTURE_1 : CAPn.1 input pin for TIMERn. */
    TIM_CTR_EDGE edge;    /**< Should be:
                            - TIM_CTR_RISING  : Count rising edges on the selected capture input.
                            - TIM_CTR_FALLING : Count falling edges on the selected capture input.
                            - TIM_CTR_ANY     : Count both rising and falling edges on the selected
                            capture input. */
} TIM_COUNTERCFG_T;

/**
 * @brief Capture input configuration structure.
 */
typedef struct {
    TIM_CAPTURE_CH channel;    /**< TIM_CAPTURE_x [0...1]. */
    FunctionalState risingEn;  /**< Should be:
                                        - ENABLE  : Enable capture on rising edge.
                                        - DISABLE : Disable capture on rising edge. */
    FunctionalState fallingEn; /**< Should be:
                                        - ENABLE  : Enable capture on falling edge.
                                        - DISABLE : Disable capture on falling edge. */
    FunctionalState intEn;     /**< Should be:
                                        - ENABLE  : Enable interrupt on capture event.
                                        - DISABLE : Disable interrupt on capture event. */
} TIM_CAPTURECFG_T;

/**
 * @brief Match channel configuration structure.
 */
typedef struct {
    TIM_MATCH_CH channel;    /**< TIM_MATCH_x [0...3]. */
    FunctionalState intEn;   /**< Should be:
                                - ENABLE  : Enable interrupt on match.
                                - DISABLE : Disable interrupt on match. */
    FunctionalState stopEn;  /**< Should be:
                                - ENABLE  : Stop timer on match.
                                - DISABLE : Do not stop timer on match. */
    FunctionalState resetEn; /**< Should be:
                                - ENABLE  : Reset timer on match.
                                - DISABLE : Do not reset timer on match. */
    TIM_EXTMATCH_OPT extOpt; /**< Should be:
                                - TIM_NOTHING : Do nothing for external output pin if matched.
                                - TIM_LOW     : Force external output pin to low if matched.
                                - TIM_HIGH    : Force external output pin to high if matched.
                                - TIM_TOGGLE  : Toggle external output pin if matched. */
    uint32_t matchValue;     /**< Match value to compare with timer counter. */
} TIM_MATCHCFG_T;

/**
 * @}
 */

/* ---------------------------- Public Functions ---------------------------- */
/** @defgroup TIM_Public_Functions TIM Public Functions
 * @{
 */

/**
 * @brief Initializes the timer in Timer Mode.
 *
 * Powers up the selected timer (TIM0-TIM3) and configures the Prescale Register (PR). It supports
 * setting the prescaler in raw clock ticks or calculating the equivalent value for a specific
 * microsecond interval based on the peripheral clock.
 *
 * @param TIMx     Pointer to the timer peripheral (LPC_TIMx [0...3]).
 * @param timerCfg Pointer to a TIM_TIMERCFG_T structure with prescale settings.
 */
void TIM_InitTimer(LPC_TIM_TypeDef* TIMx, const TIM_TIMERCFG_T* timerCfg);

/**
 * @brief Initializes the timer in Counter Mode.
 *
 * Configures the Count Control Register (CTCR) to increment the Timer Counter (TC) based on
 * transitions on an external capture input pin instead of the internal peripheral clock.
 *
 * @param TIMx       Pointer to the timer peripheral (LPC_TIMx [0...3]).
 * @param counterCfg Pointer to a TIM_COUNTERCFG_T structure specifying the input channel and edge
 * type.
 */
void TIM_InitCounter(LPC_TIM_TypeDef* TIMx, const TIM_COUNTERCFG_T* counterCfg);

/**
 * @brief De-initializes the timer peripheral.
 *
 * Stops the timer by clearing the TCR register and disables the peripheral clock in the PCONP
 * register to minimize power consumption.
 *
 * @param TIMx Pointer to the timer peripheral to disable (LPC_TIMx [0...3]).
 */
void TIM_DeInit(LPC_TIM_TypeDef* TIMx);

/**
 * @brief Starts the timer counter.
 *
 * Sets the Counter Enable bit in the Timer Control Register (TCR). The Timer Counter (TC) will
 * begin incrementing on every PCLK or external edge, depending on the initialized mode.
 *
 * @param TIMx Pointer to the timer peripheral (LPC_TIMx [0...3]).
 */
void TIM_Enable(LPC_TIM_TypeDef* TIMx);

/**
 * @brief Stops the timer counter.
 *
 * Clears the Counter Enable bit in the TCR register. The Timer Counter (TC) will hold its current
 * value until enabled again or reset.
 *
 * @param TIMx Pointer to the timer peripheral (LPC_TIMx [0...3]).
 */
void TIM_Disable(LPC_TIM_TypeDef* TIMx);

/**
 * @brief Retrieves the current value of the Timer Counter (TC).
 *
 * Reads the 32-bit TC register of the specified timer peripheral, which contains the current count
 * value.
 *
 * @param TIMx Pointer to the timer peripheral (LPC_TIMx [0...3]).
 * @return Current 32-bit counter value.
 */
uint32_t TIM_ReadTimer(LPC_TIM_TypeDef* TIMx);

/**
 * @brief Retrieves the current value of the Prescale Counter (PC).
 *
 * Reads the 32-bit PC register of the specified timer peripheral, which contains the current value
 * of the prescale counter.
 *
 * @param TIMx Pointer to the timer peripheral (LPC_TIMx [0...3]).
 * @return Current 32-bit prescale counter value.
 */
uint32_t TIM_ReadPrescale(LPC_TIM_TypeDef* TIMx);

/**
 * @brief Resets the Timer Counter and Prescale Counter.
 *
 * Toggles the Reset bit in the Timer Control Register (TCR). This synchronously clears both the TC
 * and PC to zero on the next positive edge of PCLK.
 *
 * @param TIMx Pointer to the timer peripheral (LPC_TIMx [0...3]).
 */
void TIM_ResetCounter(LPC_TIM_TypeDef* TIMx);

/**
 * @brief Configures the match logic and external output for a timer channel.
 *
 * Sets the Match Control Register (MCR) to define hardware actions (Interrupt, Reset, or Stop) upon
 * a match event. It also updates the match value and configures the External Match Register (EMR)
 * to control physical MATn.x pins.
 *
 * @param TIMx     Pointer to the timer peripheral (LPC_TIMx [0...3]).
 * @param matchCfg Pointer to a TIM_MATCHCFG_T structure with match settings.
 */
void TIM_ConfigMatch(LPC_TIM_TypeDef* TIMx, const TIM_MATCHCFG_T* matchCfg);

/**
 * @brief Updates the value of a specific Match Register.
 *
 * Writes a new 32-bit value to the selected Match Register (MR0-MR3). It also clears the
 * corresponding interrupt flag to prevent immediate triggering if the new value is equal to the
 * current counter.
 *
 * @param TIMx       Pointer to the timer peripheral (LPC_TIMx [0...3]).
 * @param channel    The match channel (TIM_MATCH_x [0...3]).
 * @param matchValue The new 32-bit match value.
 */
void TIM_UpdateMatchValue(LPC_TIM_TypeDef* TIMx, TIM_MATCH_CH channel, uint32_t matchValue);

/**
 * @brief Configures the behavior of the external match pins (MATn.x).
 *
 * Sets the External Match Register (EMR) to define how the physical pin reacts when a match occurs.
 * Options include Do Nothing, Clear, Set, or Toggle the pin.
 *
 * @param TIMx    Pointer to the timer peripheral (LPC_TIMx [0...3]).
 * @param channel The match channel associated with the MAT pin (TIM_MATCH_x [0...3]).
 * @param type    The external match operation (TIM_NOTHING, TIM_LOW, TIM_HIGH, TIM_TOGGLE).
 */
void TIM_SetMatchExt(LPC_TIM_TypeDef* TIMx, TIM_MATCH_CH channel, TIM_EXTMATCH_OPT type);

/**
 * @brief Configures the capture logic for external signal timing.
 *
 * Sets the Capture Control Register (CCR) to define which edges (Rising, Falling, or Both) on a
 * CAPn.x input will cause the Timer Counter (TC) to be loaded into a Capture Register (CRx).
 *
 * @param TIMx   Pointer to the timer peripheral (LPC_TIMx [0...3]).
 * @param capCfg Pointer to a TIM_CAPTURECFG_T structure with capture settings.
 */
void TIM_ConfigCapture(LPC_TIM_TypeDef* TIMx, const TIM_CAPTURECFG_T* capCfg);

/**
 * @brief Retrieves the last value stored in a Capture Register.
 *
 * Returns the 32-bit value latched in CR0 or CR1 during the last valid edge transition on the
 * associated capture pin.
 *
 * @param TIMx    Pointer to the timer peripheral (LPC_TIMx [0...3]).
 * @param channel The capture channel to read (TIM_CAPTURE_0 or TIM_CAPTURE_1).
 * @return The 32-bit captured Timer Counter value.
 */
uint32_t TIM_GetCaptureValue(LPC_TIM_TypeDef* TIMx, TIM_CAPTURE_CH channel);

/**
 * @brief Clears a pending interrupt flag for a timer event.
 *
 * Writes a '1' to the corresponding bit in the Interrupt Register (IR). This acknowledges match or
 * capture events within the ISR.
 *
 * @param TIMx    Pointer to the timer peripheral (LPC_TIMx [0...3]).
 * @param intFlag The interrupt source to clear (TIM_xxx_INT).
 */
void TIM_ClearIntPending(LPC_TIM_TypeDef* TIMx, TIM_INT intFlag);

/**
 * @brief Retrieves the status of a specific timer interrupt flag.
 *
 * @param TIMx    Pointer to the timer peripheral (LPC_TIMx [0...3]).
 * @param intFlag The interrupt source to check (TIM_xxx_INT).
 * @return SET if the interrupt is pending, RESET otherwise.
 */
FlagStatus TIM_GetIntStatus(LPC_TIM_TypeDef* TIMx, TIM_INT intFlag);

/**
 * @brief Configures the physical pin for a specific Timer function.
 *
 * Maps a Timer function (Capture or Match) to a physical pin on the MCU using a lookup table of the
 * available hardware mappings.
 *
 * @param option Selection from the available TIM_PIN_OPTION values.
 */
void TIM_PinConfig(TIM_PIN_OPTION option);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* LPC17XX_TIMER_H_ */

/**
 * @}
 */

/* ------------------------------ End Of File ------------------------------- */
