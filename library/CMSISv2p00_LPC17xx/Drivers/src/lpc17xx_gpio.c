/**
 * @file        lpc17xx_gpio.c
 * @brief       Contains all functions support for GPIO firmware library on LPC17xx
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
/** @addtogroup GPIO
 * @{
 */

/* -------------------------------- Includes -------------------------------- */
#include "lpc17xx_gpio.h"

/* If this source file built with example, the LPC17xx FW library configuration
 * file in each example directory ("lpc17xx_libcfg.h") must be included,
 * otherwise the default FW library configuration file must be included instead
 */
#ifdef __BUILD_WITH_EXAMPLE__
#include "lpc17xx_libcfg.h"
#else
#include "lpc17xx_libcfg_default.h"
#endif /* __BUILD_WITH_EXAMPLE__ */

#ifdef _GPIO

/* ---------------------- Private Function Prototypes ----------------------- */
/**
 * @brief       Returns a pointer to the GPIO peripheral structure for the given port number.
 * @param[in]   port    GPIO_PORT_x, where x is in the range [0,4].
 * @return      Pointer to GPIO peripheral, or NULL if port is invalid.
 */
static LPC_GPIO_TypeDef* GPIO_GetPointer(uint8_t port);

/**
 * @brief       Returns a pointer to the FIO peripheral structure halfword
 *              accessible for the given port number.
 * @param[in]   port    GPIO_PORT_x, where x is in the range [0,4].
 * @return      Pointer to FIO peripheral, or NULL if port is invalid.
 */
static GPIO_HalfWord_TypeDef* FIO_HalfWordGetPointer(uint8_t port);

/**
 * @brief       Returns a pointer to the FIO peripheral structure byte
 *              accessible for the given port number.
 * @param[in]   port    GPIO_PORT_x, where x is in the range [0,4].
 * @return      Pointer to FIO peripheral, or NULL if port is invalid.
 */
static GPIO_Byte_TypeDef* FIO_ByteGetPointer(uint8_t port);
/* ------------------- End of Private Function Prototypes ------------------- */

/* --------------------------- Private Functions ---------------------------- */
static LPC_GPIO_TypeDef* GPIO_GetPointer(uint8_t port) {
    return &LPC_GPIO0[port];
}

static GPIO_HalfWord_TypeDef* FIO_HalfWordGetPointer(uint8_t port) {
    return &GPIO0_HalfWord[port];
}

static GPIO_Byte_TypeDef* FIO_ByteGetPointer(uint8_t port) {
    return &GPIO0_Byte[port];
}
/* ------------------------ End of Private Functions ------------------------ */

/* ---------------------------- Public Functions ---------------------------- */
/** @addtogroup GPIO_Public_Functions
 * @{
 */

/* ------------------------------- GPIO style ------------------------------- */
void GPIO_SetDir(LPC_PORT port, uint32_t pinMask, GPIO_DIR dir) {
    CHECK_PARAM(PARAM_LPC_PORT(port));
    CHECK_PARAM(PARAM_GPIO_DIR(dir));

    LPC_GPIO_TypeDef* pGPIO = GPIO_GetPointer(port);

    if (dir == GPIO_OUTPUT) {
        pGPIO->FIODIR |= pinMask;
    } else {
        pGPIO->FIODIR &= ~pinMask;
    }
}

void GPIO_SetPins(LPC_PORT port, uint32_t pinMask) {
    CHECK_PARAM(PARAM_LPC_PORT(port));

    LPC_GPIO_TypeDef* pGPIO = GPIO_GetPointer(port);

    pGPIO->FIOSET = pinMask;
}

void GPIO_ClearPins(LPC_PORT port, uint32_t pinMask) {
    CHECK_PARAM(PARAM_LPC_PORT(port));

    LPC_GPIO_TypeDef* pGPIO = GPIO_GetPointer(port);

    pGPIO->FIOCLR = pinMask;
}

