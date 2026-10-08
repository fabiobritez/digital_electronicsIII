/**
 * @file        lpc17xx_gpio.h
 * @brief       Contains all macro definitions and function prototypes
 *              support for GPIO firmware library on LPC17xx
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
/** @defgroup GPIO GPIO
 * @ingroup LPC1700CMSIS_FwLib_Drivers
 * @{
 */

#ifndef LPC17XX_GPIO_H_
#define LPC17XX_GPIO_H_

/* -------------------------------- Includes -------------------------------- */
#include "LPC17xx.h"
#include "lpc17xx_common.h"
#include "lpc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------- Private Macros ----------------------------- */
/** @defgroup GPIO_Private_Macros GPIO Private Macros
 * @{
 */

/* ------------------------ MACROS MASKS DEFINITIONS ------------------------ */
/** Fast GPIO pin mask definition. */
#define GPIO_PIN_MASK ((0x1UL))

/* -------------------- MACROS BYTE POINTER DEFINITIONS --------------------- */
/** Fast GPIO port 0 byte accessible definition. */
#define GPIO0_Byte ((GPIO_Byte_TypeDef*)(LPC_GPIO0_BASE))
/** Fast GPIO port 1 byte accessible definition. */
#define GPIO1_Byte ((GPIO_Byte_TypeDef*)(LPC_GPIO1_BASE))
/** Fast GPIO port 2 byte accessible definition. */
#define GPIO2_Byte ((GPIO_Byte_TypeDef*)(LPC_GPIO2_BASE))
/** Fast GPIO port 3 byte accessible definition. */
#define GPIO3_Byte ((GPIO_Byte_TypeDef*)(LPC_GPIO3_BASE))
/** Fast GPIO port 4 byte accessible definition. */
#define GPIO4_Byte ((GPIO_Byte_TypeDef*)(LPC_GPIO4_BASE))

/* ------------------ MACROS HALF-WORD POINTER DEFINITIONS ------------------ */
/** Fast GPIO port 0 half-word accessible definition. */
#define GPIO0_HalfWord ((GPIO_HalfWord_TypeDef*)(LPC_GPIO0_BASE))
/** Fast GPIO port 1 half-word accessible definition. */
#define GPIO1_HalfWord ((GPIO_HalfWord_TypeDef*)(LPC_GPIO1_BASE))
/** Fast GPIO port 2 half-word accessible definition. */
#define GPIO2_HalfWord ((GPIO_HalfWord_TypeDef*)(LPC_GPIO2_BASE))
/** Fast GPIO port 3 half-word accessible definition. */
#define GPIO3_HalfWord ((GPIO_HalfWord_TypeDef*)(LPC_GPIO3_BASE))
/** Fast GPIO port 4 half-word accessible definition. */
#define GPIO4_HalfWord ((GPIO_HalfWord_TypeDef*)(LPC_GPIO4_BASE))

/**
 * @}
 */

/* ------------------------------ Public Types ------------------------------ */
/** @defgroup GPIO_Public_Types GPIO Public Types
 * @{
 */

/** Check GPIO port option parameter for interrupt. */
#define PARAM_GPIO_INT_PORT(PORT) ((PORT) == PORT_0 || (PORT) == PORT_2)

/**
 * @brief Half-word access for GPIO ports.
 */
typedef enum {
    GPIO_HALFWORD_LOW = 0,
    GPIO_HALFWORD_HIGH
} GPIO_HALFWORD;
/** Check GPIO half-word option parameter. */
#define PARAM_GPIO_HALFWORD(HW) ((HW) == GPIO_HALFWORD_LOW || (HW) == GPIO_HALFWORD_HIGH)

/**
 * @brief Byte access for GPIO ports.
 */
typedef enum {
    GPIO_BYTE_0 = 0,
    GPIO_BYTE_1,
    GPIO_BYTE_2,
    GPIO_BYTE_3
} GPIO_BYTE;
/** Check GPIO byte option parameter. */
#define PARAM_GPIO_BYTE(BYTE) ((BYTE) >= GPIO_BYTE_0 && (BYTE) <= GPIO_BYTE_3)

/**
 * @brief Direction selection for GPIO pins.
 */
typedef enum {
    GPIO_INPUT = 0,
    GPIO_OUTPUT
} GPIO_DIR;
/** Check GPIO direction option parameter. */
#define PARAM_GPIO_DIR(DIR) ((DIR) == GPIO_INPUT || (DIR) == GPIO_OUTPUT)

