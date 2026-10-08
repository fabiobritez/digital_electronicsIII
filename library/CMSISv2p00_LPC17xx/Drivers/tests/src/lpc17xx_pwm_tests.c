/**
 * @file     lpc17xx_pwm_tests.c
 * @brief    Tests for the LPC17xx TIMER driver.
 * @version  V1.0
 * @date     21 February 2026
 */
#ifdef UNIT_TESTING_ENABLED

#include "lpc17xx_pwm_tests.h"

#include "lpc17xx_pwm.h"

uint8_t PWM_InitTimerTest(void);
uint8_t PWM_InitCounterTest(void);
uint8_t PWM_DeInitTest(void);
uint8_t PWM_ChannelConfigTest(void);
uint8_t PWM_ChannelCmdTest(void);
uint8_t PWM_CmdTest(void);
uint8_t PWM_CounterCmdTest(void);
uint8_t PWM_ResetCounterTest(void);
uint8_t PWM_ConfigMatchTest(void);
uint8_t PWM_MatchUpdateSingleTest(void);
uint8_t PWM_MatchUpdateDoubleTest(void);
uint8_t PWM_ClearIntPendingTest(void);
uint8_t PWM_GetIntStatusTest(void);
uint8_t PWM_ConfigCaptureTest(void);
uint8_t PWM_GetCaptureValueTest(void);

void PWM_Setup(void) {
    LPC_PWM1->TCR = 0;
    LPC_PWM1->IR |= 0x3F;
    LPC_PWM1->CTCR = 0;
    LPC_PWM1->PR   = 0;
    LPC_PWM1->MCR  = 0;
    LPC_PWM1->CCR  = 0;
    LPC_PWM1->MR0  = 0;
    LPC_PWM1->MR1  = 0;
    LPC_PWM1->MR2  = 0;
    LPC_PWM1->MR3  = 0;
    LPC_PWM1->MR4  = 0;
    LPC_PWM1->MR5  = 0;
    LPC_PWM1->MR6  = 0;
    LPC_PWM1->LER  = 0x3F;
    LPC_PWM1->TCR  = (1 << 1);
    LPC_PWM1->TCR  = 0;
}

void PWM_TearDown(void) {}

void PWM_RunTests(void) {
    RUN_TESTS_INIT();

    RUN_TEST(PWM_InitTimerTest)
    RUN_TEST(PWM_InitCounterTest);
    RUN_TEST(PWM_DeInitTest);
    RUN_TEST(PWM_ChannelConfigTest);
    RUN_TEST(PWM_ChannelCmdTest);
    RUN_TEST(PWM_CmdTest);
    RUN_TEST(PWM_CounterCmdTest);
    RUN_TEST(PWM_ResetCounterTest);
    RUN_TEST(PWM_ConfigMatchTest);
    RUN_TEST(PWM_MatchUpdateSingleTest);
    RUN_TEST(PWM_MatchUpdateDoubleTest);
    RUN_TEST(PWM_ClearIntPendingTest);
    RUN_TEST(PWM_GetIntStatusTest);
    RUN_TEST(PWM_ConfigCaptureTest);
    RUN_TEST(PWM_GetCaptureValueTest);

    RUN_TESTS_END("PWM");
}

uint8_t PWM_InitTimerTest(void) {
    PWM_Setup();
    TEST_INIT();

    PWM_TIMERCFG_T cfg = {0};
    cfg.prescaleOpt    = PWM_US;
    cfg.prescaleValue  = 1000;

    PWM_InitTimer(&cfg);
    EXPECT_EQUAL(LPC_PWM1->CTCR & PWM_CTCR_MODE_MASK, TIM_TIMER_MODE);
    EXPECT_EQUAL(LPC_PWM1->CTCR & PWM_CTCR_INPUT_MASK, 0x00);
    EXPECT_EQUAL(LPC_PWM1->PR, SystemCoreClock / 4 / 1000000 * cfg.prescaleValue - 1);
    EXPECT_EQUAL(LPC_PWM1->IR & 0x3F, 0x0);
    EXPECT_EQUAL(LPC_PWM1->TCR & (PWM_TCR_COUNTER_ENABLE | PWM_TCR_PWM_ENABLE), 0x0);

    ASSERT_TEST();
}