void GPIO_SetPinState(LPC_PORT port, LPC_PIN pin, SetState newState) {
    CHECK_PARAM(PARAM_LPC_PORT(port));
    CHECK_PARAM(PARAM_LPC_PIN(pin));
    CHECK_PARAM(PARAM_SETSTATE(newState));

    LPC_GPIO_TypeDef* pGPIO = GPIO_GetPointer(port);
    const uint32_t pinMask  = GPIO_PIN_MASK << pin;

    if (newState == SET) {
        pGPIO->FIOSET = pinMask;
    } else {
        pGPIO->FIOCLR = pinMask;
    }
}

void GPIO_WriteValue(LPC_PORT port, uint32_t newValue) {
    CHECK_PARAM(PARAM_LPC_PORT(port));

    LPC_GPIO_TypeDef* pGPIO = GPIO_GetPointer(port);

    pGPIO->FIOCLR = ~newValue;
    pGPIO->FIOSET = newValue;
}

uint32_t GPIO_ReadValue(LPC_PORT port) {
    CHECK_PARAM(PARAM_LPC_PORT(port));

    LPC_GPIO_TypeDef* pGPIO = GPIO_GetPointer(port);

    return pGPIO->FIOPIN;

    return 0;
}

void GPIO_TogglePins(LPC_PORT port, uint32_t pinMask) {
    CHECK_PARAM(PARAM_LPC_PORT(port));

    LPC_GPIO_TypeDef* pGPIO = GPIO_GetPointer(port);

    const uint32_t current = pGPIO->FIOPIN;

    pGPIO->FIOSET = ~current & pinMask;
    pGPIO->FIOCLR = current & pinMask;
}

void GPIO_SetMask(LPC_PORT port, uint32_t pinMask, FunctionalState newState) {
    CHECK_PARAM(PARAM_LPC_PORT(port));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(newState));

    LPC_GPIO_TypeDef* pFIO = GPIO_GetPointer(port);

    if (newState) {
        pFIO->FIOMASK |= pinMask;
    } else {
        pFIO->FIOMASK &= ~pinMask;
    }
}

void GPIO_IntConfigPort(LPC_PORT port, uint32_t newValue, GPIO_INT_EDGE edgeState) {
    CHECK_PARAM(PARAM_GPIO_INT_PORT(port));
    CHECK_PARAM(PARAM_GPIO_INT_EDGE(edgeState));

    __IO uint32_t* pIntReg = NULL;

    if (port == PORT_0) {
        pIntReg = edgeState == GPIO_INT_RISING ? &LPC_GPIOINT->IO0IntEnR : &LPC_GPIOINT->IO0IntEnF;
    } else {
        pIntReg = edgeState == GPIO_INT_RISING ? &LPC_GPIOINT->IO2IntEnR : &LPC_GPIOINT->IO2IntEnF;
    }

    *pIntReg = newValue;
}

void GPIO_IntConfigPin(LPC_PORT port, LPC_PIN pin, GPIO_INT_EDGE edgeState,
                       FunctionalState newState) {
    CHECK_PARAM(PARAM_GPIO_INT_PORT(port));
    CHECK_PARAM(PARAM_LPC_PIN(pin));
    CHECK_PARAM(PARAM_GPIO_INT_EDGE(edgeState));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(newState));

    __IO uint32_t* pIntReg = NULL;
    const uint32_t pinMask = GPIO_PIN_MASK << pin;

    if (port == PORT_0) {
        pIntReg = edgeState == GPIO_INT_RISING ? &LPC_GPIOINT->IO0IntEnR : &LPC_GPIOINT->IO0IntEnF;
    } else {
        pIntReg = edgeState == GPIO_INT_RISING ? &LPC_GPIOINT->IO2IntEnR : &LPC_GPIOINT->IO2IntEnF;
    }

    if (newState == ENABLE) {
        *pIntReg |= pinMask;
    } else {
        *pIntReg &= ~pinMask;
    }
}

SetState GPIO_GetPortIntStatus(LPC_PORT port) {
    CHECK_PARAM(PARAM_GPIO_INT_PORT(port));

    const uint32_t shift = port == PORT_0 ? 0 : 2;

    if ((LPC_GPIOINT->IntStatus >> shift) & 0x01UL) {
        return SET;
    }
    return RESET;
}

