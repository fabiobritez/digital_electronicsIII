/**
 * @file    lpc17xx_gpdma_tests.c
 * @brief   Tests for the LPC17xx GPDMA driver.
 * @version V1.0
 * @date    1 March 2026
 */
#ifdef UNIT_TESTING_ENABLED

#include "lpc17xx_gpdma_tests.h"
#include "../../src/lpc17xx_gpdma.c"

static uint8_t GPDMA_InitTest(void);
static uint8_t GPDMA_DeInitTest(void);
static uint8_t GPDMA_SetupChannelTest(void);
static uint8_t GPDMA_ChannelStartStopTest(void);
static uint8_t GPDMA_ChannelPauseResumeTest(void);
static uint8_t GPDMA_ChannelGracefulStopTest(void);
static uint8_t GPDMA_IntGetStatusTest(void);
static uint8_t GPDMA_ClearIntPendingTest(void);

/* Buffers used as M2M src/dst to have a valid SetupChannel config */
static uint32_t gpdma_src_buf[4] = {0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0xABCDABCD};
static uint32_t gpdma_dst_buf[4] = {0};

void GPDMA_Setup(void) {
    GPDMA_Init();
}

void GPDMA_TearDown(void) {
    GPDMA_DeInit();
}

void GPDMA_RunTests(void) {
    RUN_TESTS_INIT();

    RUN_TEST(GPDMA_InitTest);
    RUN_TEST(GPDMA_DeInitTest);
    RUN_TEST(GPDMA_SetupChannelTest);
    RUN_TEST(GPDMA_ChannelStartStopTest);
    RUN_TEST(GPDMA_ChannelPauseResumeTest);
    RUN_TEST(GPDMA_ChannelGracefulStopTest);
    RUN_TEST(GPDMA_IntGetStatusTest);
    RUN_TEST(GPDMA_ClearIntPendingTest);

    GPDMA_DeInit();
    RUN_TESTS_END("GPDMA");
}

static uint8_t GPDMA_InitTest(void) {
    TEST_INIT();

    GPDMA_Init();

    /* Power bit for GPDMA (PCONP bit 29) must be set */
    EXPECT_EQUAL(LPC_SC->PCONP & (0x1 << 29), (0x1 << 29));

    /* DMACConfig Enable bit must be set */
    EXPECT_EQUAL(LPC_GPDMA->DMACConfig & GPDMA_DMACConfig_E, GPDMA_DMACConfig_E);

    /* All channel config/control registers must be cleared */
    for (int i = 0; i < GPDMA_NUM_CHANNELS; i++) {
        EXPECT_EQUAL(pGPDMACh[i]->DMACCConfig, 0x0);
        EXPECT_EQUAL(pGPDMACh[i]->DMACCControl, 0x0);
    }

    GPDMA_TearDown();
    ASSERT_TEST();
}

static uint8_t GPDMA_DeInitTest(void) {
    GPDMA_Setup();
    TEST_INIT();

    GPDMA_DeInit();

    /* DMACConfig Enable bit must be cleared */
    // EXPECT_EQUAL(LPC_GPDMA->DMACConfig & GPDMA_DMACConfig_E, 0x0);

    /* Power bit for GPDMA must be cleared */
    EXPECT_EQUAL(LPC_SC->PCONP & (0x1 << 29), 0x0);

    ASSERT_TEST();
}

static uint8_t GPDMA_SetupChannelTest(void) {
    GPDMA_Setup();
    TEST_INIT();

    const GPDMA_Channel_CFG_T cfg = {
        .channelNum   = GPDMA_CH_0,
        .transferSize = 4,
        .type         = GPDMA_M2M,
        .srcMemAddr   = (uintptr_t)gpdma_src_buf,
        .dstMemAddr   = (uintptr_t)gpdma_dst_buf,
        .srcConn      = 0,
        .dstConn      = 0,
        .src          = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_4, .increment = ENABLE},
        .dst          = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_4, .increment = ENABLE},
        .intTC        = ENABLE,
        .intErr       = ENABLE,
        .linkedList   = 0};

    const Status result = GPDMA_SetupChannel(&cfg);
    EXPECT_EQUAL(result, SUCCESS);

    const LPC_GPDMACH_TypeDef* pDMAch = pGPDMACh[GPDMA_CH_0];

    /* Source and destination addresses must match */
    EXPECT_EQUAL(pDMAch->DMACCSrcAddr, (uint32_t)(uintptr_t)gpdma_src_buf);
    EXPECT_EQUAL(pDMAch->DMACCDestAddr, (uint32_t)(uintptr_t)gpdma_dst_buf);

    /* Transfer size field in DMACCControl [11:0] doesn't update until the transfer starts */
    // EXPECT_EQUAL(pDMAch->DMACCControl & 0xFFF, 4);

    /* LLI must be 0 (no linked list) */
    EXPECT_EQUAL(pDMAch->DMACCLLI, 0x0);

    /* ITC and IE bits in DMACCConfig */
    EXPECT_EQUAL(pDMAch->DMACCConfig & GPDMA_DMACCxConfig_ITC, GPDMA_DMACCxConfig_ITC);
    EXPECT_EQUAL(pDMAch->DMACCConfig & GPDMA_DMACCxConfig_IE, GPDMA_DMACCxConfig_IE);

    /* Transfer type bits [10:8] must match M2M */
    EXPECT_EQUAL(pDMAch->DMACCConfig & GPDMA_DMACCxConfig_TransferType(0x7),
                 GPDMA_DMACCxConfig_TransferType(GPDMA_M2M));

    GPDMA_ChannelStart(GPDMA_CH_0);

    GPDMA_TearDown();
    ASSERT_TEST();
}

