/**
 * @file    lpc17xx_uart_tests.c
 * @brief   Tests for the LPC17xx UART driver.
 * @version V1.0
 * @date    3 March 2026
 */
#ifdef UNIT_TESTING_ENABLED

#include "lpc17xx_uart_tests.h"

static uint8_t UART_InitTest(void);
static uint8_t UART_DeInitTest(void);
static uint8_t UART_InitAllDataBitsTest(void);
static uint8_t UART_InitAllParitiesTest(void);
static uint8_t UART_InitAllStopBitsTest(void);
static uint8_t UART_FIFOConfigTest(void);
static uint8_t UART_TxEnableDisableTest(void);
static uint8_t UART_IntConfigTest(void);
static uint8_t UART_IntConfig1OnlyTest(void);
static uint8_t UART_GetLineStatusTest(void);
static uint8_t UART_CheckBusyTest(void);
static uint8_t UART_SendReceiveLoopbackTest(void);

static const UART_CFG_T uart_default_cfg = {
    .baudRate = 115200,
    .dataBits  = UART_DBITS_8,
    .parity    = UART_PARITY_NONE,
    .stopBits  = UART_STOPBIT_1,
};

static LPC_UART_TypeDef* const uart_all[4] = {UART0, UART1, UART2, UART3};

static const uint32_t uart_pconp_bit[4] = {
    (1u << 3),
    (1u << 4),
    (1u << 24),
    (1u << 25),
};

static void UART_SetupLoopbackPins(void) {
    /* P0.2 → TXD0 (PINSEL0[5:4] = 01)
     * P0.3 → RXD0 (PINSEL0[7:6] = 01) */
    LPC_PINCON->PINSEL0 |= (0x1 << 4) | (0x1 << 6);
    LPC_PINCON->PINSEL0 &= ~((0x2 << 4) | (0x2 << 6));
    LPC_PINCON->PINMODE0 |= (0x1 << 4) | (0x1 << 6);
    LPC_PINCON->PINMODE0 &= ~((0x2 << 4) | (0x2 << 6));
}

static void UART_ClearLoopbackPins(void) {
    LPC_PINCON->PINSEL0 &= ~((0x3 << 4) | (0x3 << 6));
}

void UART_Setup(void) {
    for (int i = 0; i < 4; i++) {
        UART_Init(uart_all[i], &uart_default_cfg);
    }
}

void UART_TearDown(void) {
    for (int i = 0; i < 4; i++) {
        UART_DeInit(uart_all[i]);
    }
}

void UART_RunTests(void) {
    RUN_TESTS_INIT();

    RUN_TEST(UART_InitTest);
    RUN_TEST(UART_DeInitTest);
    RUN_TEST(UART_InitAllDataBitsTest);
    RUN_TEST(UART_InitAllParitiesTest);
    RUN_TEST(UART_InitAllStopBitsTest);
    RUN_TEST(UART_FIFOConfigTest);
    RUN_TEST(UART_TxEnableDisableTest);
    RUN_TEST(UART_IntConfigTest);
    RUN_TEST(UART_IntConfig1OnlyTest);
    RUN_TEST(UART_GetLineStatusTest);
    RUN_TEST(UART_CheckBusyTest);
    RUN_TEST(UART_SendReceiveLoopbackTest);

    RUN_TESTS_END("UART");
}

static uint8_t UART_InitTest(void) {
    TEST_INIT();

    for (int i = 0; i < 4; i++) {
        UART_Init(uart_all[i], &uart_default_cfg);

        // Power bit must be set
        EXPECT_EQUAL(LPC_SC->PCONP & uart_pconp_bit[i], uart_pconp_bit[i]);

        // LCR: 8 data bits, no parity, 1 stop bit
        EXPECT_EQUAL(uart_all[i]->LCR & 0x0F, 0x03);

        // TER: transmitter must be disabled after init
        EXPECT_EQUAL(uart_all[i]->TER & UART_TER_TXEN, 0x00);
    }

    UART_TearDown();
    ASSERT_TEST();
}