SetState GPIO_GetPinIntStatus(LPC_PORT port, uint32_t pin, GPIO_INT_EDGE edgeState) {
    CHECK_PARAM(PARAM_GPIO_INT_PORT(port));
    CHECK_PARAM(PARAM_GPIO_INT_EDGE(edgeState));

    __I uint32_t* pIntStatReg = NULL;
    const uint32_t pinMask    = GPIO_PIN_MASK << pin;

    if (port == PORT_0) {
        pIntStatReg =
            edgeState == GPIO_INT_RISING ? &LPC_GPIOINT->IO0IntStatR : &LPC_GPIOINT->IO0IntStatF;
    } else {
        pIntStatReg =
            edgeState == GPIO_INT_RISING ? &LPC_GPIOINT->IO2IntStatR : &LPC_GPIOINT->IO2IntStatF;
    }

    if ((*pIntStatReg & pinMask) != 0U) {
        return SET;
    }
    return RESET;
}

void GPIO_ClearInt(LPC_PORT port, uint32_t pinMask) {
    CHECK_PARAM(PARAM_GPIO_INT_PORT(port));

    if (port == 0) {
        LPC_GPIOINT->IO0IntClr = pinMask;
    } else {
        LPC_GPIOINT->IO2IntClr = pinMask;
    }
}

/* ---------------------- FIO (word-accessible) style ----------------------- */
void FIO_SetDir(LPC_PORT port, uint32_t pinMask, GPIO_DIR dir) {
    GPIO_SetDir(port, pinMask, dir);
}

void FIO_SetPins(LPC_PORT port, uint32_t pinMask) {
    GPIO_SetPins(port, pinMask);
}

void FIO_ClearPins(LPC_PORT port, uint32_t pinMask) {
    GPIO_ClearPins(port, pinMask);
}

void FIO_SetPinState(LPC_PORT port, LPC_PIN pin, SetState newState) {
    GPIO_SetPinState(port, pin, newState);
}

void FIO_WriteValue(LPC_PORT port, uint32_t newValue) {
    GPIO_WriteValue(port, newValue);
}

uint32_t FIO_ReadValue(LPC_PORT port) {
    return (GPIO_ReadValue(port));
}

void FIO_TogglePins(LPC_PORT port, uint32_t pinMask) {
    GPIO_TogglePins(port, pinMask);
}

void FIO_SetMask(LPC_PORT port, uint32_t pinMask, FunctionalState newState) {
    GPIO_SetMask(port, pinMask, newState);
}

void FIO_IntConfigPort(LPC_PORT port, uint32_t newValue, GPIO_INT_EDGE edgeState) {
    GPIO_IntConfigPort(port, newValue, edgeState);
}

void FIO_IntConfigPin(LPC_PORT port, LPC_PIN pin, GPIO_INT_EDGE edgeState,
                      FunctionalState newState) {
    GPIO_IntConfigPin(port, pin, edgeState, newState);
}

SetState FIO_GetPortIntStatus(LPC_PORT port) {
    return GPIO_GetPortIntStatus(port);
}

SetState FIO_GetPinIntStatus(LPC_PORT port, uint32_t pin, GPIO_INT_EDGE edgeState) {
    return GPIO_GetPinIntStatus(port, pin, edgeState);
}

void FIO_ClearInt(LPC_PORT port, uint32_t pinMask) {
    GPIO_ClearInt(port, pinMask);
}

/* -------------------- FIO (halfword-accessible) style --------------------- */
void FIO_HalfWordSetDir(LPC_PORT port, GPIO_HALFWORD halfword, uint16_t pinMask, GPIO_DIR dir) {
    CHECK_PARAM(PARAM_LPC_PORT(port));
    CHECK_PARAM(PARAM_GPIO_HALFWORD(halfword));
    CHECK_PARAM(PARAM_GPIO_DIR(dir));

    GPIO_HalfWord_TypeDef* pFIO = FIO_HalfWordGetPointer(port);

    if (dir == GPIO_OUTPUT) {
        if (halfword == GPIO_HALFWORD_HIGH) {
            pFIO->FIODIRU |= pinMask;
        } else {
            pFIO->FIODIRL |= pinMask;
        }
    } else {
        if (halfword == GPIO_HALFWORD_HIGH) {
            pFIO->FIODIRU &= ~pinMask;
        } else {
            pFIO->FIODIRL &= ~pinMask;
        }
    }
}

