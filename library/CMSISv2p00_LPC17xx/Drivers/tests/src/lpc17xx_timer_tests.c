/**
 * @file     lpc17xx_timer_tests.c
 * @brief    Tests for the LPC17xx TIMER driver.
 * @version  V1.0
 * @date     01 August 2025
 */
#ifdef UNIT_TESTING_ENABLED

#include "lpc17xx_timer_tests.h"

uint8_t TIM_InitTimerTest(void);
uint8_t TIM_InitCounterTest(void);
uint8_t TIM_DeInitTest(void);
uint8_t TIM_ClearIntPendingTest(void);
uint8_t TIM_GetIntStatusTest(void);
uint8_t TIM_ConfigStructInitTimerTest(void);
uint8_t TIM_ConfigStructInitCounterTest(void);
uint8_t TIM_ConfigMatchTest(void);
uint8_t TIM_UpdateMatchValueTest(void);
uint8_t TIM_SetMatchExtTest(void);
uint8_t TIM_ConfigCaptureTest(void);
uint8_t TIM_CmdTest(void);
uint8_t TIM_GetCaptureValueTest(void);
uint8_t TIM_ResetCounterTest(void);

void TIMER_Setup(void) {
    LPC_TIM0->TCR = 0;
    LPC_TIM0->TCR = (1 << 1);
    LPC_TIM0->TCR = 0;
    LPC_TIM0->IR |= 0x3F;
    LPC_TIM0->CTCR = 0;
    LPC_TIM0->PR   = 0;
    LPC_TIM0->MCR  = 0;
    LPC_TIM0->EMR  = 0;
    LPC_TIM0->CCR  = 0;
    LPC_TIM0->MR0  = 0;
    LPC_TIM0->MR1  = 0;
}

void TIMER_TearDown(void) {}

void TIMER_RunTests(void) {
    RUN_TESTS_INIT();

    RUN_TEST(TIM_InitTimerTest);
    RUN_TEST(TIM_InitCounterTest);
    RUN_TEST(TIM_DeInitTest);
    RUN_TEST(TIM_ClearIntPendingTest);
    RUN_TEST(TIM_GetIntStatusTest);
    RUN_TEST(TIM_ConfigMatchTest);
    RUN_TEST(TIM_UpdateMatchValueTest);
    RUN_TEST(TIM_SetMatchExtTest);
    RUN_TEST(TIM_ConfigCaptureTest);
    RUN_TEST(TIM_CmdTest);
    RUN_TEST(TIM_GetCaptureValueTest);
    RUN_TEST(TIM_ResetCounterTest);

    RUN_TESTS_END("TIMER");
}

uint8_t TIM_InitTimerTest(void) {
    TIMER_Setup();
    TEST_INIT();

    TIM_TIMERCFG_T cfg = {0};
    cfg.prescaleOpt    = TIM_US;
    cfg.prescaleValue  = 1000;

    TIM_InitTimer(LPC_TIM0, &cfg);
    EXPECT_EQUAL(LPC_TIM0->CTCR & TIM_CTCR_MODE_MASK, TIM_TIMER_MODE);
    EXPECT_EQUAL(LPC_TIM0->CTCR & TIM_CTCR_INPUT_MASK, 0x00);
    EXPECT_EQUAL(LPC_TIM0->PR, SystemCoreClock / 4 / 1000000 * cfg.prescaleValue - 1);
    EXPECT_EQUAL(LPC_TIM0->IR & 0x3F, 0x0);
    EXPECT_EQUAL(LPC_TIM0->TCR & TIM_ENABLE, 0x00);

    ASSERT_TEST();
}

uint8_t TIM_InitCounterTest(void) {
    TIMER_Setup();
    TEST_INIT();

    TIM_COUNTERCFG_T cfg = {0};
    cfg.input            = TIM_CAPTURE_1;
    cfg.edge             = TIM_CTR_FALLING;

    TIM_InitCounter(LPC_TIM0, &cfg);
    EXPECT_EQUAL(LPC_TIM0->CTCR & TIM_CTCR_MODE_MASK, TIM_CTR_FALLING);
    EXPECT_EQUAL(LPC_TIM0->CTCR & TIM_CTCR_INPUT_MASK, TIM_CAPTURE_1 << 2);
    EXPECT_EQUAL(LPC_TIM0->PR, 0);
    EXPECT_EQUAL(LPC_TIM0->IR & 0x3F, 0x0);
    EXPECT_EQUAL(LPC_TIM0->TCR & TIM_ENABLE, 0x00);

    ASSERT_TEST();
}

uint8_t TIM_DeInitTest(void) {
    TIMER_Setup();
    TEST_INIT();

    TIM_TIMERCFG_T cfg = {0};
    cfg.prescaleOpt    = TIM_TICK;
    cfg.prescaleValue  = 1;

    TIM_InitTimer(LPC_TIM0, &cfg);
    LPC_TIM0->TCR |= TIM_ENABLE;
    TIM_DeInit(LPC_TIM0);
    EXPECT_EQUAL((LPC_SC->PCONP & 0x2) >> 1, 0x00);
    LPC_SC->PCONP |= 0x2;

    ASSERT_TEST();
}

uint8_t TIM_ClearIntPendingTest(void) {
    TIMER_Setup();
    TEST_INIT();

    TIM_TIMERCFG_T cfg = {0};
    cfg.prescaleOpt    = TIM_TICK;
    cfg.prescaleValue  = 1;

    TIM_InitTimer(LPC_TIM0, &cfg);
    LPC_TIM0->MR0 = 0x10;
    LPC_TIM0->MCR |= TIM_INT_ON_MATCH(0) | TIM_STOP_ON_MATCH(0);
    LPC_TIM0->TCR |= TIM_ENABLE;

    for (volatile unsigned int _d = 0; _d < 100; ++_d) {}

    EXPECT_EQUAL(LPC_TIM0->IR & 0x1, 0x1);

    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);

    EXPECT_EQUAL(LPC_TIM0->IR & 0x1, 0x00);

    ASSERT_TEST();
}

