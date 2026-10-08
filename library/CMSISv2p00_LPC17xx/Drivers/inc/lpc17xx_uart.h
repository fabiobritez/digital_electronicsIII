/**
 * @file        lpc17xx_uart.h
 * @brief       Contains all macro definitions and function prototypes
 *              support for UART firmware library on LPC17xx
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
 * Last update: 02/03/2026, Author: David Trujillo Medina
 */

/* Peripheral group ----------------------------------------------------------- */
/** @defgroup UART UART
 * @ingroup LPC1700CMSIS_FwLib_Drivers
 * @{
 */

#ifndef __LPC17XX_UART_H
#define __LPC17XX_UART_H

/* Includes ------------------------------------------------------------------- */
#include "LPC17xx.h"
#include "lpc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Public Macros -------------------------------------------------------------- */
/** @defgroup UART_Public_Macros  UART Public Macros
 * @{
 */

/** UART time-out definitions in case of using Read() and Write function
 * with Blocking Flag mode
 */
#define UART_BLOCKING_TIMEOUT (0xFFFFFFFFUL)

/**
 * @}
 */

/* Private Macros ------------------------------------------------------------- */
/** @defgroup UART_Private_Macros UART Private Macros
 * @{
 */

/** UART0 peripheral instance pointer (Type-casted for common access). */
#define UART0 ((LPC_UART_TypeDef*)LPC_UART0)
/** UART1 peripheral instance pointer (Type-casted for common access). */
#define UART1 ((LPC_UART_TypeDef*)LPC_UART1)
/** UART2 peripheral instance pointer (Type-casted for common access). */
#define UART2 ((LPC_UART_TypeDef*)LPC_UART2)
/** UART3 peripheral instance pointer (Type-casted for common access). */
#define UART3 ((LPC_UART_TypeDef*)LPC_UART3)

/** UART One Stop Bits Select. */
#define UART_LCR_STOPBITS_1 ((0 << 2))
/** UART Two Stop Bits Select. */
#define UART_LCR_STOPBITS_2 ((1 << 2))

/* Accepted Error baud rate value (in percent unit) */
#define UART_ACCEPTED_BAUDRATE_ERROR (3) /*!< Acceptable UART baudrate error */

/* --------------------- BIT DEFINITIONS -------------------------------------- */
/** UART Received Buffer mask bit (8 bits) */
#define UART_RBR_MASKBIT ((uint8_t)0xFF)

/** UART Transmit Holding mask bit (8 bits) */
#define UART_THR_MASKBIT ((uint8_t)0xFF)

/** Macro for loading least significant halfs of divisors */
#define UART_LOAD_DLL(div) ((div) & 0xFF)
/** Divisor latch LSB bit mask */
#define UART_DLL_MASKBIT   ((uint8_t)0xFF)

/** Divisor latch MSB bit mask */
#define UART_DLM_MASKBIT   ((uint8_t)0xFF)
/** Macro for loading most significant halfs of divisors */
#define UART_LOAD_DLM(div) (((div) >> 8) & 0xFF)

/** RBR Interrupt enable*/
#define UART_IER_RBRINT_EN  ((uint32_t)(1 << 0))
/** THR Interrupt enable*/
#define UART_IER_THREINT_EN ((uint32_t)(1 << 1))
/** RX line status interrupt enable*/
#define UART_IER_RLSINT_EN  ((uint32_t)(1 << 2))
/** Modem status interrupt enable */
#define UART1_IER_MSINT_EN  ((uint32_t)(1 << 3))
/** CTS1 signal transition interrupt enable */
#define UART1_IER_CTSINT_EN ((uint32_t)(1 << 7))
/** Enables the end of auto-baud interrupt */
#define UART_IER_ABEOINT_EN ((uint32_t)(1 << 8))
/** Enables the auto-baud time-out interrupt */
#define UART_IER_ABTOINT_EN ((uint32_t)(1 << 9))
/** UART interrupt enable register bit mask */
#define UART_IER_BITMASK    ((uint32_t)(0x307))
/** UART1 interrupt enable register bit mask */
#define UART1_IER_BITMASK   ((uint32_t)(0x38F))

/** Interrupt Status - Active low */
#define UART_IIR_INTSTAT_PEND ((uint32_t)(1 << 0))
/** Interrupt identification: Receive line status*/
#define UART_IIR_INTID_RLS    ((uint32_t)(3 << 1))
/** Interrupt identification: Receive data available*/
#define UART_IIR_INTID_RDA    ((uint32_t)(2 << 1))
/** Interrupt identification: Character time-out indicator*/
#define UART_IIR_INTID_CTI    ((uint32_t)(6 << 1))
/** Interrupt identification: THRE interrupt*/
#define UART_IIR_INTID_THRE   ((uint32_t)(1 << 1))
/** Interrupt identification: Modem interrupt*/
#define UART1_IIR_INTID_MODEM ((uint32_t)(0 << 1))
/** Interrupt identification: Interrupt ID mask */
#define UART_IIR_INTID_MASK   ((uint32_t)(7 << 1))
/** These bits are equivalent to UnFCR[0] */
#define UART_IIR_FIFO_EN      ((uint32_t)(3 << 6))
/** End of auto-baud interrupt */
#define UART_IIR_ABEO_INT     ((uint32_t)(1 << 8))
/** Auto-baud time-out interrupt */
#define UART_IIR_ABTO_INT     ((uint32_t)(1 << 9))
/** UART interrupt identification register bit mask */
#define UART_IIR_BITMASK      ((uint32_t)(0x3CF))