/**
 * @brief Edge selection for GPIO interrupts.
 */
typedef enum {
    GPIO_INT_RISING = 0,
    GPIO_INT_FALLING
} GPIO_INT_EDGE;
/** Check GPIO interrupt edge option parameter. */
#define PARAM_GPIO_INT_EDGE(EDGE) ((EDGE) == GPIO_INT_RISING || (EDGE) == GPIO_INT_FALLING)

/**
 * @brief Fast GPIO port byte type definition.
 */
typedef struct {
    __IO uint8_t FIODIR[4];  /**< FIO direction register in byte-align. */
    uint32_t RESERVED0[3];   /**< Reserved. */
    __IO uint8_t FIOMASK[4]; /**< FIO mask register in byte-align. */
    __IO uint8_t FIOPIN[4];  /**< FIO pin register in byte align. */
    __IO uint8_t FIOSET[4];  /**< FIO set register in byte-align. */
    __O uint8_t FIOCLR[4];   /**< FIO clear register in byte-align. */
} GPIO_Byte_TypeDef;

/**
 * @brief Fast GPIO port half-word type definition
 */
typedef struct {
    __IO uint16_t FIODIRL;  /**< FIO direction register lower halfword part. */
    __IO uint16_t FIODIRU;  /**< FIO direction register upper halfword part. */
    uint32_t RESERVED0[3];  /**< Reserved. */
    __IO uint16_t FIOMASKL; /**< FIO mask register lower halfword part. */
    __IO uint16_t FIOMASKU; /**< FIO mask register upper halfword part. */
    __IO uint16_t FIOPINL;  /**< FIO pin register lower halfword part. */
    __IO uint16_t FIOPINU;  /**< FIO pin register upper halfword part. */
    __IO uint16_t FIOSETL;  /**< FIO set register lower halfword part. */
    __IO uint16_t FIOSETU;  /**< FIO set register upper halfword part. */
    __O uint16_t FIOCLRL;   /**< FIO clear register lower halfword part. */
    __O uint16_t FIOCLRU;   /**< FIO clear register upper halfword part. */
} GPIO_HalfWord_TypeDef;

/**
 * @}
 */

/* ---------------------------- Public Functions ---------------------------- */
/** @defgroup GPIO_Public_Functions GPIO Public Functions
 * @{
 */

/* ------------------------------- GPIO style ------------------------------- */
/**
 * @brief Sets the direction for a group of pins on a specific port.
 *
 * Configures the FIODIR register. Sets bits to 1 for output and clears bits to 0 for input based on
 * the provided pinMask. Only the pins specified in pinMask are modified; other pins remain
 * unchanged.
 *
 * @param port    The GPIO port (PORT_x [0...4]).
 * @param pinMask A 32-bit mask representing the pins to modify.
 * @param dir     The direction (GPIO_INPUT or GPIO_OUTPUT).
 */
void GPIO_SetDir(LPC_PORT port, uint32_t pinMask, GPIO_DIR dir);

/**
 * @brief Sets specific pins to a high logic level.
 *
 * Uses the FIOSET register to drive the selected pins high. Bits in the mask set to 0 are not
 * affected. Only pins configured as output are affected; input pins remain unchanged. Pins masked
 * in the FIOMASK register will not be modified.
 *
 * @param port    The GPIO port (PORT_x [0...4]).
 * @param pinMask A 32-bit mask of the pins to set.
 */
void GPIO_SetPins(LPC_PORT port, uint32_t pinMask);

/**
 * @brief Clears specific pins to a low logic level.
 *
 * Uses the FIOCLR register to drive the selected pins low. Bits in the mask set to 0 are not
 * affected. Only pins configured as output are affected; input pins remain unchanged. Pins masked
 * in the FIOMASK register will not be modified.
 *
 * @param port    The GPIO port (PORT_x [0...4]).
 * @param pinMask A 32-bit mask of the pins to clear.
 */
void GPIO_ClearPins(LPC_PORT port, uint32_t pinMask);

/**
 * @brief Sets the logic state of a single pin.
 *
 * Calculates the mask for a specific pin and uses either the FIOSET or FIOCLR registers to update
 * its state. Only pins configured as output are affected; input pins remain unchanged. Pins masked
 * in the FIOMASK register will not be modified.
 *
 * @param port     The GPIO port (PORT_x [0...4]).
 * @param pin      The pin number (PIN_x [0...31]).
 * @param newState The desired state (SET for High, RESET for Low).
 */