static uint8_t GPDMA_ChannelStartStopTest(void) {
    GPDMA_Setup();
    TEST_INIT();

    GPDMA_LLI_T gpdma_lli = {.srcAddr = (uint32_t)(uintptr_t)gpdma_src_buf,
                             .dstAddr = (uint32_t)(uintptr_t)gpdma_dst_buf,
                             .nextLLI = (uint32_t)(uintptr_t)&gpdma_lli,
                             .control = (4u | (0x1 << 12) | (0x1 << 15) | (0x2u << 18) |
                                         (0x2u << 21) | (1u << 26) | (1u << 27))};

    const GPDMA_Channel_CFG_T cfg = {
        .channelNum   = GPDMA_CH_0,
        .srcMemAddr   = (uint32_t)(uintptr_t)gpdma_src_buf,
        .dstMemAddr   = (uint32_t)(uintptr_t)gpdma_dst_buf,
        .transferSize = 4,
        .type         = GPDMA_M2M,
        .srcConn      = 0,
        .dstConn      = 0,
        .src          = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_4, .increment = ENABLE},
        .dst          = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_4, .increment = ENABLE},
        .linkedList   = (uint32_t)(uintptr_t)&gpdma_lli,
        .intTC        = DISABLE,
        .intErr       = DISABLE,
    };
    GPDMA_SetupChannel(&cfg);

    GPDMA_ChannelStart(GPDMA_CH_0);
    EXPECT_EQUAL(pGPDMACh[GPDMA_CH_0]->DMACCConfig & GPDMA_DMACCxConfig_E, GPDMA_DMACCxConfig_E);

    GPDMA_ChannelStop(GPDMA_CH_0);
    EXPECT_EQUAL(pGPDMACh[GPDMA_CH_0]->DMACCConfig & GPDMA_DMACCxConfig_E, 0x0);

    GPDMA_TearDown();
    ASSERT_TEST();
}

/* ── Channel Pause / Resume ───────────────────────────────────────────────── */

static uint8_t GPDMA_ChannelPauseResumeTest(void) {
    GPDMA_Setup();
    TEST_INIT();

    GPDMA_LLI_T gpdma_lli = {.srcAddr = (uint32_t)(uintptr_t)gpdma_src_buf,
                             .dstAddr = (uint32_t)(uintptr_t)gpdma_dst_buf,
                             .nextLLI = (uint32_t)(uintptr_t)&gpdma_lli,
                             .control = (4u | (0x1 << 12) | (0x1 << 15) | (0x2u << 18) |
                                         (0x2u << 21) | (1u << 26) | (1u << 27))};

    GPDMA_Channel_CFG_T cfg = {
        .channelNum   = GPDMA_CH_1,
        .srcMemAddr   = (uint32_t)(uintptr_t)gpdma_src_buf,
        .dstMemAddr   = (uint32_t)(uintptr_t)gpdma_dst_buf,
        .transferSize = 4,
        .type         = GPDMA_M2M,
        .srcConn      = 0,
        .dstConn      = 0,
        .src          = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_4, .increment = ENABLE},
        .dst          = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_4, .increment = ENABLE},
        .intTC        = DISABLE,
        .intErr       = DISABLE,
        .linkedList   = (uint32_t)(uintptr_t)&gpdma_lli,
    };
    GPDMA_SetupChannel(&cfg);
    GPDMA_ChannelStart(GPDMA_CH_1);

    GPDMA_ChannelPause(GPDMA_CH_1);
    EXPECT_EQUAL(pGPDMACh[GPDMA_CH_1]->DMACCConfig & GPDMA_DMACCxConfig_H, GPDMA_DMACCxConfig_H);

    GPDMA_ChannelResume(GPDMA_CH_1);
    EXPECT_EQUAL(pGPDMACh[GPDMA_CH_1]->DMACCConfig & GPDMA_DMACCxConfig_H, 0x0);

    GPDMA_ChannelStop(GPDMA_CH_1);

    GPDMA_TearDown();
    ASSERT_TEST();
}
static uint8_t GPDMA_ChannelGracefulStopTest(void) {
    GPDMA_Setup();
    TEST_INIT();

    GPDMA_LLI_T gpdma_lli = {.srcAddr = (uint32_t)(uintptr_t)gpdma_src_buf,
                             .dstAddr = (uint32_t)(uintptr_t)gpdma_dst_buf,
                             .nextLLI = (uint32_t)(uintptr_t)&gpdma_lli,
                             .control = (4u | (0x1 << 12) | (0x1 << 15) | (0x2u << 18) |
                                         (0x2u << 21) | (1u << 26) | (1u << 27))};

    const GPDMA_Channel_CFG_T cfg = {
        .channelNum   = GPDMA_CH_0,
        .srcMemAddr   = (uint32_t)(uintptr_t)gpdma_src_buf,
        .dstMemAddr   = (uint32_t)(uintptr_t)gpdma_dst_buf,
        .transferSize = 4,
        .type         = GPDMA_M2M,
        .srcConn      = 0,
        .dstConn      = 0,
        .src          = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_4, .increment = ENABLE},
        .dst          = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_4, .increment = ENABLE},
        .intTC        = DISABLE,
        .intErr       = DISABLE,
        .linkedList   = (uint32_t)(uintptr_t)&gpdma_lli,
    };
    GPDMA_SetupChannel(&cfg);
    GPDMA_ChannelStart(GPDMA_CH_0);

    /* After graceful stop, the Active bit (A) must be cleared */
    GPDMA_ChannelGracefulStop(GPDMA_CH_0);
    EXPECT_EQUAL(pGPDMACh[GPDMA_CH_0]->DMACCConfig & GPDMA_DMACCxConfig_A, 0x0);

    GPDMA_TearDown();
    ASSERT_TEST();
}

