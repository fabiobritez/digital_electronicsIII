/**
 * @file        lpc17xx_uart.c
 * @brief        Contains all functions support for UART firmware library on LPC17xx
 * @version        3.0
 * @date        18. June. 2010
 * @author        NXP MCU SW Application Team
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
/** @addtogroup UART
 * @{
 */

/* Includes ------------------------------------------------------------------- */
#include "lpc17xx_uart.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_pinsel.h"

/* If this source file built with example, the LPC17xx FW library configuration
 * file in each example directory ("lpc17xx_libcfg.h") must be included,
 * otherwise the default FW library configuration file must be included instead
 */
#ifdef __BUILD_WITH_EXAMPLE__
#include "lpc17xx_libcfg.h"
#else
#include "lpc17xx_libcfg_default.h"
#endif /* __BUILD_WITH_EXAMPLE__ */

#ifdef _UART

/* Private Functions ---------------------------------------------------------- */

static Status uart_set_divisors(LPC_UART_TypeDef* UARTx, uint32_t baudrate);

/**
 * @brief        Determines best dividers to get a target clock rate
 * @param[in]    UARTx    Pointer to selected UART peripheral, should be:
 *                 - LPC_UART0: UART0 peripheral
 *                 - LPC_UART1: UART1 peripheral
 *                 - LPC_UART2: UART2 peripheral
 *                 - LPC_UART3: UART3 peripheral
 * @param[in]    baudrate Desired UART baud rate.
 * @return         Error status, could be:
 *                 - SUCCESS
 *                 - ERROR
 */
static Status uart_set_divisors(LPC_UART_TypeDef* UARTx, uint32_t baudrate) {
    Status errorStatus = ERROR;

    uint32_t uClk         = 0;
    uint32_t calcBaudrate = 0;
    uint32_t temp         = 0;

    uint32_t mulFracDiv, dividerAddFracDiv;
    uint32_t diviser           = 0;
    uint32_t mulFracDivOptimal = 1;
    uint32_t dividerAddOptimal = 0;
    uint32_t diviserOptimal    = 0;

    uint32_t relativeError        = 0;
    uint32_t relativeOptimalError = 100000;

    /* get UART block clock */
    if (UARTx == (LPC_UART_TypeDef*)LPC_UART0) {
        uClk = CLKPWR_GetPCLK(CLKPWR_PCLKSEL_UART0);
    } else if (UARTx == (LPC_UART_TypeDef*)LPC_UART1) {
        uClk = CLKPWR_GetPCLK(CLKPWR_PCLKSEL_UART1);
    } else if (UARTx == (LPC_UART_TypeDef*)LPC_UART2) {
        uClk = CLKPWR_GetPCLK(CLKPWR_PCLKSEL_UART2);
    } else if (UARTx == (LPC_UART_TypeDef*)LPC_UART3) {
        uClk = CLKPWR_GetPCLK(CLKPWR_PCLKSEL_UART3);
    }

    uClk = uClk >> 4; /* div by 16 */
    /* In the Uart IP block, baud rate is calculated using FDR and DLL-DLM registers
     * The formula is :
     * BaudRate= uClk * (mulFracDiv/(mulFracDiv+dividerAddFracDiv) / (16 * (DLL)
     * It involves floating point calculations. That's the reason the formulae are adjusted with
     * Multiply and divide method.*/
    /* The value of mulFracDiv and dividerAddFracDiv should comply to the following expressions:
     * 0 < mulFracDiv <= 15, 0 <= dividerAddFracDiv <= 15 */
    for (mulFracDiv = 1; mulFracDiv <= 15; mulFracDiv++) {
        for (dividerAddFracDiv = 0; dividerAddFracDiv <= 15; dividerAddFracDiv++) {
            temp = (mulFracDiv * uClk) / ((mulFracDiv + dividerAddFracDiv));

            diviser = temp / baudrate;
            if ((temp % baudrate) > (baudrate / 2))
                diviser++;

            if (diviser > 2 && diviser < 65536) {
                calcBaudrate = temp / diviser;

                if (calcBaudrate <= baudrate)
                    relativeError = baudrate - calcBaudrate;
                else
                    relativeError = calcBaudrate - baudrate;

                if ((relativeError < relativeOptimalError)) {
                    mulFracDivOptimal    = mulFracDiv;
                    dividerAddOptimal    = dividerAddFracDiv;
                    diviserOptimal       = diviser;
                    relativeOptimalError = relativeError;
                    if (relativeError == 0)
                        break;
                }
            } /* End of if */
        } /* end of inner for loop */
        if (relativeError == 0)
            break;
    } /* end of outer for loop  */

    if (relativeOptimalError < ((baudrate * UART_ACCEPTED_BAUDRATE_ERROR) / 100)) {
        if (((LPC_UART1_TypeDef*)UARTx) == LPC_UART1) {
            ((LPC_UART1_TypeDef*)UARTx)->LCR |= UART_LCR_DLAB_EN;
            ((LPC_UART1_TypeDef*)UARTx)->/*DLIER.*/ DLM   = UART_LOAD_DLM(diviserOptimal);
            ((LPC_UART1_TypeDef*)UARTx)->/*RBTHDLR.*/ DLL = UART_LOAD_DLL(diviserOptimal);
            /* Then reset DLAB bit */
            ((LPC_UART1_TypeDef*)UARTx)->LCR &= (~UART_LCR_DLAB_EN) & UART_LCR_BITMASK;
            ((LPC_UART1_TypeDef*)UARTx)->FDR =
                (UART_FDR_MULVAL(mulFracDivOptimal) | UART_FDR_DIVADDVAL(dividerAddOptimal)) &
                UART_FDR_BITMASK;
        } else {
            UARTx->LCR |= UART_LCR_DLAB_EN;
            UARTx->/*DLIER.*/ DLM   = UART_LOAD_DLM(diviserOptimal);
            UARTx->/*RBTHDLR.*/ DLL = UART_LOAD_DLL(diviserOptimal);
            /* Then reset DLAB bit */
            UARTx->LCR &= (~UART_LCR_DLAB_EN) & UART_LCR_BITMASK;
            UARTx->FDR =
                (UART_FDR_MULVAL(mulFracDivOptimal) | UART_FDR_DIVADDVAL(dividerAddOptimal)) &
                UART_FDR_BITMASK;
        }
        errorStatus = SUCCESS;
    }

    return errorStatus;
}