/** UART FIFO enable */
#define UART_FCR_FIFO_EN     ((uint8_t)(1 << 0))
/** UART FIFO RX reset */
#define UART_FCR_RX_RS       ((uint8_t)(1 << 1))
/** UART FIFO TX reset */
#define UART_FCR_TX_RS       ((uint8_t)(1 << 2))
/** UART DMA mode selection */
#define UART_FCR_DMAMODE_SEL ((uint8_t)(1 << 3))
/** UART FIFO trigger level 0: 1 character */
#define UART_FCR_TRG_LEV0    ((uint8_t)(0))
/** UART FIFO trigger level 1: 4 character */
#define UART_FCR_TRG_LEV1    ((uint8_t)(1 << 6))
/** UART FIFO trigger level 2: 8 character */
#define UART_FCR_TRG_LEV2    ((uint8_t)(2 << 6))
/** UART FIFO trigger level 3: 14 character */
#define UART_FCR_TRG_LEV3    ((uint8_t)(3 << 6))
/** UART FIFO control bit mask */
#define UART_FCR_BITMASK     ((uint8_t)(0xCF))
#define UART_TX_FIFO_SIZE    (16)

/** UART 5 bit data mode */
#define UART_LCR_WLEN5       ((uint8_t)(0))
/** UART 6 bit data mode */
#define UART_LCR_WLEN6       ((uint8_t)(1 << 0))
/** UART 7 bit data mode */
#define UART_LCR_WLEN7       ((uint8_t)(2 << 0))
/** UART 8 bit data mode */
#define UART_LCR_WLEN8       ((uint8_t)(3 << 0))
/** UART Two Stop Bits Select */
#define UART_LCR_STOPBIT_SEL ((uint8_t)(1 << 2))
/** UART Parity Enable */
#define UART_LCR_PARITY_EN   ((uint8_t)(1 << 3))
/** UART Odd Parity Select */
#define UART_LCR_PARITY_ODD  ((uint8_t)(0))
/** UART Even Parity Select */
#define UART_LCR_PARITY_EVEN ((uint8_t)(1 << 4))
/** UART force 1 stick parity */
#define UART_LCR_PARITY_F_1  ((uint8_t)(2 << 4))
/** UART force 0 stick parity */
#define UART_LCR_PARITY_F_0  ((uint8_t)(3 << 4))
/** UART Transmission Break enable */
#define UART_LCR_BREAK_EN    ((uint8_t)(1 << 6))
/** UART Divisor Latches Access bit enable */
#define UART_LCR_DLAB_EN     ((uint8_t)(1 << 7))
/** UART line control bit mask */
#define UART_LCR_BITMASK     ((uint8_t)(0xFF))

/** Source for modem output pin DTR */
#define UART1_MCR_DTR_CTRL    ((uint8_t)(1 << 0))
/** Source for modem output pin RTS */
#define UART1_MCR_RTS_CTRL    ((uint8_t)(1 << 1))
/** Loop back mode select */
#define UART1_MCR_LOOPB_EN    ((uint8_t)(1 << 4))
/** Enable Auto RTS flow-control */
#define UART1_MCR_AUTO_RTS_EN ((uint8_t)(1 << 6))
/** Enable Auto CTS flow-control */
#define UART1_MCR_AUTO_CTS_EN ((uint8_t)(1 << 7))
/** UART1 bit mask value */
#define UART1_MCR_BITMASK     ((uint8_t)(0x0F3))

/** Line status register: Receive data ready*/
#define UART_LSR_RDR     ((uint8_t)(1 << 0))
/** Line status register: Overrun error*/
#define UART_LSR_OE      ((uint8_t)(1 << 1))
/** Line status register: Parity error*/
#define UART_LSR_PE      ((uint8_t)(1 << 2))
/** Line status register: Framing error*/
#define UART_LSR_FE      ((uint8_t)(1 << 3))
/** Line status register: Break interrupt*/
#define UART_LSR_BI      ((uint8_t)(1 << 4))
/** Line status register: Transmit holding register empty*/
#define UART_LSR_THRE    ((uint8_t)(1 << 5))
/** Line status register: Transmitter empty*/
#define UART_LSR_TEMT    ((uint8_t)(1 << 6))
/** Error in RX FIFO*/
#define UART_LSR_RXFE    ((uint8_t)(1 << 7))
/** UART Line status bit mask */
#define UART_LSR_BITMASK ((uint8_t)(0xFF))

/** Set upon state change of input CTS */
#define UART1_MSR_DELTA_CTS ((uint8_t)(1 << 0))
/** Set upon state change of input DSR */
#define UART1_MSR_DELTA_DSR ((uint8_t)(1 << 1))
/** Set upon low to high transition of input RI */
#define UART1_MSR_LO2HI_RI  ((uint8_t)(1 << 2))
/** Set upon state change of input DCD */
#define UART1_MSR_DELTA_DCD ((uint8_t)(1 << 3))
/** Clear To Send State */
#define UART1_MSR_CTS       ((uint8_t)(1 << 4))
/** Data Set Ready State */
#define UART1_MSR_DSR       ((uint8_t)(1 << 5))
/** Ring Indicator State */
#define UART1_MSR_RI        ((uint8_t)(1 << 6))
/** Data Carrier Detect State */
#define UART1_MSR_DCD       ((uint8_t)(1 << 7))
/** MSR register bit-mask value */
#define UART1_MSR_BITMASK   ((uint8_t)(0xFF))

/** UART Scratch Pad bit mask */
#define UART_SCR_BIMASK ((uint8_t)(0xFF))

/** UART Auto-baud start */
#define UART_ACR_START        ((uint32_t)(1 << 0))
/** UART Auto baudrate Mode 1 */
#define UART_ACR_MODE         ((uint32_t)(1 << 1))
/** UART Auto baudrate restart */
#define UART_ACR_AUTO_RESTART ((uint32_t)(1 << 2))
/** UART End of auto-baud interrupt clear */
#define UART_ACR_ABEOINT_CLR  ((uint32_t)(1 << 8))
/** UART Auto-baud time-out interrupt clear */
#define UART_ACR_ABTOINT_CLR  ((uint32_t)(1 << 9))
/** UART Auto Baudrate register bit mask */
#define UART_ACR_BITMASK      ((uint32_t)(0x307))