uint8_t TIM_GetIntStatusTest(void) {
    TIMER_Setup();
    TEST_INIT();

    TIM_TIMERCFG_T cfg = {0};
    cfg.prescaleOpt    = TIM_TICK;
    cfg.prescaleValue  = 1;

    TIM_InitTimer(LPC_TIM0, &cfg);
    LPC_TIM0->MR0 = 0x10;
    LPC_TIM0->MCR |= TIM_INT_ON_MATCH(0) | TIM_STOP_ON_MATCH(0);
    LPC_TIM0->TCR |= TIM_ENABLE;

    for (volatile unsigned int _d = 0; _d < 100; ++_d) {}

    EXPECT_EQUAL(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT), SET);
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
    EXPECT_EQUAL(TIM_GetIntStatus(LPC_TIM0, TIM_MR1_INT), RESET);

    ASSERT_TEST();
}

uint8_t TIM_ConfigMatchTest(void) {
    TIMER_Setup();
    TEST_INIT();

    TIM_MATCHCFG_T matchCfg = {0};
    matchCfg.channel        = 0;
    matchCfg.intEn          = ENABLE;
    matchCfg.resetEn        = DISABLE;
    matchCfg.stopEn         = ENABLE;
    matchCfg.extOpt         = TIM_TOGGLE;
    matchCfg.matchValue     = 0xFF;
    TIM_ConfigMatch(LPC_TIM0, &matchCfg);

    EXPECT_EQUAL(LPC_TIM0->MR0, 0xFF);
    EXPECT_EQUAL(LPC_TIM0->MCR & TIM_MCR_CHANNEL_MASKBIT(0),
                 TIM_INT_ON_MATCH(0) | TIM_STOP_ON_MATCH(0));
    EXPECT_EQUAL(LPC_TIM0->EMR & TIM_EMR_MASK(0), TIM_EM_SET(0, TIM_TOGGLE));

    ASSERT_TEST();
}

uint8_t TIM_UpdateMatchValueTest(void) {
    TIMER_Setup();
    TEST_INIT();

    TIM_UpdateMatchValue(LPC_TIM0, 1, 0x200);
    EXPECT_EQUAL(LPC_TIM0->MR1, 0x200);

    ASSERT_TEST();
}

uint8_t TIM_SetMatchExtTest(void) {
    TIMER_Setup();
    TEST_INIT();

    TIM_SetMatchExt(LPC_TIM0, 1, TIM_TOGGLE);
    EXPECT_EQUAL(LPC_TIM0->EMR & TIM_EMR_MASK(1), TIM_EM_SET(1, TIM_TOGGLE));

    ASSERT_TEST();
}

uint8_t TIM_ConfigCaptureTest(void) {
    TIMER_Setup();
    TEST_INIT();

    TIM_CAPTURECFG_T capCfg = {0, 1, 0, 1};
    TIM_ConfigCapture(LPC_TIM0, &capCfg);

    EXPECT_EQUAL(LPC_TIM0->CCR & TIM_CCR_CHANNEL_MASKBIT(0), TIM_CAP_RISING(0) | TIM_INT_ON_CAP(0));

    ASSERT_TEST();
}

uint8_t TIM_CmdTest(void) {
    TIMER_Setup();
    TEST_INIT();

    TIM_Enable(LPC_TIM0);
    EXPECT_EQUAL(LPC_TIM0->TCR & TIM_ENABLE, TIM_ENABLE);
    TIM_Disable(LPC_TIM0);
    EXPECT_EQUAL(LPC_TIM0->TCR & TIM_ENABLE, 0x00);

    ASSERT_TEST();
}

uint8_t TIM_GetCaptureValueTest(void) {
    TIMER_Setup();
    TEST_INIT();
    LPC_PINCON->PINSEL3 |= (0x3 << 20);
    LPC_TIM0->CCR |= TIM_CAP_FALLING(0);

    if (LPC_TIM0->CR0 == 0x00) {
        EXPECT_EQUAL(LPC_TIM0->CR0, 0x00);
    } else {
        printf(
            "[⚠️ Warning] TIM_GetCaptureValueTest requires a full board reset before re-running.\n");
        __test_passed = 0;
        ASSERT_TEST();
    }

    TIM_Enable(LPC_TIM0);
    EDGE_INT_P1_HIGH(26);
    EXPECT_TRUE(LPC_TIM0->CR0 != 0x00);

    ASSERT_TEST();
}

uint8_t TIM_ResetCounterTest(void) {
    TIMER_Setup();
    TEST_INIT();

    LPC_TIM0->PR = 0xF;
    LPC_TIM0->TCR |= TIM_ENABLE;
    PROPAGATION_DELAY();
    LPC_TIM0->TCR &= ~TIM_ENABLE;
    EXPECT_TRUE(LPC_TIM0->TC != 0x00);
    EXPECT_TRUE(LPC_TIM0->PC != 0x00);
    TIM_ResetCounter(LPC_TIM0);
    EXPECT_EQUAL(LPC_TIM0->TC, 0x00);
    EXPECT_EQUAL(LPC_TIM0->PC, 0x00);

    ASSERT_TEST();
}

#endif  // UNIT_TESTING_ENABLED