uint8_t PWM_InitCounterTest(void) {
    PWM_Setup();
    TEST_INIT();

    PWM_COUNTERCFG_T cfg = {0};
    cfg.input            = PWM_CAPTURE_1;
    cfg.edge             = PWM_CTR_FALLING;

    PWM_InitCounter(&cfg);
    EXPECT_EQUAL(LPC_PWM1->CTCR & PWM_CTCR_MODE_MASK, PWM_CTR_FALLING);
    EXPECT_EQUAL(LPC_PWM1->CTCR & PWM_CTCR_INPUT_MASK, PWM_CAPTURE_1 << 2);
    EXPECT_EQUAL(LPC_PWM1->PR, 0);
    EXPECT_EQUAL(LPC_PWM1->IR & 0x3F, 0x0);
    EXPECT_EQUAL(LPC_PWM1->TCR & (PWM_TCR_COUNTER_ENABLE | PWM_TCR_PWM_ENABLE), 0x0);

    ASSERT_TEST();
}

uint8_t PWM_DeInitTest(void) {
    PWM_Setup();
    TEST_INIT();

    PWM_TIMERCFG_T cfg = {0};
    cfg.prescaleOpt    = PWM_TICK;
    cfg.prescaleValue  = 1;

    PWM_InitTimer(&cfg);
    // counter enable and PWM enable bits are set to check if they are cleared after
    // de-initialization
    LPC_PWM1->TCR |= PWM_TCR_COUNTER_ENABLE | PWM_TCR_PWM_ENABLE;

    PWM_DeInit();
    // Power for PWM1 should be disabled after de-initialization, so bit 6 in PCONP should be
    // cleared
    EXPECT_EQUAL((LPC_SC->PCONP & 1UL << 6) >> 1, 0x00);
    LPC_SC->PCONP |= 1UL << 6;  // Re-enable power for PWM1 for other tests

    // TCR Counter enable and PWM enable bits should be cleared after de-initialization
    EXPECT_EQUAL(LPC_PWM1->TCR, 0x0);

    ASSERT_TEST();
}

uint8_t PWM_ChannelConfigTest(void) {
    PWM_Setup();
    TEST_INIT();

    PWM_ChannelConfig(PWM_CHANNEL_1, PWM_DUAL_EDGE);
    // Nothing changed after configuring channel 1
    EXPECT_EQUAL(LPC_PWM1->PCR, 0x0);

    PWM_ChannelConfig(PWM_CHANNEL_3, PWM_SINGLE_EDGE);
    // Nothing changed after configuring channel 2 with single edge, as it is default value
    EXPECT_EQUAL(LPC_PWM1->PCR, 0x0);

    PWM_ChannelConfig(PWM_CHANNEL_3, PWM_DUAL_EDGE);
    // Channel 3 is configured to dual edge, so bit 3 in PCR should be set
    EXPECT_EQUAL(LPC_PWM1->PCR & 1UL << 3, 1UL << 3);

    PWM_ChannelConfig(PWM_CHANNEL_3, PWM_SINGLE_EDGE);
    // Channel 3 is configured to single edge, so bit 3 in PCR should be cleared again
    EXPECT_EQUAL(LPC_PWM1->PCR & 1UL << 3, 0x0);

    ASSERT_TEST();
}

uint8_t PWM_ChannelCmdTest(void) {
    PWM_Setup();
    TEST_INIT();

    PWM_ChannelEnable(PWM_CHANNEL_2);
    // Channel 2 is enabled, so bit 2 in PCR should be set
    EXPECT_EQUAL(LPC_PWM1->PCR & 1UL << 10, 1UL << 10);

    PWM_ChannelDisable(PWM_CHANNEL_2);
    // Channel 2 is disabled, so bit 2 in PCR should be cleared again
    EXPECT_EQUAL(LPC_PWM1->PCR & 1UL << 10, 0x00);

    ASSERT_TEST();
}