/** IrDA mode enable */
#define UART_ICR_IRDAEN      ((uint32_t)(1 << 0))
/** IrDA serial input inverted */
#define UART_ICR_IRDAINV     ((uint32_t)(1 << 1))
/** IrDA fixed pulse width mode */
#define UART_ICR_FIXPULSE_EN ((uint32_t)(1 << 2))
/** PulseDiv - Configures the pulse when FixPulseEn = 1 */
#define UART_ICR_PULSEDIV(n) ((uint32_t)((n & 0x07) << 3))
/** UART IRDA bit mask */
#define UART_ICR_BITMASK     ((uint32_t)(0x3F))

/** Baud-rate generation pre-scaler divisor */
#define UART_FDR_DIVADDVAL(n) ((uint32_t)(n & 0x0F))
/** Baud-rate pre-scaler multiplier value */
#define UART_FDR_MULVAL(n)    ((uint32_t)((n << 4) & 0xF0))
/** UART Fractional Divider register bit mask */
#define UART_FDR_BITMASK      ((uint32_t)(0xFF))

/** Transmit enable bit */
#define UART_TER_TXEN    ((uint8_t)(1 << 7))
/** UART Transmit Enable Register bit mask */
#define UART_TER_BITMASK ((uint8_t)(0x80))

/** RS-485/EIA-485 Normal Multi-drop Mode (NMM) is disabled */
#define UART1_RS485CTRL_NMM_EN   ((uint32_t)(1 << 0))
/** The receiver is disabled */
#define UART1_RS485CTRL_RX_DIS   ((uint32_t)(1 << 1))
/** Auto Address Detect (AAD) is enabled */
#define UART1_RS485CTRL_AADEN    ((uint32_t)(1 << 2))
/** If direction control is enabled (bit DCTRL = 1), pin DTR is used for direction control */
#define UART1_RS485CTRL_SEL_DTR  ((uint32_t)(1 << 3))
/** Enable Auto Direction Control */
#define UART1_RS485CTRL_DCTRL_EN ((uint32_t)(1 << 4))
/** This bit reverses the polarity of the direction control signal on the RTS (or DTR) pin. */
#define UART1_RS485CTRL_OINV_1   ((uint32_t)(1 << 5))
/** RS485 control bit-mask value */
#define UART1_RS485CTRL_BITMASK  ((uint32_t)(0x3F))

/** Bit mask value */
#define UART1_RS485ADRMATCH_BITMASK ((uint8_t)(0xFF))

/** Bit mask value */
#define UART1_RS485DLY_BITMASK ((uint8_t)(0xFF))

/** Reflects the current level of the UART receiver FIFO */
#define UART_FIFOLVL_RXFIFOLVL(n) ((uint32_t)(n & 0x0F))
/** Reflects the current level of the UART transmitter FIFO */
#define UART_FIFOLVL_TXFIFOLVL(n) ((uint32_t)((n >> 8) & 0x0F))
/** UART FIFO Level Register bit mask */
#define UART_FIFOLVL_BITMASK      ((uint32_t)(0x0F0F))

/* ---------------- CHECK PARAMETER DEFINITIONS ---------------------------- */

/** Macro to check the input UART_AUTOBAUD_INTSTAT parameters */
#define PARAM_UART_AUTOBAUD_INTSTAT(ABIntStat) \
    ((ABIntStat == UART_AUTOBAUD_INTSTAT_ABEO) || (ABIntStat == UART_AUTOBAUD_INTSTAT_ABTO))

/* Macro to check the input UART1_SignalState parameters */
#define PARAM_UART1_SIGNALSTATE(x) ((x == INACTIVE) || (x == ACTIVE))

/** Macro to check the input PARAM_UART1_MODEM_PIN parameters */
#define PARAM_UART1_MODEM_PIN(x) ((x == UART1_MODEM_PIN_DTR) || (x == UART1_MODEM_PIN_RTS))

/** Macro to check the direction control pin type */
#define PARAM_UART_RS485_DIRCTRL_PIN(x) \
    ((x == UART1_RS485_DIRCTRL_RTS) || (x == UART1_RS485_DIRCTRL_DTR))

/* Macro to determine if it is valid UART port number */
#define PARAM_UARTx(x)                                                                           \
    ((((uint32_t*)x) == ((uint32_t*)LPC_UART0)) || (((uint32_t*)x) == ((uint32_t*)LPC_UART1)) || \
     (((uint32_t*)x) == ((uint32_t*)LPC_UART2)) || (((uint32_t*)x) == ((uint32_t*)LPC_UART3)))
#define PARAM_UART_IrDA(x)   (((uint32_t*)x) == ((uint32_t*)LPC_UART3))
#define PARAM_UART1_MODEM(x) (((uint32_t*)x) == ((uint32_t*)LPC_UART1))

/** Macro to check the input value for UART1_RS485_CFG_MATCHADDRVALUE parameter */
#define PARAM_UART1_RS485_CFG_MATCHADDRVALUE(x) ((x < 0xFF))

/** Macro to check the input value for UART1_RS485_CFG_DELAYVALUE parameter */
#define PARAM_UART1_RS485_CFG_DELAYVALUE(x) ((x < 0xFF))

/**
 * @}
 */

/* Public Types --------------------------------------------------------------- */
/** @defgroup UART_Public_Types UART Public Types
 * @{
 */

/**
 * @brief UART Databit type definitions
 */