void GPIO_SetPinState(LPC_PORT port, LPC_PIN pin, SetState newState);

/**
 * @brief Writes a 32-bit value to the entire port.
 *
 * Updates all pins on the port by clearing and setting pins according to newValue. Only pins
 * configured as output are affected; input pins remain unchanged. Pins masked in the FIOMASK
 * register will not be modified.
 *
 * @param port     The GPIO port (PORT_x [0...4]).
 * @param newValue The 32-bit value to write to the port.
 */
void GPIO_WriteValue(LPC_PORT port, uint32_t newValue);

/**
 * @brief Reads the current logic levels of all pins on a port.
 *
 * Returns the raw 32-bit value from the FIOPIN register, representing the actual state of the
 * physical pins regardless of their direction. Pins masked in the FIOMASK register will not be
 * modified.
 *
 * @param port The GPIO port to read (PORT_x [0...4]).
 * @return A 32-bit value representing the state of all pins on the port.
 */
uint32_t GPIO_ReadValue(LPC_PORT port);

/**
 * @brief Toggles the logic state of specific pins.
 *
 * Reads the current state of the port and applies a bitwise inversion to the pins defined in the
 * mask using atomic set/clear operations. Only pins configured as output are affected; input pins
 * remain unchanged. Pins masked in the FIOMASK register will not be modified.
 *
 * @param port    The GPIO port (PORT_x [0...4]).
 * @param pinMask A 32-bit mask of the pins to toggle.
 */
void GPIO_TogglePins(LPC_PORT port, uint32_t pinMask);

/**
 * @brief Configures the mask register for a port.
 *
 * Sets the FIOMASK register. Bits set to 1 in the mask will prevent write operations to the
 * corresponding physical pins through the other FIO registers. Any masked bit will read as 0.
 *
 * @param port     The GPIO port (PORT_x [0...4]).
 * @param pinMask  A 32-bit mask.
 * @param newState ENABLE to mask pins, DISABLE to unmask them.
 */
void GPIO_SetMask(LPC_PORT port, uint32_t pinMask, FunctionalState newState);

/**
 * @brief Configures the interrupt mask for an entire GPIO port.
 *
 * Writes a 32-bit value to the rising or falling edge interrupt enable registers (IOxIntEnR or
 * IOxIntEnF).
 *
 * @param port      The GPIO port (Only PORT_0 and PORT_2 are supported).
 * @param newValue  A 32-bit mask where each '1' enables the interrupt for that pin.
 * @param edgeState The edge to detect (GPIO_INT_RISING or GPIO_INT_FALLING).
 */
void GPIO_IntConfigPort(LPC_PORT port, uint32_t newValue, GPIO_INT_EDGE edgeState);

/**
 * @brief Enables or disables the interrupt for a specific GPIO pin and edge.
 *
 * Modifies the Rising Edge (EnR) or Falling Edge (EnF) interrupt enable registers for Port 0 or
 * Port 2. This allows for fine-grained control over which specific pins and transitions trigger an
 * interrupt request.
 *
 * @param port      The GPIO port (Only PORT_0 and PORT_2 are supported).
 * @param pin       The pin number (PIN_x [0...31]).
 * @param edgeState The edge to detect (GPIO_INT_RISING or GPIO_INT_FALLING).
 * @param newState  ENABLE to activate the interrupt, DISABLE to deactivate it.
 */
void GPIO_IntConfigPin(LPC_PORT port, LPC_PIN pin, GPIO_INT_EDGE edgeState,
                       FunctionalState newState);

/**
 * @brief Retrieves the global interrupt status for a specific GPIO port.
 *
 * Reads the IntStatus register to check if any pin on Port 0 or Port 2 currently has a pending
 * interrupt. This is typically used at the beginning of the EINT3 ISR to determine which port
 * caused the trigger.
 *
 * @param port The GPIO port to check (Only PORT_0 and PORT_2 are supported).
 * @return SET if the port has a pending interrupt, RESET otherwise.
 */
SetState GPIO_GetPortIntStatus(LPC_PORT port);