/* End of Private Functions ---------------------------------------------------- */

/* Public Functions ----------------------------------------------------------- */
/** @addtogroup UART_Public_Functions
 * @{
 */

void UART_PinConfig(UART_PIN_OPTION option) {
    CHECK_PARAM(PARAM_UART_PIN_OPTION(option));

    static const PINSEL_CFG_T PinCfg[16] = {
        {PORT_0, PIN_2, PINSEL_FUNC_01, PINSEL_TRISTATE},
        {PORT_0, PIN_3, PINSEL_FUNC_01, PINSEL_TRISTATE},
        {PORT_0, PIN_15, PINSEL_FUNC_01, PINSEL_TRISTATE},
        {PORT_0, PIN_16, PINSEL_FUNC_01, PINSEL_TRISTATE},
        {PORT_2, PIN_0, PINSEL_FUNC_10, PINSEL_TRISTATE},
        {PORT_2, PIN_1, PINSEL_FUNC_10, PINSEL_TRISTATE},
        {PORT_0, PIN_10, PINSEL_FUNC_01, PINSEL_TRISTATE},
        {PORT_0, PIN_11, PINSEL_FUNC_01, PINSEL_TRISTATE},
        {PORT_2, PIN_8, PINSEL_FUNC_10, PINSEL_TRISTATE},
        {PORT_2, PIN_9, PINSEL_FUNC_10, PINSEL_TRISTATE},
        {PORT_0, PIN_0, PINSEL_FUNC_10, PINSEL_TRISTATE},
        {PORT_0, PIN_1, PINSEL_FUNC_10, PINSEL_TRISTATE},
        {PORT_0, PIN_25, PINSEL_FUNC_11, PINSEL_TRISTATE},
        {PORT_0, PIN_26, PINSEL_FUNC_11, PINSEL_TRISTATE},
        {PORT_4, PIN_28, PINSEL_FUNC_11, PINSEL_TRISTATE},
        {PORT_4, PIN_29, PINSEL_FUNC_11, PINSEL_TRISTATE},
    };

    PINSEL_ConfigPin(&PinCfg[option]);
}