void FIO_HalfWordSetPins(LPC_PORT port, GPIO_HALFWORD halfword, uint16_t pinMask) {
    CHECK_PARAM(PARAM_LPC_PORT(port));
    CHECK_PARAM(PARAM_GPIO_HALFWORD(halfword));

    GPIO_HalfWord_TypeDef* pFIO = FIO_HalfWordGetPointer(port);

    if (halfword == GPIO_HALFWORD_HIGH) {
        pFIO->FIOSETU = pinMask;
    } else {
        pFIO->FIOSETL = pinMask;
    }
}

void FIO_HalfWordClearPins(LPC_PORT port, GPIO_HALFWORD halfword, uint16_t pinMask) {
    CHECK_PARAM(PARAM_LPC_PORT(port));
    CHECK_PARAM(PARAM_GPIO_HALFWORD(halfword));

    GPIO_HalfWord_TypeDef* pFIO = FIO_HalfWordGetPointer(port);

    if (halfword == GPIO_HALFWORD_HIGH) {
        pFIO->FIOCLRU = pinMask;
    } else {
        pFIO->FIOCLRL = pinMask;
    }
}

void FIO_HalfWordWriteValue(LPC_PORT port, GPIO_HALFWORD halfword, uint16_t newValue) {
    CHECK_PARAM(PARAM_LPC_PORT(port));
    CHECK_PARAM(PARAM_GPIO_HALFWORD(halfword));

    GPIO_HalfWord_TypeDef* pFIO = FIO_HalfWordGetPointer(port);

    if (halfword == GPIO_HALFWORD_HIGH) {
        pFIO->FIOCLRU = ~newValue;
        pFIO->FIOSETU = newValue;
    } else {
        pFIO->FIOCLRL = ~newValue;
        pFIO->FIOSETL = newValue;
    }
}

uint16_t FIO_HalfWordReadValue(LPC_PORT port, GPIO_HALFWORD halfword) {
    CHECK_PARAM(PARAM_LPC_PORT(port));
    CHECK_PARAM(PARAM_GPIO_HALFWORD(halfword));

    GPIO_HalfWord_TypeDef* pFIO = FIO_HalfWordGetPointer(port);

    if (halfword == GPIO_HALFWORD_HIGH) {
        return (pFIO->FIOPINU);
    }

    return (pFIO->FIOPINL);
}

void FIO_HalfWordTogglePins(LPC_PORT port, GPIO_HALFWORD halfword, uint16_t pinMask) {
    CHECK_PARAM(PARAM_LPC_PORT(port));
    CHECK_PARAM(PARAM_GPIO_HALFWORD(halfword));

    GPIO_HalfWord_TypeDef* pFIO = FIO_HalfWordGetPointer(port);

    if (halfword == GPIO_HALFWORD_HIGH) {
        const uint16_t current = pFIO->FIOPINU;

        pFIO->FIOSETU = (~current) & pinMask;
        pFIO->FIOCLRU = current & pinMask;
    } else {
        const uint16_t current = pFIO->FIOPINL;

        pFIO->FIOSETL = (~current) & pinMask;
        pFIO->FIOCLRL = current & pinMask;
    }
}

void FIO_HalfWordSetMask(LPC_PORT port, GPIO_HALFWORD halfword, uint16_t pinMask,
                         FunctionalState newState) {
    CHECK_PARAM(PARAM_LPC_PORT(port));
    CHECK_PARAM(PARAM_GPIO_HALFWORD(halfword));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(newState));

    GPIO_HalfWord_TypeDef* pFIO = FIO_HalfWordGetPointer(port);

    if (newState == ENABLE) {
        if (halfword == GPIO_HALFWORD_HIGH) {
            pFIO->FIOMASKU |= pinMask;
        } else {
            pFIO->FIOMASKL |= pinMask;
        }
    } else {
        if (halfword == GPIO_HALFWORD_HIGH) {
            pFIO->FIOMASKU &= ~pinMask;
        } else {
            pFIO->FIOMASKL &= ~pinMask;
        }
    }
}