static uint8_t UART_DeInitTest(void) {
    UART_Setup();
    TEST_INIT();

    for (int i = 0; i < 4; i++) {
        UART_DeInit(uart_all[i]);
        // PCONP: power bit must be clear
        EXPECT_EQUAL(LPC_SC->PCONP & uart_pconp_bit[i], 0x0);
    }

    ASSERT_TEST();
}

static uint8_t UART_InitAllDataBitsTest(void) {
    TEST_INIT();

    const UART_DATABITS bits[4] = {UART_DBITS_5, UART_DBITS_6, UART_DBITS_7,
                                       UART_DBITS_8};

    UART_CFG_T cfg = uart_default_cfg;

    for (int i = 0; i < 4; i++) {
        const uint32_t expected_lcr[4] = {0x00, 0x01, 0x02, 0x03};
        cfg.dataBits                   = bits[i];
        UART_Init(UART2, &cfg);
        // For every data bit setting, LCR[1:0] should match the expected value
        EXPECT_EQUAL(UART2->LCR & 0x03, expected_lcr[i]);
    }

    UART_DeInit(UART2);
    ASSERT_TEST();
}

static uint8_t UART_InitAllParitiesTest(void) {
    TEST_INIT();

    const UART_PARITY modes[5] = {
        UART_PARITY_NONE, UART_PARITY_ODD, UART_PARITY_EVEN, UART_PARITY_1, UART_PARITY_0,
    };

    UART_CFG_T cfg = uart_default_cfg;

    for (int i = 0; i < 5; i++) {
        const uint32_t expected[5] = {
            0x00, 0x01, 0x03, 0x05, 0x07,
        };
        cfg.parity = modes[i];
        UART_Init(UART2, &cfg);
        // For every parity setting, LCR[5:3] should match the expected value
        EXPECT_EQUAL((UART2->LCR >> 3) & 0x07, expected[i]);
    }

    UART_DeInit(UART2);
    ASSERT_TEST();
}

static uint8_t UART_InitAllStopBitsTest(void) {
    TEST_INIT();

    UART_CFG_T cfg = uart_default_cfg;

    cfg.stopBits = UART_STOPBIT_1;
    UART_Init(UART2, &cfg);
    EXPECT_EQUAL(UART2->LCR & (1 << 2), 0x0);

    cfg.stopBits = UART_STOPBIT_2;
    UART_Init(UART2, &cfg);
    EXPECT_EQUAL(UART2->LCR & (1 << 2), (1 << 2));

    UART_DeInit(UART2);
    ASSERT_TEST();
}

static uint8_t UART_FIFOConfigTest(void) {
    UART_Init(UART2, &uart_default_cfg);
    TEST_INIT();

    UART_FIFO_CFG_T fifo_cfg = {
        .resetRxBuf = ENABLE,
        .resetTxBuf = ENABLE,
        .dmaMode    = DISABLE,
        .level      = UART_FIFO_TRGLEV0,
    };

    UART_FIFOConfig(UART2, &fifo_cfg);

    EXPECT_EQUAL(UART2->IIR & (0x03 << 6), (0x03 << 6));

    EXPECT_EQUAL(UART2->LSR & UART_LSR_RDR, 0x0);

    EXPECT_EQUAL(UART2->LSR & UART_LSR_THRE, UART_LSR_THRE);

    UART_DeInit(UART2);
    ASSERT_TEST();
}

static uint8_t UART_TxEnableDisableTest(void) {
    UART_Setup();
    TEST_INIT();

    for (int i = 0; i < 4; i++) {
        UART_TxDisable(uart_all[i]);
        EXPECT_EQUAL(uart_all[i]->TER & UART_TER_TXEN, 0x0);

        UART_TxEnable(uart_all[i]);
        EXPECT_EQUAL(uart_all[i]->TER & UART_TER_TXEN, UART_TER_TXEN);
    }

    UART_TearDown();
    ASSERT_TEST();
}