void UART_Init(LPC_UART_TypeDef* UARTx, const UART_CFG_T* uartCfg) {
    CHECK_PARAM(PARAM_UARTx(UARTx));
    CHECK_PARAM(PARAM_UART_DBITS(uartCfg->dataBits));
    CHECK_PARAM(PARAM_UART_STOPBITS(uartCfg->stopBits));
    CHECK_PARAM(PARAM_UART_PARITY(uartCfg->parity));

    if (UARTx == UART0) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCUART0, ENABLE);
    } else if (UARTx == UART1) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCUART1, ENABLE);
    } else if (UARTx == UART2) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCUART2, ENABLE);
    } else if (UARTx == UART3) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCUART3, ENABLE);
    }

    UARTx->FCR = UART_FCR_FIFO_EN | UART_FCR_RX_RS | UART_FCR_TX_RS;
    UARTx->FCR = 0;

    while (UARTx->LSR & UART_LSR_RDR) {
        (void)UARTx->RBR;
    }

    UARTx->TER = UART_TER_TXEN;
    while (!(UARTx->LSR & UART_LSR_THRE)) {}
    UARTx->TER = 0;

    UARTx->IER = 0;
    UARTx->LCR = 0;
    UARTx->ACR = 0;
    (void)UARTx->LSR;

    if (UARTx == UART1) {
        LPC_UART1->MCR       = 0;
        LPC_UART1->RS485CTRL = 0;
        LPC_UART1->RS485DLY  = 0;
        LPC_UART1->ADRMATCH  = 0;
        (void)LPC_UART1->MSR;
    } else {
        UARTx->ICR = 0;
    }

    uart_set_divisors(UARTx, uartCfg->baudRate);

    uint32_t lineCtrl = 0;

    switch (uartCfg->dataBits) {
        case UART_DBITS_5: lineCtrl |= UART_DBITS_5; break;
        case UART_DBITS_6: lineCtrl |= UART_DBITS_6; break;
        case UART_DBITS_7: lineCtrl |= UART_DBITS_7; break;
        case UART_DBITS_8: lineCtrl |= UART_DBITS_8; break;
        default: break;
    }

    if (uartCfg->parity != UART_PARITY_NONE) {
        lineCtrl |= UART_LCR_PARITY_EN;
        switch (uartCfg->parity) {
            case UART_PARITY_ODD: lineCtrl |= UART_LCR_PARITY_ODD; break;
            case UART_PARITY_EVEN: lineCtrl |= UART_LCR_PARITY_EVEN; break;
            case UART_PARITY_1: lineCtrl |= UART_LCR_PARITY_F_1; break;
            case UART_PARITY_0: lineCtrl |= UART_LCR_PARITY_F_0; break;
            default: break;
        }
    }

    switch (uartCfg->stopBits) {
        case UART_STOPBIT_1: lineCtrl |= UART_LCR_STOPBITS_1; break;
        case UART_STOPBIT_2: lineCtrl |= UART_LCR_STOPBITS_2; break;
        default: break;
    }

    UARTx->LCR = lineCtrl & UART_LCR_BITMASK;
}

void UART_DeInit(LPC_UART_TypeDef* UARTx) {
    CHECK_PARAM(PARAM_UARTx(UARTx));

    UART_TxDisable(UARTx);

    if (UARTx == UART0) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCUART0, DISABLE);
    } else if (UARTx == UART1) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCUART1, DISABLE);
    } else if (UARTx == UART2) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCUART2, DISABLE);
    } else if (UARTx == UART3) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCUART3, DISABLE);
    }
}

void UART_FIFOConfig(LPC_UART_TypeDef* UARTx, const UART_FIFO_CFG_T* fifoCfg) {
    CHECK_PARAM(PARAM_UARTx(UARTx));
    CHECK_PARAM(PARAM_UART_FIFO_TRGLVL(fifoCfg->level));

    uint8_t ctrl = UART_FCR_FIFO_EN;

    switch (fifoCfg->level) {
        case UART_FIFO_TRGLEV0: ctrl |= UART_FCR_TRG_LEV0; break;
        case UART_FIFO_TRGLEV1: ctrl |= UART_FCR_TRG_LEV1; break;
        case UART_FIFO_TRGLEV2: ctrl |= UART_FCR_TRG_LEV2; break;
        case UART_FIFO_TRGLEV3: ctrl |= UART_FCR_TRG_LEV3; break;
        default: break;
    }

    if (fifoCfg->resetTxBuf == ENABLE) {
        ctrl |= UART_FCR_TX_RS;
    }
    if (fifoCfg->resetRxBuf == ENABLE) {
        ctrl |= UART_FCR_RX_RS;
    }
    if (fifoCfg->dmaMode == ENABLE) {
        ctrl |= UART_FCR_DMAMODE_SEL;
    }

    UARTx->FCR = (uint32_t)(ctrl & UART_FCR_BITMASK);
}