uint8_t PWM_CmdTest(void) {
    PWM_Setup();
    TEST_INIT();

    PWM_Enable();
    // PWM enable bit should be set in TCR after enabling PWM
    EXPECT_EQUAL(LPC_PWM1->TCR & PWM_TCR_PWM_ENABLE, PWM_TCR_PWM_ENABLE);

    PWM_Disable();
    // PWM enable bit should be cleared in TCR after disabling PWM
    EXPECT_EQUAL(LPC_PWM1->TCR & PWM_TCR_PWM_ENABLE, 0x00);

    ASSERT_TEST();
}

uint8_t PWM_CounterCmdTest(void) {
    PWM_Setup();
    TEST_INIT();

    PWM_CounterEnable();
    // Counter enable bit should be set in TCR after enabling counter
    EXPECT_EQUAL(LPC_PWM1->TCR & PWM_TCR_COUNTER_ENABLE, PWM_TCR_COUNTER_ENABLE);

    PWM_CounterDisable();
    // Counter enable bit should be cleared in TCR after disabling counter
    EXPECT_EQUAL(LPC_PWM1->TCR & PWM_TCR_COUNTER_ENABLE, 0x00);

    ASSERT_TEST();
}

uint8_t PWM_ResetCounterTest(void) {
    PWM_Setup();
    TEST_INIT();

    // Set some non-zero values in TC and PC by enabling counter with a non-zero prescale value
    LPC_PWM1->PR = 0xF;
    LPC_PWM1->TCR |= PWM_TCR_COUNTER_ENABLE;
    PROPAGATION_DELAY();
    LPC_PWM1->TCR &= ~PWM_TCR_COUNTER_ENABLE;

    // TC and PC should have non-zero values after counter is enabled
    EXPECT_TRUE(LPC_PWM1->TC != 0x00);
    EXPECT_TRUE(LPC_PWM1->PC != 0x00);

    PWM_ResetCounter();
    // TC and PC should be reset to zero after resetting counter
    EXPECT_EQUAL(LPC_PWM1->TC, 0x00);
    EXPECT_EQUAL(LPC_PWM1->PC, 0x00);

    ASSERT_TEST();
}

uint8_t PWM_ConfigMatchTest(void) {
    PWM_Setup();
    TEST_INIT();

    PWM_MATCHCFG_T matchCfg = {0};

    matchCfg.channel    = PWM_MATCH_5;
    matchCfg.intEn      = ENABLE;
    matchCfg.resetEn    = DISABLE;
    matchCfg.stopEn     = ENABLE;
    matchCfg.matchValue = 0xFF;
    PWM_ConfigMatch(&matchCfg);

    EXPECT_EQUAL(LPC_PWM1->MR5, 0xFF);
    EXPECT_EQUAL(LPC_PWM1->MCR & PWM_MCR_CHANNEL_MASKBIT(5), PWM_MCR_INT(5) | PWM_MCR_STOP(5));

    ASSERT_TEST();
}

uint8_t PWM_MatchUpdateSingleTest(void) {
    PWM_Setup();
    TEST_INIT();

    EXPECT_EQUAL(LPC_PWM1->MR1, 0x00);
    PWM_MatchUpdateSingle(PWM_MATCH_1, 0xFF);

    // Force update of MR1 by resetting counter
    PWM_ResetCounter();
    EXPECT_EQUAL(LPC_PWM1->MR1, 0xFF);

    ASSERT_TEST();
}