/**
 * @brief Checks the interrupt status of a specific pin and edge.
 *
 * Reads the IOxIntStatR or IOxIntStatF registers to verify if a specific pin triggered an interrupt
 * on the selected edge.
 *
 * @param port      The GPIO port (Only PORT_0 and PORT_2 are supported).
 * @param pin       The pin number (PIN_x [0...31]).
 * @param edgeState The edge status to verify (GPIO_INT_RISING or GPIO_INT_FALLING).
 * @return SET if the specific pin/edge interrupt is pending, RESET otherwise.
 */
SetState GPIO_GetPinIntStatus(LPC_PORT port, uint32_t pin, GPIO_INT_EDGE edgeState);

/**
 * @brief Clears the pending interrupt status for specific pins.
 *
 * Writes to the IOxIntClr register to acknowledge and clear the interrupt request. This must be
 * performed within the ISR to allow for future interrupt triggers.
 *
 * @param port     The GPIO port (Only PORT_0 and PORT_2 are supported).
 * @param pinMask  A 32-bit mask of the pins to clear.
 */
void GPIO_ClearInt(LPC_PORT port, uint32_t pinMask);

/* ---------------------- FIO (word-accessible) style ----------------------- */
/**
 * @brief The same as GPIO_SetDir().
 */
void FIO_SetDir(LPC_PORT port, uint32_t pinMask, GPIO_DIR dir);

/**
 * @brief The same as GPIO_SetPins().
 */
void FIO_SetPins(LPC_PORT port, uint32_t pinMask);

/**
 * @brief The same as GPIO_ClearPins().
 */
void FIO_ClearPins(LPC_PORT port, uint32_t pinMask);

/**
 * @brief The same as GPIO_SetPinState().
 */
void FIO_SetPinState(LPC_PORT port, LPC_PIN pin, SetState newState);

/**
 * @brief The same as GPIO_WriteValue().
 */
void FIO_WriteValue(LPC_PORT port, uint32_t newValue);

/**
 * @brief The same with GPIO_ReadValue().
 */
uint32_t FIO_ReadValue(LPC_PORT port);

/**
 * @brief The same with GPIO_TogglePins().
 */
void FIO_TogglePins(LPC_PORT port, uint32_t pinMask);

/**
 * @brief The same with GPIO_SetMask().
 */
void FIO_SetMask(LPC_PORT port, uint32_t pinMask, FunctionalState newState);

/**
 * @brief The same with GPIO_IntConfigPort().
 */
void FIO_IntConfigPort(LPC_PORT port, uint32_t newValue, GPIO_INT_EDGE edgeState);

/**
 * @brief The same with GPIO_IntConfigPin().
 */
void FIO_IntConfigPin(LPC_PORT port, LPC_PIN pin, GPIO_INT_EDGE edgeState,
                      FunctionalState newState);

/**
 * @brief The same with GPIO_GetPortIntStatus().
 */
SetState FIO_GetPortIntStatus(LPC_PORT port);

/**
 * @brief The same with GPIO_GetPinIntStatus().
 */
SetState FIO_GetPinIntStatus(LPC_PORT port, uint32_t pin, GPIO_INT_EDGE edgeState);

/**
 * @brief The same with GPIO_ClearInt().
 */
void FIO_ClearInt(LPC_PORT port, uint32_t pinMask);

/* -------------------- FIO (halfword-accessible) style --------------------- */
/**
 * @brief Sets the direction for a group of pins on a specific port in halfword-accessible mode.
 *
 * Configures the FIODIR register. Sets bits to 1 for output and clears bits to 0 for input based on
 * the provided pinMask. Only the pins specified in pinMask are modified; other pins remain
 * unchanged.
 *
 * @param port      The GPIO port (PORT_x [0...4]).
 * @param halfword  GPIO_HALFWORD_LOW (bits 0-15) or GPIO_HALFWORD_HIGH (bits 16-31).
 * @param pinMask   A 32-bit mask representing the pins to modify.
 * @param dir       The direction (GPIO_INPUT or GPIO_OUTPUT).
 */
void FIO_HalfWordSetDir(LPC_PORT port, GPIO_HALFWORD halfword, uint16_t pinMask, GPIO_DIR dir);

/**
 * @brief Sets specific pins to a high logic level in halfword-accessible mode.
 *
 * Uses the FIOSET register to drive the selected pins high. Bits in the mask set to 0 are not
 * affected. Only pins configured as output are affected; input pins remain unchanged. Pins masked
 * in the FIOMASK register will not be modified.
 *
 * @param port    The GPIO port (PORT_x [0...4]).
 * @param pinMask A 32-bit mask of the pins to set.
 */