void UART_SendByte(LPC_UART_TypeDef* UARTx, uint8_t data) {
    CHECK_PARAM(PARAM_UARTx(UARTx));

    UARTx->THR = data & UART_THR_MASKBIT;
}

uint8_t UART_ReceiveByte(LPC_UART_TypeDef* UARTx) {
    CHECK_PARAM(PARAM_UARTx(UARTx));

    return UARTx->RBR & UART_RBR_MASKBIT;
}

uint32_t UART_Send(LPC_UART_TypeDef* UARTx, const uint8_t* txbuf, uint32_t buflen,
                   TRANSFER_BLOCK_Type flag) {
    uint32_t bSent       = 0;
    const uint8_t* pChar = txbuf;

    while (bSent < buflen) {
        if (!(UARTx->LSR & UART_LSR_THRE)) {
            if (flag == NONE_BLOCKING) {
                break;
            }

            uint32_t timeOut = UART_BLOCKING_TIMEOUT;
            while (!(UARTx->LSR & UART_LSR_THRE) && --timeOut) {}

            if (timeOut == 0) {
                break;
            }
        }

        uint32_t fifo_available = UART_TX_FIFO_SIZE;
        while (fifo_available > 0 && bSent < buflen) {
            UART_SendByte(UARTx, *pChar++);
            fifo_available--;
            bSent++;
        }
    }
    return bSent;
}

uint32_t UART_Receive(LPC_UART_TypeDef* UARTx, uint8_t* rxbuf, uint32_t buflen,
                      TRANSFER_BLOCK_Type flag) {
    uint32_t bRecv = 0;
    uint8_t* pChar = rxbuf;

    while (bRecv < buflen) {
        if (!(UARTx->LSR & UART_LSR_RDR)) {
            if (flag == NONE_BLOCKING) {
                break;
            }

            uint32_t timeOut = UART_BLOCKING_TIMEOUT;
            while (!(UARTx->LSR & UART_LSR_RDR) && --timeOut) {}

            if (timeOut == 0) {
                break;
            }
        }

        *pChar++ = UART_ReceiveByte(UARTx);
        bRecv++;
    }

    return bRecv;
}

uint32_t UART_GetIntId(LPC_UART_TypeDef* UARTx) {
    CHECK_PARAM(PARAM_UARTx(UARTx));
    return UARTx->IIR & 0x03CF;
}

uint8_t UART_GetLineStatus(LPC_UART_TypeDef* UARTx) {
    CHECK_PARAM(PARAM_UARTx(UARTx));

    return UARTx->LSR & UART_LSR_BITMASK;
}

void UART_IntConfig(LPC_UART_TypeDef* UARTx, UART_INT intType, FunctionalState newState) {
    CHECK_PARAM(PARAM_UARTx(UARTx));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(newState));
    if (UARTx == UART1) {
        CHECK_PARAM(PARAM_UART_INTCFG(intType) || PARAM_UART1_INTCFG(intType));
    } else {
        CHECK_PARAM(PARAM_UART_INTCFG(intType));
    }

    uint32_t tmp = 0;

    switch (intType) {
        case UART_INT_RBR: tmp = UART_IER_RBRINT_EN; break;
        case UART_INT_THRE: tmp = UART_IER_THREINT_EN; break;
        case UART_INT_RLS: tmp = UART_IER_RLSINT_EN; break;
        case UART1_INT_MS: tmp = UART1_IER_MSINT_EN; break;
        case UART1_INT_CTS: tmp = UART1_IER_CTSINT_EN; break;
        case UART_INT_ABEO: tmp = UART_IER_ABEOINT_EN; break;
        case UART_INT_ABTO: tmp = UART_IER_ABTOINT_EN; break;
        default: break;
    }

    if (newState == ENABLE) {
        UARTx->IER |= tmp;
    } else {
        UARTx->IER &= ~tmp & UART1_IER_BITMASK;
    }
}

