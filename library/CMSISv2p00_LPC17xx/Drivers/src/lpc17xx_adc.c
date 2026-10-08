/**
 * @file        lpc17xx_adc.c
 * @brief       Contains all functions support for ADC firmware library on LPC17xx
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
/** @addtogroup ADC
 * @{
 */

/* -------------------------------- Includes -------------------------------- */
#include "lpc17xx_adc.h"
#include "lpc17xx_clkpwr.h"

/* If this source file built with example, the LPC17xx FW library configuration
 * file in each example directory ("lpc17xx_libcfg.h") must be included,
 * otherwise the default FW library configuration file must be included instead
 */
#ifdef __BUILD_WITH_EXAMPLE__
#include "lpc17xx_libcfg.h"
#else
#include "lpc17xx_libcfg_default.h"
#endif /* __BUILD_WITH_EXAMPLE__ */

#ifdef _ADC

/* ---------------------------- Public Functions ---------------------------- */
/** @addtogroup ADC_Public_Functions
 * @{
 */

void ADC_Init(uint32_t rate) {
    CHECK_PARAM(PARAM_ADC_RATE(rate));

    CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCAD, ENABLE);

    const uint32_t pclk    = CLKPWR_GetPCLK(CLKPWR_PCLKSEL_ADC);
    const uint32_t divisor = rate * 65;

    uint32_t clkdiv = (pclk + divisor / 2) / divisor - 1;

    clkdiv = clkdiv > 255 ? 255 : clkdiv;

    LPC_ADC->ADCR = ADC_CR_CLKDIV(clkdiv) | ADC_CR_PDN;
}

void ADC_DeInit(void) {
    LPC_ADC->ADCR &= ~ADC_CR_PDN;

    CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCAD, DISABLE);
}

void ADC_PinConfig(ADC_CHANNEL channel) {
    CHECK_PARAM(PARAM_ADC_CHANNEL(channel));

    switch (channel) {
        case ADC_CHANNEL_0:
            LPC_PINCON->PINSEL1  = (LPC_PINCON->PINSEL1 & ~(0x3 << 14)) | (0x1 << 14);
            LPC_PINCON->PINMODE1 = (LPC_PINCON->PINMODE1 & ~(0x3 << 14)) | (0x2 << 14);
            break;

        case ADC_CHANNEL_1:
            LPC_PINCON->PINSEL1  = (LPC_PINCON->PINSEL1 & ~(0x3 << 16)) | (0x1 << 16);
            LPC_PINCON->PINMODE1 = (LPC_PINCON->PINMODE1 & ~(0x3 << 16)) | (0x2 << 16);
            break;

        case ADC_CHANNEL_2:
            LPC_PINCON->PINSEL1  = (LPC_PINCON->PINSEL1 & ~(0x3 << 18)) | (0x1 << 18);
            LPC_PINCON->PINMODE1 = (LPC_PINCON->PINMODE1 & ~(0x3 << 18)) | (0x2 << 18);
            break;

        case ADC_CHANNEL_3:
            LPC_PINCON->PINSEL1  = (LPC_PINCON->PINSEL1 & ~(0x3 << 20)) | (0x1 << 20);
            LPC_PINCON->PINMODE1 = (LPC_PINCON->PINMODE1 & ~(0x3 << 20)) | (0x2 << 20);
            break;

        case ADC_CHANNEL_4:
            LPC_PINCON->PINSEL3  = (LPC_PINCON->PINSEL3 & ~(0x3 << 28)) | (0x3 << 28);
            LPC_PINCON->PINMODE3 = (LPC_PINCON->PINMODE3 & ~(0x3 << 28)) | (0x2 << 28);
            break;

        case ADC_CHANNEL_5:
            LPC_PINCON->PINSEL3  = (LPC_PINCON->PINSEL3 & ~(0x3 << 30)) | (0x3 << 30);
            LPC_PINCON->PINMODE3 = (LPC_PINCON->PINMODE3 & ~(0x3 << 30)) | (0x2 << 30);
            break;

        case ADC_CHANNEL_6:
            LPC_PINCON->PINSEL0  = (LPC_PINCON->PINSEL0 & ~(0x3 << 6)) | (0x2 << 6);
            LPC_PINCON->PINMODE0 = (LPC_PINCON->PINMODE0 & ~(0x3 << 6)) | (0x2 << 6);
            break;

        case ADC_CHANNEL_7:
            LPC_PINCON->PINSEL0  = (LPC_PINCON->PINSEL0 & ~(0x3 << 4)) | (0x2 << 4);
            LPC_PINCON->PINMODE0 = (LPC_PINCON->PINMODE0 & ~(0x3 << 4)) | (0x2 << 4);
            break;

        default: break;
    }
}