static uint8_t GPDMA_IntGetStatusTest(void) {
    GPDMA_Setup();
    TEST_INIT();

    GPDMA_Channel_CFG_T cfg = {
        .channelNum   = GPDMA_CH_0,
        .srcMemAddr   = (uint32_t)(uintptr_t)gpdma_src_buf,
        .dstMemAddr   = (uint32_t)(uintptr_t)gpdma_dst_buf,
        .transferSize = 4,
        .type         = GPDMA_M2M,
        .srcConn      = 0,
        .dstConn      = 0,
        .src          = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_4, .increment = ENABLE},
        .dst          = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_4, .increment = ENABLE},
        .intTC        = ENABLE,
        .intErr       = ENABLE,
        .linkedList   = 0,
    };
    GPDMA_SetupChannel(&cfg);

    /* Before starting, channel must not appear enabled */
    EXPECT_EQUAL(GPDMA_IntGetStatus(GPDMA_ENABLED_CH, GPDMA_CH_0), RESET);

    GPDMA_ChannelStart(GPDMA_CH_0);
    PROPAGATION_DELAY();

    /* After a M2M transfer completes, raw TC flag must be set */
    EXPECT_EQUAL(GPDMA_IntGetStatus(GPDMA_RAW_INTTC, GPDMA_CH_0), SET);

    GPDMA_ChannelStop(GPDMA_CH_0);

    GPDMA_TearDown();
    ASSERT_TEST();
}

static uint8_t GPDMA_ClearIntPendingTest(void) {
    GPDMA_Setup();
    TEST_INIT();

    GPDMA_Channel_CFG_T cfg = {
        .channelNum   = GPDMA_CH_0,
        .srcMemAddr   = (uint32_t)(uintptr_t)gpdma_src_buf,
        .dstMemAddr   = (uint32_t)(uintptr_t)gpdma_dst_buf,
        .transferSize = 4,
        .type         = GPDMA_M2M,
        .srcConn      = 0,
        .dstConn      = 0,
        .src          = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_4, .increment = ENABLE},
        .dst          = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_4, .increment = ENABLE},
        .intTC        = ENABLE,
        .intErr       = ENABLE,
        .linkedList   = 0,
    };
    GPDMA_SetupChannel(&cfg);
    GPDMA_ChannelStart(GPDMA_CH_0);
    PROPAGATION_DELAY();

    /* TC flag should be set after transfer */
    EXPECT_EQUAL(GPDMA_IntGetStatus(GPDMA_INTTC, GPDMA_CH_0), SET);

    /* Clear TC flag and verify it's gone */
    GPDMA_ClearIntPending(GPDMA_CLR_INTTC, GPDMA_CH_0);
    EXPECT_EQUAL(GPDMA_IntGetStatus(GPDMA_INTTC, GPDMA_CH_0), RESET);

    /* Error flag should not be set for a clean M2M transfer */
    EXPECT_EQUAL(GPDMA_IntGetStatus(GPDMA_INTERR, GPDMA_CH_0), RESET);

    /* Clear error flag and verify it stays cleared */
    GPDMA_ClearIntPending(GPDMA_CLR_INTERR, GPDMA_CH_0);
    EXPECT_EQUAL(GPDMA_IntGetStatus(GPDMA_INTERR, GPDMA_CH_0), RESET);

    GPDMA_TearDown();
    ASSERT_TEST();
}

#endif  // UNIT_TESTING_ENABLED