/* ---------------------- FIO (byte-accessible) style ----------------------- */
void FIO_ByteSetDir(LPC_PORT port, GPIO_BYTE byte, uint8_t pinMask, GPIO_DIR dir) {
    CHECK_PARAM(PARAM_LPC_PORT(port));
    CHECK_PARAM(PARAM_GPIO_BYTE(byte));
    CHECK_PARAM(PARAM_GPIO_DIR(dir));

    GPIO_Byte_TypeDef* pFIO = FIO_ByteGetPointer(port);

    if (dir == GPIO_OUTPUT) {
        pFIO->FIODIR[byte] |= pinMask;
    } else {
        pFIO->FIODIR[byte] &= ~pinMask;
    }
}

void FIO_ByteSetPins(LPC_PORT port, GPIO_BYTE byte, uint8_t pinMask) {
    CHECK_PARAM(PARAM_LPC_PORT(port));
    CHECK_PARAM(PARAM_GPIO_BYTE(byte));

    GPIO_Byte_TypeDef* pFIO = FIO_ByteGetPointer(port);

    pFIO->FIOSET[byte] = pinMask;
}

void FIO_ByteClearPins(LPC_PORT port, GPIO_BYTE byte, uint8_t pinMask) {
    CHECK_PARAM(PARAM_LPC_PORT(port));
    CHECK_PARAM(PARAM_GPIO_BYTE(byte));

    GPIO_Byte_TypeDef* pFIO = FIO_ByteGetPointer(port);

    pFIO->FIOCLR[byte] = pinMask;
}

void FIO_ByteWriteValue(LPC_PORT port, GPIO_BYTE byte, uint8_t newValue) {
    CHECK_PARAM(PARAM_LPC_PORT(port));
    CHECK_PARAM(PARAM_GPIO_BYTE(byte));

    GPIO_Byte_TypeDef* pFIO = FIO_ByteGetPointer(port);

    pFIO->FIOCLR[byte] = ~newValue;
    pFIO->FIOSET[byte] = newValue;
}

uint8_t FIO_ByteReadValue(LPC_PORT port, GPIO_BYTE byte) {
    CHECK_PARAM(PARAM_LPC_PORT(port));
    CHECK_PARAM(PARAM_GPIO_BYTE(byte));

    GPIO_Byte_TypeDef* pFIO = FIO_ByteGetPointer(port);

    return (pFIO->FIOPIN[byte]);
}

void FIO_ByteTogglePins(LPC_PORT port, GPIO_BYTE byte, uint8_t pinMask) {
    CHECK_PARAM(PARAM_LPC_PORT(port));
    CHECK_PARAM(PARAM_GPIO_BYTE(byte));

    GPIO_Byte_TypeDef* pFIO = FIO_ByteGetPointer(port);

    const uint8_t current = pFIO->FIOPIN[byte];
    pFIO->FIOSET[byte]    = (~current) & pinMask;
    pFIO->FIOCLR[byte]    = current & pinMask;
}

void FIO_ByteSetMask(LPC_PORT port, GPIO_BYTE byte, uint8_t pinMask, FunctionalState newState) {
    CHECK_PARAM(PARAM_LPC_PORT(port));
    CHECK_PARAM(PARAM_GPIO_BYTE(byte));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(newState));

    GPIO_Byte_TypeDef* pFIO = FIO_ByteGetPointer(port);

    if (newState == ENABLE) {
        pFIO->FIOMASK[byte] |= pinMask;
    } else {
        pFIO->FIOMASK[byte] &= ~pinMask;
    }
}

/**
 * @}
 */

#endif /* _GPIO */

/**
 * @}
 */

/* ------------------------------ End Of File ------------------------------- */
