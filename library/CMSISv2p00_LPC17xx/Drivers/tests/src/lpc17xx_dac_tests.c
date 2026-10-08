/**
 * @file    lpc17xx_dac_tests.c
 * @brief   Tests for the LPC17xx DAC driver.
 * @version V1.0
 * @date    22 February 2026
 */
#ifdef UNIT_TESTING_ENABLED

#include "lpc17xx_dac_tests.h"

static uint8_t DAC_InitTest(void);
static uint8_t DAC_UpdateValueTest(void);
static uint8_t DAC_SetBiasTest(void);
static uint8_t DAC_ConfigDAConverterControlTest(void);
static uint8_t DAC_SetDMATimeOutTest(void);

void DAC_Setup(void) {
    DAC_Init();
}

void DAC_TearDown(void) {
    // Reset pin configuration to turn off DAC
    LPC_PINCON->PINSEL1  = 0UL;
    LPC_PINCON->PINMODE1 = 0UL;
}

void DAC_RunTests(void) {
    RUN_TESTS_INIT();

    RUN_TEST(DAC_InitTest);
    RUN_TEST(DAC_UpdateValueTest);
    RUN_TEST(DAC_SetBiasTest);
    RUN_TEST(DAC_ConfigDAConverterControlTest);
    RUN_TEST(DAC_SetDMATimeOutTest);

    RUN_TESTS_END("DAC");
}

static uint8_t DAC_InitTest(void) {
    TEST_INIT();

    DAC_Init();
    EXPECT_EQUAL(LPC_PINCON->PINSEL1 & (0x3 << 20), 0x2 << 20);   // P0.26 as DAC output
    EXPECT_EQUAL(LPC_PINCON->PINMODE1 & (0x3 << 20), 0x2 << 20);  // P0.26 with no pull-up/down

    EXPECT_EQUAL(LPC_DAC->DACR & DAC_BIAS_EN, 0);  // Default to 700 uA

    DAC_TearDown();
    ASSERT_TEST();
}

static uint8_t DAC_UpdateValueTest(void) {
    TEST_INIT();
    DAC_Init();

    DAC_UpdateValue(512);
    EXPECT_EQUAL(LPC_DAC->DACR & (0x3FF << 6), 512 << 6);

    DAC_TearDown();
    ASSERT_TEST();
}

static uint8_t DAC_SetBiasTest(void) {
    TEST_INIT();
    DAC_Init();

    DAC_SetBias(DAC_350uA);
    EXPECT_EQUAL(LPC_DAC->DACR & DAC_BIAS_EN, DAC_BIAS_EN);

    DAC_SetBias(DAC_700uA);
    EXPECT_EQUAL(LPC_DAC->DACR & DAC_BIAS_EN, 0);

    DAC_TearDown();
    ASSERT_TEST();
}

static uint8_t DAC_ConfigDAConverterControlTest(void) {
    TEST_INIT();
    DAC_Init();

    DAC_CONVERTER_CFG_T cfg = {0};
    cfg.doubleBuffer        = ENABLE;
    cfg.dmaCounter          = DISABLE;
    cfg.dmaRequest          = ENABLE;

    DAC_ConfigDAConverterControl(&cfg);
    EXPECT_EQUAL(LPC_DAC->DACCTRL & DAC_DBLBUF_ENA, DAC_DBLBUF_ENA);
    EXPECT_EQUAL(LPC_DAC->DACCTRL & DAC_CNT_ENA, 0);
    EXPECT_EQUAL(LPC_DAC->DACCTRL & DAC_DMA_ENA, DAC_DMA_ENA);

    cfg.doubleBuffer = DISABLE;
    cfg.dmaCounter   = ENABLE;
    cfg.dmaRequest   = DISABLE;

    DAC_ConfigDAConverterControl(&cfg);
    EXPECT_EQUAL(LPC_DAC->DACCTRL & DAC_DBLBUF_ENA, 0);
    EXPECT_EQUAL(LPC_DAC->DACCTRL & DAC_CNT_ENA, DAC_CNT_ENA);
    EXPECT_EQUAL(LPC_DAC->DACCTRL & DAC_DMA_ENA, 0);

    DAC_TearDown();
    ASSERT_TEST();
}

static uint8_t DAC_SetDMATimeOutTest(void) {
    TEST_INIT();
    DAC_Init();

    DAC_SetDMATimeOut(1000);
    EXPECT_EQUAL(LPC_DAC->DACCNTVAL & 0xFFFF, 1000 & 0xFFFF);

    DAC_SetDMATimeOut(0xFFFF);
    EXPECT_EQUAL(LPC_DAC->DACCNTVAL & 0xFFFF, 0xFFFF);

    DAC_TearDown();
    ASSERT_TEST();
}

#endif  // UNIT_TESTING_ENABLED