typedef enum {
    UART_DBITS_5 = 0,
    UART_DBITS_6,
    UART_DBITS_7,
    UART_DBITS_8
} UART_DATABITS;
/** Check the input UART_BITS parameters */
#define PARAM_UART_DBITS(dbit) (((dbit) >= UART_DBITS_5) && ((dbit) <= UART_DBITS_8))

/**
 * @brief UART Stop bits type definitions
 */
typedef enum {
    UART_STOPBIT_1 = 0,
    UART_STOPBIT_2,
} UART_STOPBITS;
/** Check the input UART_STOPBITS parameters */
#define PARAM_UART_STOPBITS(stopbit) ((stopbit == UART_STOPBIT_1) || (stopbit == UART_STOPBIT_2))

/**
 * @brief UART Parity type definitions
 */
typedef enum {
    UART_PARITY_NONE = 0,
    UART_PARITY_ODD,
    UART_PARITY_EVEN,
    UART_PARITY_1,
    UART_PARITY_0
} UART_PARITY;
/** Check the input UART_PARITY parameters */
#define PARAM_UART_PARITY(parity) (((parity) >= UART_PARITY_NONE) && ((parity) <= UART_PARITY_0))

/**
 * @brief FIFO Level type definitions
 */
typedef enum {
    UART_FIFO_TRGLEV0 = 0,
    UART_FIFO_TRGLEV1,
    UART_FIFO_TRGLEV2,
    UART_FIFO_TRGLEV3
} UART_FIFO_TRGLVL;
/** Check the input UART_FIFO_TRGLVL parameters */
#define PARAM_UART_FIFO_TRGLVL(lvl) (((lvl) >= UART_FIFO_TRGLEV0) && ((lvl) <= UART_FIFO_TRGLEV3))

/**
 * @brief UART Interrupt Type definitions
 */
typedef enum {
    UART_INT_RBR = 0,
    UART_INT_THRE,
    UART_INT_RLS,
    UART1_INT_MS,
    UART1_INT_CTS,
    UART_INT_ABEO,
    UART_INT_ABTO
} UART_INT;
/** Check the input UART_INTCFG parameters - valid for both UART and UART1 */
#define PARAM_UART_INTCFG(IntCfg)  (((IntCfg) >= UART_INT_RBR) && ((IntCfg) <= UART_INT_ABTO))
/** Check the input UART_INTCFG parameters - valid for UART1 only */
#define PARAM_UART1_INTCFG(IntCfg) ((IntCfg == UART1_INT_MS) || (IntCfg == UART1_INT_CTS))

/**
 * @brief UART Line Status Type definition
 */
typedef enum {
    UART_LINESTAT_RDR  = UART_LSR_RDR,  /*!<Line status register: Receive data ready*/
    UART_LINESTAT_OE   = UART_LSR_OE,   /*!<Line status register: Overrun error*/
    UART_LINESTAT_PE   = UART_LSR_PE,   /*!<Line status register: Parity error*/
    UART_LINESTAT_FE   = UART_LSR_FE,   /*!<Line status register: Framing error*/
    UART_LINESTAT_BI   = UART_LSR_BI,   /*!<Line status register: Break interrupt*/
    UART_LINESTAT_THRE = UART_LSR_THRE, /*!<Line status register: Transmit holding register empty*/
    UART_LINESTAT_TEMT = UART_LSR_TEMT, /*!<Line status register: Transmitter empty*/
    UART_LINESTAT_RXFE = UART_LSR_RXFE  /*!<Error in RX FIFO*/
} UART_LS_Type;

/**
 * @brief UART Auto-baudrate mode type definition
 */
typedef enum {
    UART_AB_MODE0 = 0, /**< UART Auto baudrate Mode 0 */
    UART_AB_MODE1,     /**< UART Auto baudrate Mode 1 */
} UART_AB_MODE;
/** Check the input UART_AUTOBAUD_MODE parameters */
#define PARAM_UART_AUTOBAUD_MODE(ABmode) ((ABmode == UART_AB_MODE0) || (ABmode == UART_AB_MODE1))

/**
 * @brief Auto Baudrate mode configuration type definition
 */
typedef struct {
    UART_AB_MODE abMode;         /**< Autobaudrate mode */
    FunctionalState autoRestart; /**< Auto Restart state */
} UART_AB_CFG_Type;

/**
 * @brief UART End of Auto-baudrate type definition
 */
typedef enum {
    UART_AUTOBAUD_INTSTAT_ABEO = UART_IIR_ABEO_INT, /**< UART End of auto-baud interrupt  */
    UART_AUTOBAUD_INTSTAT_ABTO = UART_IIR_ABTO_INT  /**< UART Auto-baud time-out interrupt  */
} UART_ABEO_Type;

/**
 * UART IrDA Control type Definition
 */
typedef enum {
    UART_IrDA_PULSEDIV2 = 0, /**< Pulse width = 2 * Tpclk
                                        - Configures the pulse when FixPulseEn = 1 */
    UART_IrDA_PULSEDIV4,     /**< Pulse width = 4 * Tpclk
                                        - Configures the pulse when FixPulseEn = 1 */
    UART_IrDA_PULSEDIV8,     /**< Pulse width = 8 * Tpclk
                                        - Configures the pulse when FixPulseEn = 1 */
    UART_IrDA_PULSEDIV16,    /**< Pulse width = 16 * Tpclk
                                        - Configures the pulse when FixPulseEn = 1 */
    UART_IrDA_PULSEDIV32,    /**< Pulse width = 32 * Tpclk
                                        - Configures the pulse when FixPulseEn = 1 */
    UART_IrDA_PULSEDIV64,    /**< Pulse width = 64 * Tpclk
                                        - Configures the pulse when FixPulseEn = 1 */
    UART_IrDA_PULSEDIV128,   /**< Pulse width = 128 * Tpclk
                                        - Configures the pulse when FixPulseEn = 1 */
    UART_IrDA_PULSEDIV256    /**< Pulse width = 256 * Tpclk
                                        - Configures the pulse when FixPulseEn = 1 */
} UART_IrDA_PULSE_Type;
/** Check the input UART_IrDA_PULSEDIV parameters */
#define PARAM_UART_IrDA_PULSEDIV(PulseDiv) \
    (((PulseDiv) >= UART_IrDA_PULSEDIV2) && ((PulseDiv) <= UART_IrDA_PULSEDIV256))