static uint8_t UART_IntConfigTest(void) {
    UART_Setup();
    TEST_INIT();

    const UART_INT common_ints[3] = {
        UART_INT_RBR,
        UART_INT_THRE,
        UART_INT_RLS,
    };
    const uint32_t ier_bits[3] = {
        UART_IER_RBRINT_EN,
        UART_IER_THREINT_EN,
        UART_IER_RLSINT_EN,
    };

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            UART_IntConfig(uart_all[i], common_ints[j], ENABLE);
            EXPECT_EQUAL(uart_all[i]->IER & ier_bits[j], ier_bits[j]);

            UART_IntConfig(uart_all[i], common_ints[j], DISABLE);
            EXPECT_EQUAL(uart_all[i]->IER & ier_bits[j], 0x0);
        }
    }

    UART_TearDown();
    ASSERT_TEST();
}

static uint8_t UART_IntConfig1OnlyTest(void) {
    UART_Init(UART1, &uart_default_cfg);
    TEST_INIT();

    UART_IntConfig(UART1, UART1_INT_MS, ENABLE);
    EXPECT_EQUAL(UART1->IER & UART1_IER_MSINT_EN, UART1_IER_MSINT_EN);

    UART_IntConfig(UART1, UART1_INT_MS, DISABLE);
    EXPECT_EQUAL(UART1->IER & UART1_IER_MSINT_EN, 0x0);

    // MCR must be set to use CTSINT bit
    LPC_UART1->MCR |= 0x1 << 7;
    UART_IntConfig(UART1, UART1_INT_CTS, ENABLE);
    EXPECT_EQUAL(UART1->IER & UART1_IER_CTSINT_EN, UART1_IER_CTSINT_EN);

    UART_IntConfig(UART1, UART1_INT_CTS, DISABLE);
    EXPECT_EQUAL(UART1->IER & UART1_IER_CTSINT_EN, 0x0);

    UART_DeInit(UART1);
    ASSERT_TEST();
}

static uint8_t UART_GetLineStatusTest(void) {
    UART_Setup();
    TEST_INIT();

    for (int i = 0; i < 4; i++) {
        const uint8_t lsr = UART_GetLineStatus(uart_all[i]);
        EXPECT_EQUAL(lsr & UART_LINESTAT_THRE, UART_LINESTAT_THRE);
        EXPECT_EQUAL(lsr & UART_LINESTAT_TEMT, UART_LINESTAT_TEMT);
    }

    UART_TearDown();
    ASSERT_TEST();
}

static uint8_t UART_CheckBusyTest(void) {
    UART_Setup();
    TEST_INIT();

    for (int i = 0; i < 4; i++) {
        EXPECT_EQUAL(UART_CheckBusy(uart_all[i]), RESET);
    }

    UART_TearDown();
    ASSERT_TEST();
}

static uint8_t UART_SendReceiveLoopbackTest(void) {
    printf("[ℹ️ INFO] UART_SendReceiveLoopbackTest: P0.2 (TX0) to P0.3 (RX0) must be connected.\n");
    UART_Init(UART0, &uart_default_cfg);
    TEST_INIT();
    UART_SetupLoopbackPins();

    LPC_UART0->FCR = 0x01;
    LPC_UART0->FCR |= 0x3 << 6;

    const uint8_t tx_buf[] = {0x55, 0xAA, 0x0F, 0xF0, 0x5A};
    const uint32_t buf_len = sizeof(tx_buf);
    uint8_t rx_buf[sizeof(tx_buf)];

    UART_TxEnable(UART0);
    uint32_t sent = UART_Send(UART0, tx_buf, buf_len, BLOCKING);
    EXPECT_EQUAL(sent, buf_len);

    uint32_t recv = UART_Receive(UART0, rx_buf, buf_len, BLOCKING);
    EXPECT_EQUAL(recv, buf_len);

    for (uint32_t i = 0; i < buf_len; i++) {
        EXPECT_EQUAL(rx_buf[i], tx_buf[i]);
    }

    UART_ClearLoopbackPins();
    UART_DeInit(UART0);
    ASSERT_TEST();
}

#endif  // UNIT_TESTING_ENABLED