uint8_t PWM_MatchUpdateDoubleTest(void) {
    PWM_Setup();
    TEST_INIT();

    EXPECT_EQUAL(LPC_PWM1->MR1, 0x00);
    EXPECT_EQUAL(LPC_PWM1->MR3, 0x00);
    PWM_MatchUpdateDouble(PWM_MATCH_1, 0xFF, PWM_MATCH_3, 0xAA);

    // Force update of MR1 and MR3 by resetting counter
    PWM_ResetCounter();
    EXPECT_EQUAL(LPC_PWM1->MR1, 0xFF);
    EXPECT_EQUAL(LPC_PWM1->MR3, 0xAA);

    ASSERT_TEST();
}

uint8_t PWM_ClearIntPendingTest(void) {
    PWM_Setup();
    TEST_INIT();

    PWM_TIMERCFG_T cfg = {0};
    cfg.prescaleOpt    = PWM_TICK;
    cfg.prescaleValue  = 1;

    PWM_InitTimer(&cfg);
    // Set match value and enable interrupt on match for channel 0
    LPC_PWM1->MR0 = 0x10;
    LPC_PWM1->MCR |= PWM_MCR_INT(0) | PWM_MCR_STOP(0);
    LPC_PWM1->TCR |= PWM_TCR_COUNTER_ENABLE;

    for (volatile unsigned int _d = 0; _d < 100; ++_d) {}

    // After match, interrupt flag for channel 0 should be set in IR
    EXPECT_EQUAL(LPC_PWM1->IR & 0x1, 0x1);

    PWM_ClearIntPending(PWM_MR0_INT);
    // After clearing interrupt pending flag, it should be cleared in IR
    EXPECT_EQUAL(LPC_PWM1->IR & 0x1, 0x00);

    ASSERT_TEST();
}

uint8_t PWM_GetIntStatusTest(void) {
    PWM_Setup();
    TEST_INIT();

    PWM_TIMERCFG_T cfg = {0};
    cfg.prescaleOpt    = PWM_TICK;
    cfg.prescaleValue  = 1;

    PWM_InitTimer(&cfg);
    // Set match value and enable interrupt on match for channel 0
    LPC_PWM1->MR0 = 0x10;
    LPC_PWM1->MCR |= PWM_MCR_INT(0) | PWM_MCR_STOP(0);
    LPC_PWM1->TCR |= PWM_TCR_COUNTER_ENABLE;

    for (volatile unsigned int _d = 0; _d < 100; ++_d) {}

    EXPECT_EQUAL(PWM_GetIntStatus(PWM_MR0_INT), SET);
    PWM_ClearIntPending(PWM_MR0_INT);
    EXPECT_EQUAL(PWM_GetIntStatus(PWM_MR0_INT), RESET);

    ASSERT_TEST();
}

uint8_t PWM_ConfigCaptureTest(void) {
    PWM_Setup();
    TEST_INIT();

    const PWM_CAPTURECFG_T capCfg = {0, 1, 0, 1};
    PWM_ConfigCapture(&capCfg);

    EXPECT_EQUAL(LPC_PWM1->CCR & PWM_CCR_CHANNEL_MASKBIT(0),
                 PWM_CCR_CAP_RISING(0) | PWM_CCR_INT_ON_CAP(0));

    ASSERT_TEST();
}

uint8_t PWM_GetCaptureValueTest(void) {
    PWM_Setup();
    TEST_INIT();

    LPC_PINCON->PINSEL3 |= (0x2 << 24);
    LPC_PWM1->CCR |= PWM_CCR_CAP_FALLING(0);

    if (LPC_PWM1->CR0 == 0x00) {
        EXPECT_EQUAL(PWM_GetCaptureValue(PWM_CAPTURE_0), 0x00);
    } else {
        printf(
            "[⚠️ Warning] PWM_GetCaptureValueTest requires a full board reset before re-running.\n");
        __test_passed = 0;
        ASSERT_TEST();
    }

    PWM_CounterEnable();
    EDGE_INT_P1_HIGH(28);
    EXPECT_TRUE(PWM_GetCaptureValue(PWM_CAPTURE_0) != 0x00);

    ASSERT_TEST();
}

#endif  // UNIT_TESTING_ENABLED