/**
 * @brief UART1 Full modem -
 * Signal states definition
 */
typedef enum {
    INACTIVE = 0,        /* In-active state */
    ACTIVE   = !INACTIVE /* Active state */
} UART1_SignalState;

/**
 * @brief UART modem status type definition
 */
typedef enum {
    UART1_MODEM_STAT_DELTA_CTS = UART1_MSR_DELTA_CTS, /*!< Set upon state change of input CTS */
    UART1_MODEM_STAT_DELTA_DSR = UART1_MSR_DELTA_DSR, /*!< Set upon state change of input DSR */
    UART1_MODEM_STAT_LO2HI_RI =
        UART1_MSR_LO2HI_RI, /*!< Set upon low to high transition of input RI */
    UART1_MODEM_STAT_DELTA_DCD = UART1_MSR_DELTA_DCD, /*!< Set upon state change of input DCD */
    UART1_MODEM_STAT_CTS       = UART1_MSR_CTS,       /*!< Clear To Send State */
    UART1_MODEM_STAT_DSR       = UART1_MSR_DSR,       /*!< Data Set Ready State */
    UART1_MODEM_STAT_RI        = UART1_MSR_RI,        /*!< Ring Indicator State */
    UART1_MODEM_STAT_DCD       = UART1_MSR_DCD        /*!< Data Carrier Detect State */
} UART_MODEM_STAT_type;

/**
 * @brief Modem output pin type definition
 */
typedef enum {
    UART1_MODEM_PIN_DTR = 0, /*!< Source for modem output pin DTR */
    UART1_MODEM_PIN_RTS      /*!< Source for modem output pin RTS */
} UART_MODEM_PIN_Type;

/**
 * @brief UART Modem mode type definition
 */
typedef enum {
    UART1_MODEM_MODE_LOOPBACK = 0, /*!< Loop back mode select */
    UART1_MODEM_MODE_AUTO_RTS,     /*!< Enable Auto RTS flow-control */
    UART1_MODEM_MODE_AUTO_CTS      /*!< Enable Auto CTS flow-control */
} UART_MODEM_MODE_Type;
/** Macro to check the input PARAM_UART1_MODEM_MODE parameters */
#define PARAM_UART1_MODEM_MODE(x) \
    (((x) >= UART1_MODEM_MODE_LOOPBACK) && ((x) <= UART1_MODEM_MODE_AUTO_CTS))

/**
 * @brief UART Direction Control Pin type definition
 */
typedef enum {
    UART1_RS485_DIRCTRL_RTS = 0, /**< Pin RTS is used for direction control */
    UART1_RS485_DIRCTRL_DTR      /**< Pin DTR is used for direction control */
} UART_RS485_DIRCTRL_PIN_Type;

/**
 * @brief UART pin selection options.
 */
typedef enum {
    UART_TX0_P0_2 = 0,
    UART_RX0_P0_3,
    UART_TX1_P0_15,
    UART_RX1_P0_16,
    UART_TX1_P2_0,
    UART_RX1_P2_1,
    UART_TX2_P0_10,
    UART_RX2_P0_11,
    UART_TX2_P2_8,
    UART_RX2_P2_9,
    UART_TX3_P0_0,
    UART_RX3_P0_1,
    UART_TX3_P0_25,
    UART_RX3_P0_26,
    UART_TX3_P4_28,
    UART_RX3_P4_29,
} UART_PIN_OPTION;
/** Check UART pin option parameter. */
#define PARAM_UART_PIN_OPTION(OPT) ((OPT >= UART_TX0_P0_2) && (OPT <= UART_RX3_P4_29))

/**
 * @brief UART Configuration Structure definition
 */
typedef struct {
    uint32_t baudRate;      /**< UART baud rate */
    UART_PARITY parity;     /**< Parity selection, should be:
                                 - UART_PARITY_NONE: No parity
                                - UART_PARITY_ODD: Odd parity
                                - UART_PARITY_EVEN: Even parity
                                - UART_PARITY_SP_1: Forced "1" stick parity
                                - UART_PARITY_SP_0: Forced "0" stick parity
                                */
    UART_DATABITS dataBits; /**< Number of data bits, should be:
                               - UART_DATABIT_5: UART 5 bit data mode
                               - UART_DATABIT_6: UART 6 bit data mode
                               - UART_DATABIT_7: UART 7 bit data mode
                               - UART_DATABIT_8: UART 8 bit data mode
                               */
    UART_STOPBITS stopBits; /**< Number of stop bits, should be:
                               - UART_STOPBIT_1: UART 1 Stop Bits Select
                               - UART_STOPBIT_2: UART 2 Stop Bits Select
                               */
} UART_CFG_T;

/**
 * @brief UART FIFO Configuration Structure definition
 */