void FIO_HalfWordSetPins(LPC_PORT port, GPIO_HALFWORD halfword, uint16_t pinMask);

/**
 * @brief Clears specific pins to a low logic level in halfword-accessible mode.
 *
 * Uses the FIOCLR register to drive the selected pins low. Bits in the mask set to 0 are not
 * affected. Only pins configured as output are affected; input pins remain unchanged. Pins masked
 * in the FIOMASK register will not be modified.
 *
 * @param port    The GPIO port (PORT_x [0...4]).
 * @param pinMask A 32-bit mask of the pins to clear.
 */
void FIO_HalfWordClearPins(LPC_PORT port, GPIO_HALFWORD halfword, uint16_t pinMask);

/**
 * @brief        Writes a value to all pins of the specified FIO port halfword.
 *
 * @param[in]    port       PORT_x [0...4].
 * @param[in]    halfword   GPIO_HALFWORD_LOW (bits 0-15) or GPIO_HALFWORD_HIGH (bits 16-31).
 * @param[in]    newValue   Value to be written to the FIO halfword register (0x0 to 0xFFFF).
 *                          Each bit corresponds to a pin (1 = high, 0 = low).
 *
 * @note:
 * - Only pins configured as output are affected; input pins are not changed.
 * - Pins masked in the FIOMASK register will not be affected by this operation.
 */
void FIO_HalfWordWriteValue(LPC_PORT port, GPIO_HALFWORD halfword, uint16_t newValue);

/**
 * @brief        Reads the current state of all pins on the specified FIO port halfword.
 *
 * @param[in]    port       PORT_x [0...4].
 * @param[in]    halfword   GPIO_HALFWORD_LOW (bits 0-15) or GPIO_HALFWORD_HIGH (bits 16-31).
 *
 * @note:
 * - The returned value contains the logic state of each pin (bit) in the selected halfword,
 *   regardless of whether the pin is configured as input or output.
 * - Pins masked in the FIOMASK register will return 0 in the corresponding bits.
 *
 * @return       16-bit value representing the current state of all pins in the selected halfword.
 */
uint16_t FIO_HalfWordReadValue(LPC_PORT port, GPIO_HALFWORD halfword);

/**
 * @brief        Toggles the state of specified pins for a FIO port in halfword-accessible mode.
 *
 * @param[in]    port       PORT_x [0...4].
 * @param[in]    halfword   GPIO_HALFWORD_LOW (bits 0-15) or GPIO_HALFWORD_HIGH (bits 16-31).
 * @param[in]    pinMask    Bitmask indicating which pins to toggle (0x0 to 0xFFFF).
 *                          Example: 0x0005 toggles pins 0 and 2.
 *
 * @note:
 * - Only pins configured as output are affected; input pins are not changed.
 * - Pins not selected in pinMask remain unchanged.
 */
void FIO_HalfWordTogglePins(LPC_PORT port, GPIO_HALFWORD halfword, uint16_t pinMask);

/**
 * @brief        Sets or clears the mask for selected bits in a FIO port halfword.
 *
 * @param[in]    port       PORT_x [0...4].
 * @param[in]    halfword   GPIO_HALFWORD_LOW (bits 0-15) or GPIO_HALFWORD_HIGH (bits 16-31).
 * @param[in]    pinMask    Bitmask indicating which bits to mask or unmask (0x0 to 0xFFFF).
 * @param[in]    newState   FunctionalState value:
 *                          - ENABLE: Mask the selected bits (access disabled).
 *                          - DISABLE: Unmask the selected bits (access enabled).
 *
 * @note:
 * - Only the bits selected in pinMask are affected.
 * - After masking, read/write operations to masked bits will have no effect or return 0.
 */
void FIO_HalfWordSetMask(LPC_PORT port, GPIO_HALFWORD halfword, uint16_t pinMask,
                         FunctionalState newState);

/* ---------------------- FIO (byte-accessible) style ----------------------- */
/**
 * @brief        Sets the direction for specified pins in a FIO port byte.
 *
 * @param[in]    port       PORT_x [0...4].
 * @param[in]    byte       GPIO_BYTE_x [0...3].
 * @param[in]    pinMask    Bitmask indicating which bits to configure (0x0 to 0xFF).
 * @param[in]    dir        Pin direction: GPIO_INPUT or GPIO_OUTPUT.
 *
 * @note:
 * - Only the bits selected in pinMask are affected.
 */
