/**
 * @file     lpc17xx_exti_tests.c
 * @brief    Tests for the LPC17xx EINT driver.
 * @version  V1.0
 * @date     11 July 2025
 */
#ifdef UNIT_TESTING_ENABLED

#include "lpc17xx_exti_tests.h"
#include "test_utils.h"

uint8_t EXTI_InitTest(void);
uint8_t EXTI_ConfigTest(void);
uint8_t EXTI_ConfigEnableTest(void);
uint8_t EXTI_ClearFlagTest(void);
uint8_t EXTI_GetFlagTest(void);
uint8_t EXTI_EnableIRQTest(void);

void EXTI_Setup(void) {
    NVIC_DisableIRQ(EINT0_IRQn);
    NVIC_DisableIRQ(EINT1_IRQn);
    NVIC_DisableIRQ(EINT2_IRQn);
    NVIC_DisableIRQ(EINT3_IRQn);

    LPC_SC->EXTMODE  = 0x0;
    LPC_SC->EXTPOLAR = 0x0;
    LPC_SC->EXTINT |= 0xF;
}

void EXTI_TearDown(void) {}

void EXTI_RunTests(void) {
    RUN_TESTS_INIT();

    LPC_PINCON->PINSEL4 = (LPC_PINCON->PINSEL4 & ~(0xFF << 20)) | (0x55 << 20);

    RUN_TEST(EXTI_InitTest);
    RUN_TEST(EXTI_ConfigTest);
    RUN_TEST(EXTI_ConfigEnableTest);
    RUN_TEST(EXTI_ClearFlagTest);
    RUN_TEST(EXTI_GetFlagTest);
    RUN_TEST(EXTI_EnableIRQTest);

    LPC_PINCON->PINSEL4 &= ~(0xFF << 20);
    EXTI_Setup();

    RUN_TESTS_END("EXTI");
}

uint8_t EXTI_InitTest(void) {
    TEST_INIT();

    LPC_SC->EXTMODE  = 0xF;
    LPC_SC->EXTPOLAR = 0xF;

    EXTI_Init();
    EXPECT_EQUAL((LPC_SC->EXTMODE & EXTI_MASK), 0x0);
    EXPECT_EQUAL((LPC_SC->EXTPOLAR & EXTI_MASK), 0x0);

    ASSERT_TEST();
}

uint8_t EXTI_ConfigTest(void) {
    EXTI_Setup();
    TEST_INIT();

    const EXTI_CFG_T cfg = {
        .line = EXTI_EINT1, .mode = EXTI_EDGE_SENSITIVE, .polarity = EXTI_RISING_EDGE};

    EXTI_Config(&cfg);

    EXPECT_TRUE(LPC_SC->EXTMODE & (1 << cfg.line));
    EXPECT_TRUE(LPC_SC->EXTPOLAR & (1 << cfg.line));

    ASSERT_TEST();
}

uint8_t EXTI_ConfigEnableTest(void) {
    EXTI_Setup();
    TEST_INIT();

    const EXTI_CFG_T cfg = {
        .line = EXTI_EINT3, .mode = EXTI_EDGE_SENSITIVE, .polarity = EXTI_RISING_EDGE};

    EXTI_Config(&cfg);

    EXPECT_TRUE(LPC_SC->EXTMODE & (1 << cfg.line));
    EXPECT_TRUE(LPC_SC->EXTPOLAR & (1 << cfg.line));

    ASSERT_TEST();
}

uint8_t EXTI_ClearFlagTest(void) {
    EXTI_Setup();
    TEST_INIT();

    const EXTI_CFG_T cfg = {
        .line = EXTI_EINT3, .mode = EXTI_EDGE_SENSITIVE, .polarity = EXTI_RISING_EDGE};
    EXTI_Config(&cfg);

    EDGE_INT_P2_LOW(13);
    EXPECT_TRUE(LPC_SC->EXTINT & (1 << cfg.line));

    EXTI_ClearFlag(EXTI_EINT3);
    EXPECT_FALSE(LPC_SC->EXTINT & (1 << cfg.line));

    ASSERT_TEST();
}

uint8_t EXTI_GetFlagTest(void) {
    EXTI_Setup();
    TEST_INIT();

    const EXTI_CFG_T cfg = {
        .line = EXTI_EINT3, .mode = EXTI_EDGE_SENSITIVE, .polarity = EXTI_RISING_EDGE};
    EXTI_Config(&cfg);
    EXTI_ClearFlag(EXTI_EINT1);
    EXTI_ClearFlag(EXTI_EINT3);
    EXPECT_FALSE(EXTI_GetFlag(EXTI_EINT1));
    EXPECT_FALSE(EXTI_GetFlag(EXTI_EINT3));

    EDGE_INT_P2_LOW(13);

    EXPECT_FALSE(EXTI_GetFlag(EXTI_EINT1));
    EXPECT_TRUE(EXTI_GetFlag(EXTI_EINT3));

    ASSERT_TEST();
}

uint8_t EXTI_EnableIRQTest(void) {
    EXTI_Setup();
    TEST_INIT();

    EXTI_EnableIRQ(EXTI_EINT0);

    EXPECT_FALSE(LPC_SC->EXTINT & (1 << EXTI_EINT0));

    ASSERT_TEST();
}

#endif  // UNIT_TESTING_ENABLED