typedef struct {
    FunctionalState resetRxBuf; /**< Reset Rx FIFO command state , should be:
                                         - ENABLE: Reset Rx FIFO in UART
                                         - DISABLE: Do not reset Rx FIFO  in UART
                                         */
    FunctionalState resetTxBuf; /**< Reset Tx FIFO command state , should be:
                                         - ENABLE: Reset Tx FIFO in UART
                                         - DISABLE: Do not reset Tx FIFO  in UART
                                         */
    FunctionalState dmaMode;    /**< DMA mode, should be:
                                         - ENABLE: Enable DMA mode in UART
                                         - DISABLE: Disable DMA mode in UART
                                         */
    UART_FIFO_TRGLVL level;     /**< Rx FIFO trigger level, should be:
                                            - UART_FIFO_TRGLEV0: UART FIFO trigger level 0: 1 character
                                            - UART_FIFO_TRGLEV1: UART FIFO trigger level 1: 4 character
                                            - UART_FIFO_TRGLEV2: UART FIFO trigger level 2: 8 character
                                            - UART_FIFO_TRGLEV3: UART FIFO trigger level 3: 14 character
                                            */
} UART_FIFO_CFG_T;

/**
 * @brief UART1 Full modem - RS485 Control configuration type
 */
typedef struct {
    FunctionalState multiDropMode;          /*!< Normal MultiDrop mode State:
                                                             - ENABLE: Enable this function.
                                                             - DISABLE: Disable this function. */
    FunctionalState rxState;                /*!< Receiver State:
                                                  - ENABLE: Enable Receiver.
                                                  - DISABLE: Disable Receiver. */
    FunctionalState autoAddrDetect;         /*!< Auto Address Detect mode state:
                                                   - ENABLE: ENABLE this function.
                                                   - DISABLE: Disable this function. */
    FunctionalState autoDirCtrl;            /*!< Auto Direction Control State:
                                                   - ENABLE: Enable this function.
                                                   - DISABLE: Disable this function. */
    UART_RS485_DIRCTRL_PIN_Type dirCtrlPin; /*!< If direction control is enabled, state:
                                             - UART1_RS485_DIRCTRL_RTS:
                                             pin RTS is used for direction control.
                                             - UART1_RS485_DIRCTRL_DTR:
                                             pin DTR is used for direction control. */
    SetState dirCtrlPol;                    /*!< Polarity of the direction control signal on
                                                   the RTS (or DTR) pin:
                                                   - RESET: The direction control pin will be driven
                                                   to logic "0" when the transmitter has data to be sent.
                                                   - SET: The direction control pin will be driven
                                                   to logic "1" when the transmitter has data to be sent. */
    uint8_t matchAddrValue; /*!< address match value for RS-485/EIA-485 mode, 8-bit long */
    uint8_t delayValue;     /*!< delay time is in periods of the baud clock, 8-bit long */
} UART1_RS485_CTRLCFG_T;

/**
 * @}
 */

/* Public Functions ----------------------------------------------------------- */
/** @defgroup UART_Public_Functions UART Public Functions
 * @{
 */

/**
 * @brief Configures the physical pins for UART transmission and reception.
 *
 * Maps TX and RX signals to MCU pins based on the selected option. It configures the appropriate
 * function and sets the resistor mode to Tristate to allow the external transceiver or level
 * shifter to drive the lines.
 *
 * @param option Selection from the available UART_PIN_OPTION mappings.
 */
void UART_PinConfig(UART_PIN_OPTION option);

/**
 * @brief Initializes the UART peripheral with specified frame parameters.
 *
 * Powers up the UART block, resets the internal FIFOs, and clears stale data from the Receive
 * Buffer Register (RBR). It configures the baud rate via the Divisor Latch registers and
 * establishes the frame format (Data Bits, Parity, and Stop Bits) in the Line Control Register
 * (LCR).
 *
 * @param UARTx   Pointer to the UART peripheral (UARTx [0...3]).
 * @param uartCfg Pointer to a UART_CFG_T structure with configuration settings.
 */
void UART_Init(LPC_UART_TypeDef* UARTx, const UART_CFG_T* uartCfg);

/**
 * @brief De-initializes the UART peripheral.
 *
 * Disables the transmitter via the TER register and subsequently cuts the peripheral clock in the
 * PCONP register. This ensures the peripheral enters its lowest power state.
 *
 * @param UARTx Pointer to the UART peripheral (UARTx [0...3]).
 */
void UART_DeInit(LPC_UART_TypeDef* UARTx);

/**
 * @brief Configures the internal FIFO buffers and trigger levels.
 *
 * Sets the FIFO Control Register (FCR) to enable the 16-byte RX and TX buffers. It allows for
 * resetting the buffers, enabling DMA mode, and defining the RX trigger level that generates an
 * interrupt or DMA request.
 *
 * @param UARTx   Pointer to the UART peripheral (UARTx [0...3]).
 * @param fifoCfg Pointer to a UART_FIFO_CFG_T structure with FIFO settings.
 */
void UART_FIFOConfig(LPC_UART_TypeDef* UARTx, const UART_FIFO_CFG_T* fifoCfg);

/**
 * @brief Writes a single byte to the Transmit Holding Register (THR).
 *
 * Places one byte of data into the transmit FIFO. This function does not check if the FIFO is full;
 * it is a direct register write.
 *
 * @param UARTx Pointer to the UART peripheral (UARTx [0...3]).
 * @param data  The 8-bit data to be transmitted.
 */
void UART_SendByte(LPC_UART_TypeDef* UARTx, uint8_t data);

/**
 * @brief Reads a single byte from the Receiver Buffer Register (RBR).
 *
 * Retrieves the oldest character from the receiver FIFO. This function does not check if data is
 * available; it is a direct register read.
 *
 * @param UARTx Pointer to the UART peripheral (UARTx [0...3]).
 * @return The 8-bit data received.
 */
uint8_t UART_ReceiveByte(LPC_UART_TypeDef* UARTx);