void UART_TxEnable(LPC_UART_TypeDef* UARTx) {
    CHECK_PARAM(PARAM_UARTx(UARTx));

    UARTx->TER |= UART_TER_TXEN;
}

void UART_TxDisable(LPC_UART_TypeDef* UARTx) {
    CHECK_PARAM(PARAM_UARTx(UARTx));

    UARTx->TER &= (~UART_TER_TXEN) & UART_TER_BITMASK;
}

FlagStatus UART_CheckBusy(LPC_UART_TypeDef* UARTx) {
    CHECK_PARAM(PARAM_UARTx(UARTx));

    return UARTx->LSR & UART_LSR_TEMT ? RESET : SET;
}

void UART_ForceBreak(LPC_UART_TypeDef* UARTx) {
    CHECK_PARAM(PARAM_UARTx(UARTx));

    UARTx->LCR |= UART_LCR_BREAK_EN;
}

void UART_ABCmd(LPC_UART_TypeDef* UARTx, UART_AB_CFG_Type* abCfg,
                FunctionalState newState) {
    CHECK_PARAM(PARAM_UARTx(UARTx));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(newState));

    if (newState == ENABLE) {
        uint32_t tmp = UART_ACR_START;

        if (abCfg->abMode == UART_AB_MODE1) {
            tmp |= UART_ACR_MODE;
        }
        if (abCfg->autoRestart == ENABLE) {
            tmp |= UART_ACR_AUTO_RESTART;
        }

        UARTx->LCR |= UART_LCR_DLAB_EN;
        UARTx->DLL = 0;
        UARTx->DLM = 0;
        UARTx->LCR &= ~UART_LCR_DLAB_EN;

        UARTx->FDR = 0x10;

        UARTx->ACR = tmp;
    } else {
        UARTx->ACR = 0;
    }
}

void UART_ABClearIntPending(LPC_UART_TypeDef* UARTx, UART_ABEO_Type intType) {
    CHECK_PARAM(PARAM_UARTx(UARTx));

    UARTx->ACR |= intType;
}

void UART_IrDAInvtInputEnable(LPC_UART_TypeDef* UARTx) {
    CHECK_PARAM(PARAM_UART_IrDA(UARTx));

    UARTx->ICR |= UART_ICR_IRDAINV;
}

void UART_IrDAInvtInputDisable(LPC_UART_TypeDef* UARTx) {
    CHECK_PARAM(PARAM_UART_IrDA(UARTx));

    UARTx->ICR &= ~UART_ICR_IRDAINV & UART_ICR_BITMASK;
}

void UART_IrDAPulseDivConfig(LPC_UART_TypeDef* UARTx, UART_IrDA_PULSE_Type pulseDiv) {
    CHECK_PARAM(PARAM_UART_IrDA(UARTx));
    CHECK_PARAM(PARAM_UART_IrDA_PULSEDIV(pulseDiv));

    uint32_t icr_val = UARTx->ICR & ~UART_ICR_PULSEDIV(7);
    icr_val |= UART_ICR_PULSEDIV(pulseDiv) | UART_ICR_FIXPULSE_EN;

    UARTx->ICR = icr_val & UART_ICR_BITMASK;
}

void UART_ModemPinActive(UART_MODEM_PIN_Type pin) {
    CHECK_PARAM(PARAM_UART1_MODEM_PIN(pin));

    const uint32_t bit = pin == UART1_MODEM_PIN_DTR ? UART1_MCR_DTR_CTRL : UART1_MCR_RTS_CTRL;

    LPC_UART1->MCR |= bit;
}

void UART_ModemPinInactive(UART_MODEM_PIN_Type pin) {
    CHECK_PARAM(PARAM_UART1_MODEM_PIN(pin));

    const uint32_t bit = pin == UART1_MODEM_PIN_DTR ? UART1_MCR_DTR_CTRL : UART1_MCR_RTS_CTRL;

    LPC_UART1->MCR &= ~bit & UART1_MCR_BITMASK;
}

void UART_ModemModeEnable(UART_MODEM_MODE_Type mode) {
    CHECK_PARAM(PARAM_UART1_MODEM_MODE(mode));

    uint32_t bit = 0;
    switch (mode) {
        case UART1_MODEM_MODE_LOOPBACK: bit = UART1_MCR_LOOPB_EN; break;
        case UART1_MODEM_MODE_AUTO_RTS: bit = UART1_MCR_AUTO_RTS_EN; break;
        case UART1_MODEM_MODE_AUTO_CTS: bit = UART1_MCR_AUTO_CTS_EN; break;
        default: return;
    }

    LPC_UART1->MCR |= bit;
}