void FIO_ByteSetDir(LPC_PORT port, GPIO_BYTE byte, uint8_t pinMask, GPIO_DIR dir);

/**
 * @brief        Sets the specified output pins to high for a FIO port in byte-accessible mode.
 *
 * @param[in]    port       PORT_x [0...4].
 * @param[in]    byte       GPIO_BYTE_x [0...3].
 * @param[in]    pinMask    Bitmask specifying which pins to set high (0x0 to 0xFF).
 *
 * @note:
 * - Only pins configured as output are affected; input pins are not changed.
 * - Pins not selected in pinMask remain unchanged.
 */
void FIO_ByteSetPins(LPC_PORT port, GPIO_BYTE byte, uint8_t pinMask);

/**
 * @brief        Clears the specified output pins to low for a FIO port in byte-accessible mode.
 *
 * @param[in]    port       PORT_x [0...4].
 * @param[in]    byte       GPIO_BYTE_x [0...3].
 * @param[in]    pinMask    Bitmask specifying which pins to set low (0x0 to 0xFF).
 *
 * @note:
 * - Only pins configured as output are affected; input pins are not changed.
 * - Pins not selected in pinMask remain unchanged.
 */
void FIO_ByteClearPins(LPC_PORT port, GPIO_BYTE byte, uint8_t pinMask);

/**
 * @brief        Writes a value to all pins of the specified FIO port byte.
 *
 * @param[in]    port       PORT_x [0...4].
 * @param[in]    byte       GPIO_BYTE_x [0...3].
 * @param[in]    newValue   Value to be written to the FIO byte register (0x0 to 0xFF).
 *                          Each bit corresponds to a pin (1 = high, 0 = low).
 *
 * @note:
 * - Only pins configured as output are affected; input pins are not changed.
 * - Pins masked in the FIOMASK register will not be affected by this operation.
 */
void FIO_ByteWriteValue(LPC_PORT port, GPIO_BYTE byte, uint8_t newValue);

/**
 * @brief        Reads the current state of all pins on the specified FIO port byte.
 *
 * @param[in]    port   PORT_x [0...4].
 * @param[in]    byte   GPIO_BYTE_x [0...3].
 *
 * @note:
 * - The returned value contains the logic state of each pin (bit) in the selected byte,
 *   regardless of whether the pin is configured as input or output.
 * - Pins masked in the FIOMASK register will return 0 in the corresponding bits.
 *
 * @return       8-bit value representing the current state of all pins in the selected byte.
 */
uint8_t FIO_ByteReadValue(LPC_PORT port, GPIO_BYTE byte);

/**
 * @brief        Toggles the state of specified pins for a FIO port in byte-accessible mode.
 *
 * @param[in]    port       PORT_x [0...4].
 * @param[in]    byte       GPIO_BYTE_x [0...3].
 * @param[in]    pinMask    Bitmask indicating which pins to toggle (0x0 to 0xFF).
 *
 * @note:
 * - Only pins configured as output are affected; input pins are not changed.
 * - Pins not selected in pinMask remain unchanged.
 */
void FIO_ByteTogglePins(LPC_PORT port, GPIO_BYTE byte, uint8_t pinMask);

/**
 * @brief        Sets or clears the mask for selected bits in a FIO port byte.
 *
 * @param[in]    port       PORT_x [0...4].
 * @param[in]    byte       GPIO_BYTE_x [0...3].
 * @param[in]    pinMask    Bitmask indicating which bits to mask or unmask (0x0 to 0xFF).
 * @param[in]    newState   FunctionalState value:
 *                          - ENABLE: Mask the selected bits (access disabled).
 *                          - DISABLE: Unmask the selected bits (access enabled).
 *
 * @note:
 * - Only the bits selected in pinMask are affected.
 * - After masking, read/write operations to masked bits will have no effect or return 0.
 */
void FIO_ByteSetMask(LPC_PORT port, GPIO_BYTE byte, uint8_t pinMask, FunctionalState newState);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  // LPC17XX_GPIO_H_

/**
 * @}
 */

/* ------------------------------ End Of File ------------------------------- */