/**
 * @brief Transmits a block of data via the UART.
 *
 * Transfers a buffer of a specified length. In BLOCKING mode, it waits for FIFO space to become
 * available or for a timeout to occur. In NONE_BLOCKING mode, it sends only as much as the current
 * FIFO capacity allows and returns.
 *
 * @param UARTx  Pointer to the UART peripheral (UARTx [0...3]).
 * @param txbuf  Pointer to the source data buffer.
 * @param buflen Number of bytes to transmit.
 * @param flag   Transfer mode (BLOCKING or NONE_BLOCKING).
 * @return Total number of bytes actually sent.
 */
uint32_t UART_Send(LPC_UART_TypeDef* UARTx, const uint8_t* txbuf, uint32_t buflen,
                   TRANSFER_BLOCK_Type flag);

/**
 * @brief Receives a block of data via the UART.
 *
 * Reads incoming data into a buffer. In BLOCKING mode, it waits for the Receiver Data Ready (RDR)
 * flag or a timeout. In NONE_BLOCKING mode, it reads only the data currently present in the RX
 * FIFO.
 *
 * @param UARTx  Pointer to the UART peripheral (UARTx [0...3]).
 * @param rxbuf  Pointer to the destination data buffer.
 * @param buflen Number of bytes to receive.
 * @param flag   Transfer mode (BLOCKING or NONE_BLOCKING).
 * @return Total number of bytes actually received.
 */
uint32_t UART_Receive(LPC_UART_TypeDef* UARTx, uint8_t* rxbuf, uint32_t buflen,
                      TRANSFER_BLOCK_Type flag);

/**
 * @brief Retrieves the Interrupt Identification and FIFO status.
 *
 * Reads the IIR register to determine the priority and source of a pending interrupt. It masks out
 * reserved bits to return only the interrupt ID and the FIFO enable status bits.
 *
 * @param UARTx Pointer to the UART peripheral (UARTx [0...3]).
 * @return The 32-bit value of the IIR register (masked for valid bits).
 */
uint32_t UART_GetIntId(LPC_UART_TypeDef* UARTx);

/**
 * @brief Retrieves the current status of the UART line.
 *
 * Returns the 8-bit Line Status Register (LSR) value, which provides information on data ready,
 * transmitter state, and error conditions (Parity, Framing, Break, and Overrun errors).
 *
 * @param UARTx Pointer to the UART peripheral (UARTx [0...3]).
 * @return Current LSR register value.
 */
uint8_t UART_GetLineStatus(LPC_UART_TypeDef* UARTx);

/**
 * @brief Enables or disables specific UART interrupt sources.
 *
 * Configures the Interrupt Enable Register (IER). Supports standard sources like RBR, THRE, and
 * Line Status for all UARTs, as well as Modem Status and CTS interrupts specific to UART1.
 *
 * @param UARTx     Pointer to the UART peripheral (UARTx [0...3]).
 * @param intType   The interrupt source to configure (e.g., UART_INT_RBR).
 * @param newState  ENABLE to activate the interrupt, DISABLE to deactivate it.
 */
void UART_IntConfig(LPC_UART_TypeDef* UARTx, UART_INT intType, FunctionalState newState);

/**
 * @brief Enables the UART transmitter.
 *
 * Sets the TXEN bit in the Transmit Enable Register (TER). When enabled, data written to the THR
 * will be shifted out to the physical TX pin.
 *
 * @param UARTx Pointer to the UART peripheral (UARTx [0...3]).
 */
void UART_TxEnable(LPC_UART_TypeDef* UARTx);

/**
 * @brief Disables the UART transmitter.
 *
 * Clears the TXEN bit in the TER register. This allows for safe halting of serial data
 * transmission.
 *
 * @param UARTx Pointer to the UART peripheral (UARTx [0...3]).
 */
void UART_TxDisable(LPC_UART_TypeDef* UARTx);

/**
 * @brief Checks if the UART transmitter is currently active.
 *
 * Evaluates the Transmitter Empty (TEMT) bit in the LSR register.
 *
 * @param UARTx Pointer to the UART peripheral (UARTx [0...3]).
 * @return SET if the transmitter or FIFO still contains data, RESET if completely idle.
 */
FlagStatus UART_CheckBusy(LPC_UART_TypeDef* UARTx);

/**
 * @brief Forces a continuous "break" condition on the TX line.
 *
 * Sets the Break Control bit in the Line Control Register (LCR). This drives the TX output to a
 * continuous spacing (logic 0) state until explicitly disabled.
 *
 * @param UARTx Pointer to the UART peripheral (UARTx [0...3]).
 */
void UART_ForceBreak(LPC_UART_TypeDef* UARTx);

/**
 * @brief Configures and enables the Auto-Baud rate detection logic.
 *
 * Activates the Auto-Baud Control Register (ACR) to automatically measure the incoming bit rate. It
 * resets the divisor latch registers and fractional divider to ensure a clean measurement
 * environment for synchronization.
 *
 * @param UARTx     Pointer to the UART peripheral (UARTx [0...3]).
 * @param abCfg     Pointer to an Auto-Baud configuration structure.
 * @param newState  ENABLE to start detection, DISABLE to stop.
 */
void UART_ABCmd(LPC_UART_TypeDef* UARTx, UART_AB_CFG_Type* abCfg, FunctionalState newState);

/**
 * @brief Clears pending Auto-Baud interrupts.
 *
 * Acknowledges the End of Auto-Baud (ABEO) or Auto-Baud Timeout (ABTO) flags by writing to the
 * Auto-Baud Control Register (ACR).
 *
 * @param UARTx     Pointer to the UART peripheral (UARTx [0...3]).
 * @param intType   The interrupt flag to clear (ABEO or ABTO).
 */
void UART_ABClearIntPending(LPC_UART_TypeDef* UARTx, UART_ABEO_Type intType);