void UART_ModemModeDisable(UART_MODEM_MODE_Type Mode) {
    CHECK_PARAM(PARAM_UART1_MODEM_MODE(Mode));

    uint32_t bit = 0;
    switch (Mode) {
        case UART1_MODEM_MODE_LOOPBACK: bit = UART1_MCR_LOOPB_EN; break;
        case UART1_MODEM_MODE_AUTO_RTS: bit = UART1_MCR_AUTO_RTS_EN; break;
        case UART1_MODEM_MODE_AUTO_CTS: bit = UART1_MCR_AUTO_CTS_EN; break;
        default: return;
    }

    LPC_UART1->MCR &= ~bit & UART1_MCR_BITMASK;
}

uint8_t UART_FullModemGetStatus(LPC_UART1_TypeDef* UARTx) {
    CHECK_PARAM(PARAM_UART1_MODEM(UARTx));

    return UARTx->MSR & UART1_MSR_BITMASK;
}

void UART_RS485Config(const UART1_RS485_CTRLCFG_T* cfg) {
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(cfg->multiDropMode));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(cfg->rxState));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(cfg->autoAddrDetect));
    CHECK_PARAM(PARAM_FUNCTIONALSTATE(cfg->autoDirCtrl));
    CHECK_PARAM(PARAM_UART_RS485_DIRCTRL_PIN(cfg->dirCtrlPin));
    CHECK_PARAM(PARAM_SETSTATE(cfg->dirCtrlPol));

    uint32_t ctrl = 0;

    if (cfg->autoDirCtrl == ENABLE) {
        ctrl |= UART1_RS485CTRL_DCTRL_EN;

        if (cfg->dirCtrlPol == SET) {
            ctrl |= UART1_RS485CTRL_OINV_1;
        }

        if (cfg->dirCtrlPin == UART1_RS485_DIRCTRL_DTR) {
            ctrl |= UART1_RS485CTRL_SEL_DTR;
        }

        LPC_UART1->RS485DLY = cfg->delayValue & UART1_RS485DLY_BITMASK;
    }

    if (cfg->multiDropMode == ENABLE) {
        ctrl |= UART1_RS485CTRL_NMM_EN;
    }

    if (cfg->autoAddrDetect == ENABLE) {
        ctrl |= UART1_RS485CTRL_AADEN;
        LPC_UART1->ADRMATCH = cfg->matchAddrValue & UART1_RS485ADRMATCH_BITMASK;
    }

    if (cfg->rxState == DISABLE) {
        ctrl |= UART1_RS485CTRL_RX_DIS;
    }

    LPC_UART1->RS485CTRL = ctrl & UART1_RS485CTRL_BITMASK;

    LPC_UART1->LCR |= (UART_LCR_PARITY_F_0 | UART_LCR_PARITY_EN);
}

void UART_RS485ReceiverEnable(void) {
    LPC_UART1->RS485CTRL &= ~UART1_RS485CTRL_RX_DIS;
}

void UART_RS485ReceiverDisable(void) {
    LPC_UART1->RS485CTRL |= UART1_RS485CTRL_RX_DIS;
}

uint32_t UART_RS485Send(const uint8_t* pDatFrm, uint32_t size, uint8_t isAddress) {
    const uint32_t lcr_save = LPC_UART1->LCR;

    if (isAddress) {
        LPC_UART1->LCR &= ~(UART_LCR_PARITY_EVEN);
    } else {
        LPC_UART1->LCR |= UART_LCR_PARITY_EVEN;
    }

    const uint32_t cnt = UART_Send(UART1, pDatFrm, size, BLOCKING);

    while (!(LPC_UART1->LSR & UART_LSR_TEMT)) {}

    LPC_UART1->LCR = lcr_save;

    return cnt;
}

void UART_RS485SendSlvAddr(uint8_t slvAddr) {
    UART_RS485Send(&slvAddr, 1, 1);
}

uint32_t UART_RS485SendData(uint8_t* pData, uint32_t size) {
    return (UART_RS485Send(pData, size, 0));
}

#endif /* _UART */

/**
 * @}
 */

/**
 * @}
 */
/* --------------------------------- End Of File ------------------------------ */