void ADC_BurstEnable(void) {
    LPC_ADC->ADCR |= ADC_CR_BURST;
}

void ADC_BurstDisable(void) {
    LPC_ADC->ADCR &= ~ADC_CR_BURST;
}

void ADC_PowerUp(void) {
    LPC_ADC->ADCR |= ADC_CR_PDN;
}

void ADC_PowerDown(void) {
    LPC_ADC->ADCR &= ~ADC_CR_PDN;
}

void ADC_StartCmd(ADC_START_MODE mode) {
    CHECK_PARAM(PARAM_ADC_START_MODE(mode));

    LPC_ADC->ADCR &= ~ADC_CR_START_MASK;
    LPC_ADC->ADCR |= ADC_CR_START_MODE_SEL((uint32_t)mode);
}

void ADC_ChannelEnable(ADC_CHANNEL channel) {
    CHECK_PARAM(PARAM_ADC_CHANNEL(channel));

    LPC_ADC->ADCR |= ADC_CR_CH_SEL(channel);
}

void ADC_ChannelDisable(ADC_CHANNEL channel) {
    CHECK_PARAM(PARAM_ADC_CHANNEL(channel));

    LPC_ADC->ADCR &= ~ADC_CR_CH_SEL(channel);
}

void ADC_EdgeStartConfig(ADC_START_ON_EDGE edge) {
    CHECK_PARAM(PARAM_ADC_START_ON_EDGE(edge));

    if (edge) {
        LPC_ADC->ADCR |= ADC_CR_EDGE;
        return;
    }
    LPC_ADC->ADCR &= ~ADC_CR_EDGE;
}

void ADC_IntEnable(ADC_INT_SOURCE source) {
    CHECK_PARAM(PARAM_ADC_INT_SOURCE(source));

    LPC_ADC->ADINTEN |= (1UL << source);
}

void ADC_IntDisable(ADC_INT_SOURCE source) {
    CHECK_PARAM(PARAM_ADC_INT_SOURCE(source));

    LPC_ADC->ADINTEN &= ~ADC_INTEN_CH(source);
}

FlagStatus ADC_GlobalGetStatus(ADC_DATA_STATUS type) {
    CHECK_PARAM(PARAM_ADC_DATA_STATUS(type));

    uint32_t temp = LPC_ADC->ADGDR;

    if (type == ADC_DATA_DONE) {
        temp &= ADC_DR_DONE_FLAG;
    } else {
        temp &= ADC_DR_OVERRUN_FLAG;
    }

    if (temp) {
        return SET;
    }

    return RESET;
}

FlagStatus ADC_ChannelGetStatus(ADC_CHANNEL channel, ADC_DATA_STATUS type) {
    CHECK_PARAM(PARAM_ADC_CHANNEL(channel));
    CHECK_PARAM(PARAM_ADC_DATA_STATUS(type));

    uint32_t temp = *(uint32_t*)((&LPC_ADC->ADDR0) + channel);

    if (type) {
        temp &= ADC_DR_DONE_FLAG;
    } else {
        temp &= ADC_DR_OVERRUN_FLAG;
    }

    if (temp) {
        return SET;
    }

    return RESET;
}

uint16_t ADC_GlobalGetData() {
    return ADC_GDR_RESULT(LPC_ADC->ADGDR);
}

uint16_t ADC_ChannelGetData(ADC_CHANNEL channel) {
    CHECK_PARAM(PARAM_ADC_CHANNEL(channel));

    const uint32_t adc_value = *(&LPC_ADC->ADDR0 + channel);
    return ADC_DR_RESULT(adc_value);
}

/**
 * @}
 */

#endif /* _ADC */

/**
 * @}
 */

/* ------------------------------ End Of File ------------------------------- */
