/**
 * @file    lpc17xx_adc_tests.c
 * @brief   Tests for the LPC17xx ADC driver.
 * @version V1.0
 * @date    8 August 2025
 */
#ifdef UNIT_TESTING_ENABLED

#include "lpc17xx_adc_tests.h"

static uint8_t ADC_InitTest(void);
static uint8_t ADC_DeInitTest(void);
static uint8_t ADC_BurstCmdTest(void);
static uint8_t ADC_PowerdownCmdTest(void);
static uint8_t ADC_StartCmdTest(void);
static uint8_t ADC_ChannelCmdTest(void);
static uint8_t ADC_EdgeStartConfigTest(void);
static uint8_t ADC_IntConfigTest(void);
static uint8_t ADC_GlobalGetStatusTest(void);
static uint8_t ADC_ChannelGetStatusTest(void);
static uint8_t ADC_GlobalGetDataTest(void);
static uint8_t ADC_ChannelGetDataTest(void);

void ADC_Setup(void) {
    ADC_Init(200000);
    ADC_ChannelEnable(ADC_CHANNEL_0);
}

void ADC_TearDown(void) {}

void ADC_RunTests(void) {
    RUN_TESTS_INIT();
    LPC_PINCON->PINSEL1 |= 0x1 << 14;
    LPC_PINCON->PINMODE1 |= 0x1 << 15;

    RUN_TEST(ADC_InitTest);
    RUN_TEST(ADC_DeInitTest);
    RUN_TEST(ADC_BurstCmdTest);
    RUN_TEST(ADC_PowerdownCmdTest);
    RUN_TEST(ADC_StartCmdTest);
    RUN_TEST(ADC_ChannelCmdTest);
    RUN_TEST(ADC_EdgeStartConfigTest);
    RUN_TEST(ADC_IntConfigTest);
    RUN_TEST(ADC_GlobalGetStatusTest);
    RUN_TEST(ADC_ChannelGetStatusTest);
    RUN_TEST(ADC_GlobalGetDataTest);
    RUN_TEST(ADC_ChannelGetDataTest);

    LPC_PINCON->PINSEL1 &= ~(0x1 << 14);
    LPC_PINCON->PINMODE1 &= ~(0x1 << 15);
    ADC_DeInit();
    RUN_TESTS_END("ADC");
}

static uint8_t ADC_InitTest(void) {
    TEST_INIT();

    ADC_Init(200000);
    EXPECT_EQUAL(LPC_ADC->ADCR, 0x1 << 21 | 0x1 << 8);
    EXPECT_EQUAL(LPC_SC->PCONP & (0x1 << 12), (0x1 << 12));

    ASSERT_TEST();
}

static uint8_t ADC_DeInitTest(void) {
    ADC_Setup();
    TEST_INIT();

    ADC_DeInit();
    EXPECT_EQUAL(LPC_SC->PCONP & (0x1 << 12), 0);

    ASSERT_TEST();
}

static uint8_t ADC_BurstCmdTest(void) {
    ADC_Setup();
    TEST_INIT();

    ADC_BurstEnable();
    EXPECT_EQUAL(LPC_ADC->ADCR & ADC_CR_BURST, ADC_CR_BURST);

    ADC_BurstDisable();
    EXPECT_EQUAL(LPC_ADC->ADCR & ADC_CR_BURST, 0x0);

    ASSERT_TEST();
}

static uint8_t ADC_PowerdownCmdTest(void) {
    ADC_Setup();
    TEST_INIT();

    ADC_PowerUp();
    EXPECT_EQUAL(LPC_ADC->ADCR & ADC_CR_PDN, ADC_CR_PDN);

    ADC_PowerDown();
    EXPECT_EQUAL(LPC_ADC->ADCR & ADC_CR_PDN, 0x0);

    ASSERT_TEST();
}

