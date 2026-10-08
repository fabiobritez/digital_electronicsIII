/**
 * @file     lpc17xx_systick_tests.c
 * @brief    Tests for the LPC17xx SYSTICK driver.
 * @version  V1.0
 * @date     16 July 2025
 */
#ifdef UNIT_TESTING_ENABLED

#include "lpc17xx_systick_tests.h"

uint8_t SYSTICK_InternalInitTest();
uint8_t SYSTICK_ExternalInitTest();
uint8_t SYSTICK_CmdTest();
uint8_t SYSTICK_IntCmdTest();
uint8_t SYSTICK_GetCurrentValueTest();
uint8_t SYSTICK_ClearCounterFlagTest();
uint8_t SYSTICK_GetReloadValueTest();
uint8_t SYSTICK_SetReloadValueTest();
uint8_t SYSTICK_IsActiveTest();
uint8_t SYSTICK_HasFiredTest();

void SYSTICK_Setup(void) {
    SysTick->CTRL &= ~(ST_CTRL_ENABLE | ST_CTRL_TICKINT);
    SysTick->CTRL |= ST_CTRL_CLKSOURCE;
    SysTick->CTRL;
    SysTick->LOAD &= ~ST_MAX_LOAD;
    SysTick->VAL = 0;
}

void SYSTICK_TearDown(void) {}

void SYSTICK_RunTests(void) {
    RUN_TESTS_INIT();

    RUN_TEST(SYSTICK_InternalInitTest);
    RUN_TEST(SYSTICK_ExternalInitTest);
    RUN_TEST(SYSTICK_CmdTest);
    RUN_TEST(SYSTICK_IntCmdTest);
    RUN_TEST(SYSTICK_GetCurrentValueTest);
    RUN_TEST(SYSTICK_ClearCounterFlagTest);
    RUN_TEST(SYSTICK_GetReloadValueTest);
    RUN_TEST(SYSTICK_SetReloadValueTest);
    RUN_TEST(SYSTICK_IsActiveTest);
    RUN_TEST(SYSTICK_HasFiredTest);

    SYSTICK_Setup();

    RUN_TESTS_END("SYSTICK");
}

uint8_t SYSTICK_InternalInitTest() {
    SYSTICK_Setup();
    TEST_INIT();

    const uint32_t time          = 10;
    const uint32_t expected_load = (SystemCoreClock / 1000) * time - 1;
    SYSTICK_InternalInit(time);

    EXPECT_EQUAL(SysTick->LOAD, expected_load);
    EXPECT_TRUE(SysTick->CTRL & ST_CTRL_CLKSOURCE);

    const uint32_t excessiveTime = 0xFFFFFFF;
    SYSTICK_InternalInit(excessiveTime);
    EXPECT_EQUAL(SysTick->LOAD, ST_MAX_LOAD);

    ASSERT_TEST();
}

uint8_t SYSTICK_ExternalInitTest() {
    SYSTICK_Setup();
    TEST_INIT();

    const uint32_t extFreq       = 1000000;
    const uint32_t time          = 20;
    const uint32_t expected_load = (extFreq / 1000) * time - 1;
    SYSTICK_ExternalInit(extFreq, time);

    EXPECT_EQUAL(SysTick->LOAD, expected_load);
    EXPECT_FALSE(SysTick->CTRL & ST_CTRL_CLKSOURCE);

    const uint32_t excessiveTime = 0xFFFFFFF;
    SYSTICK_ExternalInit(extFreq, excessiveTime);
    EXPECT_EQUAL(SysTick->LOAD, ST_MAX_LOAD);

    ASSERT_TEST();
}

uint8_t SYSTICK_CmdTest() {
    SYSTICK_Setup();
    TEST_INIT();

    SYSTICK_Cmd(ENABLE);
    EXPECT_TRUE(SysTick->CTRL & ST_CTRL_ENABLE);

    SYSTICK_Cmd(DISABLE);
    EXPECT_FALSE(SysTick->CTRL & ST_CTRL_ENABLE);

    ASSERT_TEST();
}

uint8_t SYSTICK_IntCmdTest() {
    SYSTICK_Setup();
    TEST_INIT();

    SYSTICK_IntCmd(ENABLE);
    EXPECT_TRUE(SysTick->CTRL & ST_CTRL_TICKINT);

    SYSTICK_IntCmd(DISABLE);
    EXPECT_FALSE(SysTick->CTRL & ST_CTRL_TICKINT);

    ASSERT_TEST();
}

uint8_t SYSTICK_GetCurrentValueTest() {
    SYSTICK_Setup();
    TEST_INIT();

    SysTick->LOAD = 0xFFFF;
    SysTick->CTRL |= 0x5;
    for (volatile unsigned int i = 0; i < 10; i++) {}
    const uint32_t firstVal  = SYSTICK_GetCurrentValue();
    const uint32_t secondVal = SYSTICK_GetCurrentValue();
    EXPECT_TRUE(firstVal < 0xFFFF);
    EXPECT_TRUE(secondVal < 0xFFFF);
    EXPECT_TRUE(firstVal != secondVal);
    EXPECT_TRUE(firstVal > secondVal);

    ASSERT_TEST();
}

uint8_t SYSTICK_ClearCounterFlagTest() {
    SYSTICK_Setup();
    TEST_INIT();

    SysTick->LOAD = 0xFF;
    SysTick->CTRL |= 0x5;
    for (volatile unsigned int i = 0; i < 1000; i++) {}
    uint32_t flag = SysTick->CTRL & ST_CTRL_COUNTFLAG;
    EXPECT_TRUE(flag);

    for (volatile unsigned int i = 0; i < 1000; i++) {}
    SysTick->CTRL &= ~ST_CTRL_ENABLE;
    SYSTICK_ClearCounterFlag();
    flag = SysTick->CTRL & ST_CTRL_COUNTFLAG;
    EXPECT_FALSE(flag);

    ASSERT_TEST();
}

uint8_t SYSTICK_GetReloadValueTest() {
    SYSTICK_Setup();
    TEST_INIT();

    SysTick->LOAD = 0x123456;
    EXPECT_EQUAL(SYSTICK_GetReloadValue(), 0x123456);

    SysTick->LOAD = 0xFFFFFF;
    EXPECT_EQUAL(SYSTICK_GetReloadValue(), 0xFFFFFF);

    SysTick->LOAD = 0x1FFFFFF;  // Only 24 bits should be used
    EXPECT_EQUAL(SYSTICK_GetReloadValue(), 0xFFFFFF);

    ASSERT_TEST();
}

uint8_t SYSTICK_SetReloadValueTest() {
    SYSTICK_Setup();
    TEST_INIT();

    SYSTICK_SetReloadValue(0x654321);
    EXPECT_EQUAL(SysTick->LOAD, 0x654321);

    SYSTICK_SetReloadValue(0xFFFFFF);
    EXPECT_EQUAL(SysTick->LOAD, 0xFFFFFF);

    SYSTICK_SetReloadValue(0x1FFFFFF);
    EXPECT_EQUAL(SysTick->LOAD, 0xFFFFFF);

    ASSERT_TEST();
}

uint8_t SYSTICK_IsActiveTest() {
    SYSTICK_Setup();
    TEST_INIT();

    SysTick->CTRL |= ST_CTRL_ENABLE;
    EXPECT_TRUE(SYSTICK_IsActive());

    SysTick->CTRL &= ~ST_CTRL_ENABLE;
    EXPECT_FALSE(SYSTICK_IsActive());

    ASSERT_TEST();
}

uint8_t SYSTICK_HasFiredTest() {
    SYSTICK_Setup();
    TEST_INIT();

    SysTick->LOAD = 0xFF;
    SysTick->CTRL |= 0x5;
    for (volatile unsigned int i = 0; i < 500; i++) {}
    EXPECT_TRUE(SYSTICK_HasFired());

    SysTick->LOAD = 0xFFFFFF;
    SysTick->VAL  = 0;
    SysTick->CTRL;
    for (volatile unsigned int i = 0; i < 10; i++) {}
    EXPECT_FALSE(SYSTICK_HasFired());

    ASSERT_TEST();
}

#endif  // UNIT_TESTING_ENABLED
