/**
 * @file     lpc17xx_pinsel_tests.c
 * @brief    Tests for the LPC17xx PINSEL driver.
 * @version  V1.0
 * @date     11 July 2025
 */
#ifdef UNIT_TESTING_ENABLED

#include "lpc17xx_pinsel_tests.h"

static PINSEL_CFG_T pinCfg = {0};

uint8_t PINSEL_ConfigPinFunctionLowTest(void);
uint8_t PINSEL_ConfigPinFunctionHighTest(void);
uint8_t PINSEL_ConfigPinModeLowTest(void);
uint8_t PINSEL_ConfigPinModeHighTest(void);
uint8_t PINSEL_ConfigPinODTest(void);
uint8_t PINSEL_ConfigMultiplePinsTest(void);

/**
 * @brief Guarantees that all PINCON registers are set to its default state
 *        for ports 0, 1, and 2.
 */
void PINSEL_Setup(void) {
    for (uint8_t i = 0; i < 5; i++) {
        *(&LPC_PINCON->PINSEL0 + i)  = 0x0;
        *(&LPC_PINCON->PINMODE0 + i) = 0x0;
    }
    for (uint8_t i = 0; i < 3; i++) {
        *(&LPC_PINCON->PINMODE_OD0 + i) = 0x0;
    }
}

void PINSEL_TearDown(void) {}

void PINSEL_RunTests(void) {
    RUN_TESTS_INIT();

    RUN_TEST(PINSEL_ConfigPinFunctionLowTest);
    RUN_TEST(PINSEL_ConfigPinFunctionHighTest);
    RUN_TEST(PINSEL_ConfigPinModeLowTest);
    RUN_TEST(PINSEL_ConfigPinModeHighTest);
    RUN_TEST(PINSEL_ConfigPinODTest);
    RUN_TEST(PINSEL_ConfigMultiplePinsTest);

    PINSEL_Setup();

    RUN_TESTS_END("PINSEL");
}

uint8_t PINSEL_ConfigPinFunctionLowTest(void) {
    PINSEL_Setup();
    TEST_INIT();

    pinCfg.pin  = PIN_2;
    pinCfg.func = PINSEL_FUNC_10;
    PINSEL_ConfigPin(&pinCfg);
    EXPECT_EQUAL(((LPC_PINCON->PINSEL0 >> (pinCfg.pin * 2)) & 0x3), pinCfg.func);

    ASSERT_TEST();
}

uint8_t PINSEL_ConfigPinFunctionHighTest(void) {
    PINSEL_Setup();
    TEST_INIT();

    pinCfg.pin  = PIN_19;
    pinCfg.func = PINSEL_FUNC_10;
    PINSEL_ConfigPin(&pinCfg);

    EXPECT_EQUAL(((LPC_PINCON->PINSEL1 >> ((pinCfg.pin - 16) * 2)) & 0x3), pinCfg.func);

    ASSERT_TEST();
}

uint8_t PINSEL_ConfigPinModeLowTest(void) {
    PINSEL_Setup();
    TEST_INIT();

    pinCfg.pin  = PIN_3;
    pinCfg.mode = PINSEL_TRISTATE;
    PINSEL_ConfigPin(&pinCfg);

    EXPECT_EQUAL(((LPC_PINCON->PINMODE0 >> (pinCfg.pin * 2)) & 0x3), pinCfg.mode);

    ASSERT_TEST();
}

uint8_t PINSEL_ConfigPinModeHighTest(void) {
    PINSEL_Setup();
    TEST_INIT();

    pinCfg.pin  = PIN_16;
    pinCfg.mode = PINSEL_TRISTATE;
    PINSEL_ConfigPin(&pinCfg);

    EXPECT_EQUAL(((LPC_PINCON->PINMODE1 >> ((pinCfg.pin - 16) * 2)) & 0x3), pinCfg.mode);

    ASSERT_TEST();
}

uint8_t PINSEL_ConfigPinODTest(void) {
    PINSEL_Setup();
    TEST_INIT();

    pinCfg.pin       = PIN_17;
    pinCfg.openDrain = ENABLE;
    PINSEL_ConfigPin(&pinCfg);

    EXPECT_EQUAL(((LPC_PINCON->PINMODE_OD0 >> pinCfg.pin) & 0x1), pinCfg.openDrain);

    ASSERT_TEST();
}

uint8_t PINSEL_ConfigMultiplePinsTest(void) {
    PINSEL_Setup();
    TEST_INIT();

    pinCfg.port      = PORT_0;
    pinCfg.func      = PINSEL_FUNC_01;
    pinCfg.mode      = PINSEL_REPEATER;
    pinCfg.openDrain = ENABLE;

    PINSEL_ConfigMultiplePins(&pinCfg, 0x00F003FF);

    EXPECT_EQUAL((LPC_PINCON->PINSEL0 & PORT_0_MASK_LOW), (0x55555 & PORT_0_MASK_LOW));
    EXPECT_EQUAL((LPC_PINCON->PINSEL1 & PORT_0_MASK_LOW), (0x5500 & PORT_0_MASK_LOW));

    EXPECT_EQUAL((LPC_PINCON->PINMODE0 & PORT_0_MASK_LOW), (0x55555 & PORT_0_MASK_LOW));
    EXPECT_EQUAL((LPC_PINCON->PINMODE1 & PORT_0_MASK_LOW), (0x5500 & PORT_0_MASK_LOW));

    EXPECT_EQUAL((LPC_PINCON->PINMODE_OD0 & PORT_0_MASK), (0x00F003FF & PORT_0_MASK));

    ASSERT_TEST();
}

#endif  // UNIT_TESTING_ENABLED