static uint8_t ADC_StartCmdTest(void) {
    ADC_Setup();
    TEST_INIT();

    ADC_StartCmd(ADC_START_NOW);
    EXPECT_EQUAL(LPC_ADC->ADCR & ADC_CR_START_MASK, ADC_CR_START_NOW);

    ADC_StartCmd(ADC_START_ON_EINT0);
    EXPECT_EQUAL(LPC_ADC->ADCR & ADC_CR_START_MASK, ADC_CR_START_EINT0);

    ADC_StartCmd(ADC_START_CONTINUOUS);
    EXPECT_EQUAL(LPC_ADC->ADCR & ADC_CR_START_MASK, 0x0);

    ASSERT_TEST();
}

static uint8_t ADC_ChannelCmdTest(void) {
    ADC_Setup();
    TEST_INIT();

    ADC_ChannelEnable(ADC_CHANNEL_1);
    EXPECT_EQUAL(LPC_ADC->ADCR & ADC_CR_CH_SEL(1), ADC_CR_CH_SEL(1));

    ADC_ChannelDisable(ADC_CHANNEL_1);
    EXPECT_EQUAL(LPC_ADC->ADCR & ADC_CR_CH_SEL(1), 0x0);

    ASSERT_TEST();
}

static uint8_t ADC_EdgeStartConfigTest(void) {
    ADC_Setup();
    TEST_INIT();

    ADC_EdgeStartConfig(ADC_START_ON_FALLING);
    EXPECT_EQUAL(LPC_ADC->ADCR & ADC_CR_EDGE, ADC_CR_EDGE);

    ADC_EdgeStartConfig(ADC_START_ON_RISING);
    EXPECT_EQUAL(LPC_ADC->ADCR & ADC_CR_EDGE, 0x0);

    ASSERT_TEST();
}

static uint8_t ADC_IntConfigTest(void) {
    ADC_Setup();
    TEST_INIT();

    ADC_IntEnable(ADC_INT_CH0);
    EXPECT_EQUAL(LPC_ADC->ADINTEN & ADC_INTEN_CH(0), ADC_INTEN_CH(0));

    ADC_IntDisable(ADC_INT_CH0);
    EXPECT_EQUAL(LPC_ADC->ADINTEN & ADC_INTEN_CH(0), 0x0);

    ADC_IntEnable(ADC_INT_GLOBAL);
    EXPECT_EQUAL(LPC_ADC->ADINTEN & ADC_INTEN_CH(8), ADC_INTEN_CH(8));

    ASSERT_TEST();
}

static uint8_t ADC_GlobalGetStatusTest(void) {
    ADC_Setup();
    TEST_INIT();

    ADC_StartCmd(ADC_START_NOW);
    PROPAGATION_DELAY();
    EXPECT_EQUAL(ADC_GlobalGetStatus(ADC_DATA_DONE), SET);
    EXPECT_EQUAL(ADC_GlobalGetStatus(ADC_DATA_DONE), RESET);

    ASSERT_TEST();
}

static uint8_t ADC_ChannelGetStatusTest(void) {
    ADC_Setup();
    TEST_INIT();

    ADC_StartCmd(ADC_START_NOW);
    PROPAGATION_DELAY();
    EXPECT_EQUAL(ADC_ChannelGetStatus(ADC_CHANNEL_0, ADC_DATA_DONE), SET);

    ASSERT_TEST();
}

static uint8_t ADC_GlobalGetDataTest(void) {
    ADC_Setup();
    TEST_INIT();

    ADC_StartCmd(ADC_START_NOW);
    PROPAGATION_DELAY();
    EXPECT_EQUAL(ADC_GlobalGetData(), (LPC_ADC->ADGDR >> 4) & 0xFFF);

    ASSERT_TEST();
}

static uint8_t ADC_ChannelGetDataTest(void) {
    ADC_Setup();
    TEST_INIT();

    ADC_StartCmd(ADC_START_NOW);
    PROPAGATION_DELAY();
    EXPECT_EQUAL(ADC_ChannelGetData(ADC_CHANNEL_0), (LPC_ADC->ADDR0 >> 4) & 0xFFF);

    ASSERT_TEST();
}

#endif  // UNIT_TESTING_ENABLED