/**
 * @brief Enables inversion of the IrDA input signal.
 *
 * Modifies the IrDA Control Register (ICR) to treat a high level as a space and a low level as a
 * mark.
 *
 * @param UARTx Pointer to the UART peripheral (IrDA supported).
 */
void UART_IrDAInvtInputEnable(LPC_UART_TypeDef* UARTx);

/**
 * @brief Disables inversion of the IrDA input signal.
 *
 * Modifies the IrDA Control Register (ICR) to treat a high level as a space and a low level as a
 * mark.
 *
 * @param UARTx Pointer to the UART peripheral.
 */
void UART_IrDAInvtInputDisable(LPC_UART_TypeDef* UARTx);

/**
 * @brief Configures the pulse width for IrDA communication.
 *
 * Sets the divider for the IrDA pulse width in the ICR register. This defines the duration of the
 * infrared pulse relative to the bit period.
 *
 * @param UARTx    Pointer to the UART peripheral. (UARTx [0,2,3]).
 * @param pulseDiv Pulse width divider selection.
 */
void UART_IrDAPulseDivConfig(LPC_UART_TypeDef* UARTx, UART_IrDA_PULSE_Type pulseDiv);

/**
 * @brief Configures Modem Control signals (DTR/RTS) for UART1.
 *
 * Drives the selected modem output pin to its active state (logic 0) by modifying the Modem Control
 * Register (MCR).
 *
 * @param pin The modem pin to activate (UART1_MODEM_PIN_DTR or RTS).
 */
void UART_ModemPinActive(UART_MODEM_PIN_Type pin);

/**
 * @brief Drives Modem Control signals (DTR/RTS) to their inactive state.
 *
 * Drives the selected modem output pin to its active state (logic 0) by modifying the Modem Control
 * Register (MCR).
 *
 * @param pin The modem pin to deactivate.
 */
void UART_ModemPinInactive(UART_MODEM_PIN_Type pin);

/**
 * @brief Enables specialized hardware modem modes for UART1.
 *
 * Configures the Modem Control Register (MCR) to activate Loopback mode
 * for internal testing or Auto-RTS/Auto-CTS for hardware-managed
 * flow control.
 *
 * @param mode The modem mode to enable (LOOPBACK, AUTO_RTS, or AUTO_CTS).
 */
void UART_ModemModeEnable(UART_MODEM_MODE_Type mode);

/**
 * @brief Disables specialized hardware modem modes for UART1.
 *
 * Clears the corresponding bits in the MCR to return the modem interface to standard manual
 * software control.
 *
 * @param mode The modem mode to deactivate.
 */
void UART_ModemModeDisable(UART_MODEM_MODE_Type mode);

/**
 * @brief Retrieves the current status of the modem interface signals.
 *
 * Reads the Modem Status Register (MSR), which provides the current state and "delta" (change)
 * flags for CTS, DSR, RI, and DCD lines.
 *
 * @param UARTx Pointer to the UART1 peripheral.
 * @return The 8-bit value of the MSR register.
 */
uint8_t UART_FullModemGetStatus(LPC_UART1_TypeDef* UARTx);

/**
 * @brief Configures UART1 for RS-485/EIA-485 communication.
 *
 * Sets up the RS485CTRL register to manage half-duplex direction control, multi-drop (9-bit) mode,
 * and automatic address detection. It also configures the Line Control Register (LCR) to support
 * the parity-based addressing scheme required for RS-485.
 *
 * @param cfg Pointer to a UART1_RS485_CTRLCFG_T structure with RS-485 settings.
 */
void UART_RS485Config(const UART1_RS485_CTRLCFG_T* cfg);

/**
 * @brief Enables the UART1 receiver in RS-485 mode.
 *
 * Clears the RX_DIS bit in the RS485CTRL register, allowing the peripheral to process incoming data
 * from the bus.
 */
void UART_RS485ReceiverEnable(void);

/**
 * @brief Disables the UART1 receiver in RS-485 mode.
 *
 * Sets the RX_DIS bit, preventing incoming data from filling the RX FIFO. This is often used during
 * transmission in half-duplex systems to avoid "echoing" the transmitted data back into the
 * receiver.
 */
void UART_RS485ReceiverDisable(void);

/**
 * @brief Sends a block of data or an address frame in RS-485 mode.
 *
 * Temporarily modifies the Line Control Register (LCR) to set the 9th bit (parity bit) based on
 * whether the frame is an address or data. It waits for the transmitter to be completely empty
 * (TEMT) before restoring the original LCR state to ensure the last byte is sent correctly.
 *
 * @param pDatFrm   Pointer to the data or address buffer.
 * @param size      Number of bytes to transmit.
 * @param isAddress Set to 1 to send as an Address frame, 0 for Data frame.
 * @return Total number of bytes successfully transmitted.
 */
uint32_t UART_RS485Send(const uint8_t* pDatFrm, uint32_t size, uint8_t isAddress);

/**
 * @brief Sends a single slave address frame.
 *
 * A specialized wrapper for UART_RS485Send that transmits one byte with the 9th bit set to '1',
 * signaling to all slaves on the bus that an address is being broadcast.
 *
 * @param slvAddr The 8-bit address of the target slave device.
 */
void UART_RS485SendSlvAddr(uint8_t slvAddr);

/**
 * @brief Sends a block of data frames.
 *
 * A specialized wrapper for UART_RS485Send that transmits data with the 9th bit set to '0'. Slaves
 * typically ignore these frames unless they have been previously addressed.
 *
 * @param pData Pointer to the data buffer.
 * @param size  Number of bytes to transmit.
 * @return Total number of bytes successfully transmitted.
 */
uint32_t UART_RS485SendData(uint8_t* pData, uint32_t size);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __LPC17XX_UART_H */

/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */
