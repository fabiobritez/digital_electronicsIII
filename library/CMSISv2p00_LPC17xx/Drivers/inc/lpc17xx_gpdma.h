/**
 * @file    lpc17xx_gpdma.h
 * @brief   Contains all macro definitions and function prototypes
 *          support for GPDMA firmware library on LPC17xx
 * @version 2.0
 * @date    21. May. 2010
 * @author  NXP MCU SW Application Team
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
 * Last update: 28/02/2026, Author: David Trujillo Medina
 */

/* ---------------------------- Peripheral group ---------------------------- */
/** @defgroup GPDMA GPDMA (General Purpose Direct Memory Access)
 * @ingroup LPC1700CMSIS_FwLib_Drivers
 * @{
 */

#ifndef LPC17XX_GPDMA_H_
#define LPC17XX_GPDMA_H_

/* -------------------------------- Includes -------------------------------- */
#include "LPC17xx.h"
#include "lpc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------- Private Macros ----------------------------- */
/** @defgroup GPDMA_Private_Macros GPDMA Private Macros
 * @{
 */

#define GPDMA_NUM_CHANNELS ((8))

/* ---------------------------- BIT DEFINITIONS ----------------------------- */
/** Bit mask for an especific channel. */
#define GPDMA_ChannelBit(n)       ((1UL << n))
/** Bit mask for an especific request source. */
#define GPDMA_DMACSoftBReq_Src(n) ((1UL << n))

/** DMACConfig register, enable bit. */
#define GPDMA_DMACConfig_E ((0x01))
/** DMACConfig register, endianness configuration bit. */
#define GPDMA_DMACConfig_M ((0x02))

/** Transfer size bits mask. */
#define GPDMA_DMACCxControl_TransferSize(n) (((n & 0xFFF) << 0))
/** Source burst size bits mask. */
#define GPDMA_DMACCxControl_SBSize(n)       (((n & 0x07) << 12))
/** Destination burst size bits mask. */
#define GPDMA_DMACCxControl_DBSize(n)       (((n & 0x07) << 15))
/** Source transfer width bits mask. */
#define GPDMA_DMACCxControl_SWidth(n)       (((n & 0x07) << 18))
/** Destination transfer width bits mask. */
#define GPDMA_DMACCxControl_DWidth(n)       (((n & 0x07) << 21))
/** Source increment control bit. */
#define GPDMA_DMACCxControl_SI              ((1UL << 26))
/** Destination increment control bit. */
#define GPDMA_DMACCxControl_DI              ((1UL << 27))
/** Terminal count interrupt enable bit. */
#define GPDMA_DMACCxControl_I               ((1UL << 31))

/** Channel enable bit. */
#define GPDMA_DMACCxConfig_E                 ((1UL << 0))
/** Source peripheral bits mask. */
#define GPDMA_DMACCxConfig_SrcPeripheral(n)  (((n) << 1))
/** Destination peripheral bits mask. */
#define GPDMA_DMACCxConfig_DestPeripheral(n) (((n) << 6))
/** Transfer type bits mask. */
#define GPDMA_DMACCxConfig_TransferType(n)   (((n) << 11))
/** Interrupt error mask. */
#define GPDMA_DMACCxConfig_IE                ((1UL << 14))
/** Interrupt terminal count mask. */
#define GPDMA_DMACCxConfig_ITC               ((1UL << 15))
/** Active status bit. */
#define GPDMA_DMACCxConfig_A                 ((1UL << 17))
/** Halt status bit. */
#define GPDMA_DMACCxConfig_H                 ((1UL << 18))

/* ---------------------------- MASK DEFINITIONS ---------------------------- */
#define GPDMA_DMACIntStat_ALL ((0xFF))

/**
 * @}
 */

/* ------------------------------ Public Types ------------------------------ */
/** @defgroup GPDMA_Public_Types GPDMA Public Types
 * @{
 */

/**
 * @brief GPDMA Channel enumeration.
 */
typedef enum {
    GPDMA_CH_0 = 0,
    GPDMA_CH_1,
    GPDMA_CH_2,
    GPDMA_CH_3,
    GPDMA_CH_4,
    GPDMA_CH_5,
    GPDMA_CH_6,
    GPDMA_CH_7
} GPDMA_CH;
/** Check GPDMA channel option parameter. */
#define PARAM_GPDMA_CHANNEL(CH) ((CH >= GPDMA_CH_0) && (CH <= GPDMA_CH_7))

/**
 * @brief GPDMA Connection type enumeration.
 */
typedef enum {
    GPDMA_SSP0_Tx = 0,
    GPDMA_SSP0_Rx,
    GPDMA_SSP1_Tx,
    GPDMA_SSP1_Rx,
    GPDMA_ADC,
    GPDMA_I2S_C0,
    GPDMA_I2S_C1,
    GPDMA_DAC,
    GPDMA_UART0_Tx,
    GPDMA_UART0_Rx,
    GPDMA_UART1_Tx,
    GPDMA_UART1_Rx,
    GPDMA_UART2_Tx,
    GPDMA_UART2_Rx,
    GPDMA_UART3_Tx,
    GPDMA_UART3_Rx,
    GPDMA_MAT0_0,
    GPDMA_MAT0_1,
    GPDMA_MAT1_0,
    GPDMA_MAT1_1,
    GPDMA_MAT2_0,
    GPDMA_MAT2_1,
    GPDMA_MAT3_0,
    GPDMA_MAT3_1
} GPDMA_CONNECTION;
/** Check GPDMA connection option parameter. */
#define PARAM_GPDMA_CONNECTION(CONN) ((CONN >= GPDMA_SSP0_Tx) && (CONN <= GPDMA_MAT3_1))

/**
 * @brief GPDMA Transfer type enumeration.
 */
typedef enum {
    GPDMA_M2M = 0,
    GPDMA_M2P,
    GPDMA_P2M,
    GPDMA_P2P
} GPDMA_TRANSFER_TYPE;
/** Check GPDMA transfer type option parameter. */
#define PARAM_GPDMA_TRANSFER_TYPE(TYPE) ((TYPE >= GPDMA_M2M) && (TYPE <= GPDMA_P2P))

/**
 * @brief Burst size in Source and Destination enumeration.
 */
typedef enum {
    GPDMA_BSIZE_1 = 0,
    GPDMA_BSIZE_4,
    GPDMA_BSIZE_8,
    GPDMA_BSIZE_16,
    GPDMA_BSIZE_32,
    GPDMA_BSIZE_64,
    GPDMA_BSIZE_128,
    GPDMA_BSIZE_256,
    GPDMA_BURST_AUTO
} GPDMA_BURST_SIZE;
/** Check GPDMA burst size option parameter. */
#define PARAM_GPDMA_BURST_SIZE(SIZE) ((SIZE >= GPDMA_BSIZE_1) && (SIZE <= GPDMA_BURST_AUTO))

/**
 * @brief Transfer width in Source and Destination enumeration.
 */
typedef enum {
    GPDMA_BYTE = 0,
    GPDMA_HALFWORD,
    GPDMA_WORD,
    GPDMA_WIDTH_AUTO
} GPDMA_TRANSFER_WIDTH;
/** Check GPDMA transfer width option parameter. */
#define PARAM_GPDMA_TRANSFER_WIDTH(WIDTH) ((WIDTH >= GPDMA_BYTE) && (WIDTH <= GPDMA_WIDTH_AUTO))

/**
 * @brief GPDMA Request select enumeration.
 */
typedef enum {
    GPDMA_UART = 0,
    GPDMA_TIMER
} GPDMA_REQUEST_SELECT;
/** Check GPDMA request select option parameter. */
#define PARAM_GPDMA_REQUEST_SELECT(SEL) ((SEL == GPDMA_UART) || (SEL == GPDMA_TIMER))

/**
 * @brief GPDMA Status type enumeration
 */
typedef enum {
    GPDMA_INT,        /**< GPDMA Interrupt Status */
    GPDMA_INTTC,      /**< GPDMA Interrupt Terminal Count Request Status */
    GPDMA_INTERR,     /**< GPDMA Interrupt Error Status */
    GPDMA_RAW_INTTC,  /**< GPDMA Raw Interrupt Terminal Count Status */
    GPDMA_RAW_INTERR, /**< GPDMA Raw Error Interrupt Status */
    GPDMA_ENABLED_CH  /**< GPDMA Enabled Channel Status */
} GPDMA_STATUS_TYPE;
/** Check GPDMA status type option parameter. */
#define PARAM_GPDMA_STAT(TYPE) ((TYPE >= GPDMA_INT) && (TYPE <= GPDMA_ENABLED_CH))

/**
 * @brief GPDMA Interrupt clear status enumeration
 */
typedef enum {
    GPDMA_CLR_INTTC, /**< GPDMA Interrupt Terminal Count Request Clear */
    GPDMA_CLR_INTERR /**< GPDMA Interrupt Error Clear */
} GPDMA_CLEAR_INT;
/** Check GPDMA clear interrupt option parameter. */
#define PARAM_GPDMA_CLEAR_INT(TYPE) ((TYPE == GPDMA_CLR_INTTC) || (TYPE == GPDMA_CLR_INTERR))

/**
 * @brief GPDMA Endpoint configuration structure.
 */
typedef struct {
    GPDMA_TRANSFER_WIDTH width; /**< GPDMA_BYTE, GPDMA_HALFWORD, GPDMA_WORD, GPDMA_WIDTH_AUTO. */
    GPDMA_BURST_SIZE
    burst; /**< PDMA_BSIZE_x [1, 4, 8, 16, 32, 64, 128, 256] or GPDMA_BURST_AUTO. */
    FunctionalState increment; /**< ENABLE or DISABLE. */
} GPDMA_Endpoint_T;

/**
 * @brief GPDMA Channel configuration structure.
 */
typedef struct {
    GPDMA_CH channelNum;      /**< GPDMA_CH_x [0...7]. */
    uint32_t transferSize;    /**< Number of destination-bus transfers. Max 4095. */
    GPDMA_TRANSFER_TYPE type; /**< GPDMA_M2M, GPDMA_M2P, GPDMA_P2M, GPDMA_P2P. */

    uint32_t srcMemAddr;      /**< Memory address. Ignored if source is a peripheral. */
    uint32_t dstMemAddr;      /**< Memory address. Ignored if destination is a peripheral. */
    GPDMA_CONNECTION srcConn; /**< Source hardware request. Triggers the DMA to READ from source. */
    GPDMA_CONNECTION
    dstConn; /**< Destination hardware request. Triggers the DMA to WRITE to destination. */

    GPDMA_Endpoint_T src; /**< Source transfer configuration (width, burst size, increment). */
    GPDMA_Endpoint_T dst; /**< Destination transfer configuration (width, burst size, increment). */

    FunctionalState intTC;  /**< Terminal Count Interrupt. ENABLE or DISABLE. */
    FunctionalState intErr; /**< Error Interrupt. ENABLE or DISABLE. */

    uint32_t linkedList; /**< Address of next LLI for scatter/gather; set to 0 if not used. */
} GPDMA_Channel_CFG_T;

/**
 * @brief GPDMA Linker List Item structure.
 */
typedef struct {
    uint32_t srcAddr; /**< Source Address */
    uint32_t dstAddr; /**< Destination address */
    uint32_t nextLLI; /**< Next LLI address, otherwise set to '0' */
    uint32_t control; /**< GPDMA Control of this LLI */
} GPDMA_LLI_T;

/**
 * @}
 */

/* ---------------------------- Public Functions ---------------------------- */
/** @defgroup GPDMA_Public_Functions GPDMA Public Functions
 * @{
 */

/**
 * @brief Initializes the GPDMA controller and resets all channels.
 *
 * Enables the GPDMA clock in the PCONP register and proceeds to clear the configuration and control
 * registers of all 8 DMA channels. It also clears any pending terminal count or error interrupts
 * and enables the controller in the DMACConfig register.
 */
void GPDMA_Init(void);

/**
 * @brief De-initializes the GPDMA peripheral.
 *
 * Disables the GPDMA controller and waits for the hardware to acknowledge the disabled state before
 * cutting the peripheral clock via PCONP to save power.
 */
void GPDMA_DeInit(void);

/**
 * @brief Configures a specific GPDMA channel for a data transfer.
 *
 * Validates the transfer parameters and initializes the source and destination addresses, transfer
 * width, burst size, and flow control. It also manages peripheral connection mapping (DMA Request
 * Selection) and sets up interrupts for Terminal Count (TC) and Error conditions.
 *
 * @param dmaCfg Pointer to a GPDMA_Channel_CFG_T structure containing all transfer parameters.
 * @return SUCCESS if the configuration was applied, ERROR if the channel is busy or parameters are
 * invalid.
 */
Status GPDMA_SetupChannel(const GPDMA_Channel_CFG_T* dmaCfg);

/**
 * @brief Starts the DMA transfer on the specified channel.
 *
 * Sets the Enable bit in the DMACCConfig register to initiate the programmed transfer. The hardware
 * will begin moving data as soon as the source/destination requirements or peripheral requests are
 * met.
 *
 * @param channel The GPDMA channel to start (GPDMA_CHANNEL_x [0...7]).
 */
void GPDMA_ChannelStart(GPDMA_CH channel);

/**
 * @brief Forces an immediate stop of the DMA transfer.
 *
 * Clears the Enable bit in the DMACCConfig register. This action completes the current transaction
 * if one is in progress, but any data in the FIFO that has not been transferred will be lost. The
 * channel will be disabled and must be reconfigured before it can be started again.
 *
 * @param channel The GPDMA channel to stop (GPDMA_CHANNEL_x [0...7]).
 */
void GPDMA_ChannelStop(GPDMA_CH channel);

/**
 * @brief Stops the DMA transfer after finishing the current transaction.
 *
 * Sets the Halt bit to prevent new transactions and waits until the Active bit in the DMACCConfig
 * register is cleared by hardware. This ensures that no data is lost or corrupted during the stop
 * process.
 *
 * @param channel The GPDMA channel to stop gracefully (GPDMA_CHANNEL_x [0...7]).
 *
 * @note:
 * - The channel can't be restarted without reconfiguration after being stopped.
 */
void GPDMA_ChannelGracefulStop(GPDMA_CH channel);

/**
 * @brief Suspends the DMA transfer on the specified channel.
 *
 * Sets the Halt bit in the DMACCConfig register. The DMA controller will complete the current FIFO
 * transaction and then pause, maintaining the current transfer state.
 *
 * @param channel The GPDMA channel to pause (GPDMA_CHANNEL_x [0...7]).
 */
void GPDMA_ChannelPause(GPDMA_CH channel);

/**
 * @brief Resumes a previously paused DMA transfer.
 *
 * Clears the Halt bit in the DMACCConfig register, allowing the DMA controller to continue the
 * transfer from the point where it was suspended.
 *
 * @param channel The GPDMA channel to resume (GPDMA_CHANNEL_x [0...7]).
 */
void GPDMA_ChannelResume(GPDMA_CH channel);

/**
 * @brief Retrieves the current status of a specific GPDMA interrupt or channel state.
 *
 * Checks various hardware status registers to determine if an interrupt is pending, if a terminal
 * count has been reached, or if a channel is currently enabled. This includes support for reading
 * raw interrupt statuses, which are independent of the interrupt mask settings.
 *
 * @param type    The type of status to retrieve:
 *                - GPDMA_INT        : General interrupt status
 *                - GPDMA_INTTC      : Terminal count interrupt status
 *                - GPDMA_INTERR     : Error interrupt status
 *                - GPDMA_RAW_INTTC  : Raw terminal count status
 *                - GPDMA_RAW_INTERR : Raw error status
 *                - GPDMA_ENABLED_CH : Channel enabled status
 * @param channel The GPDMA channel to check (GPDMA_CH_x [0...7]).
 * @return SET if the specific status bit is active, RESET otherwise.
 */
IntStatus GPDMA_IntGetStatus(GPDMA_STATUS_TYPE type, GPDMA_CH channel);

/**
 * @brief Clears the pending interrupt flags for a specific GPDMA channel.
 *
 * Clears the terminal count or error interrupt flags for the specified channel by writing to the
 * corresponding clear register.
 *
 * @param type    The interrupt flag to clear (GPDMA_CLR_INTTC or GPDMA_CLR_INTERR).
 * @param channel The GPDMA channel affected (GPDMA_CH_x [0...7]).
 */
void GPDMA_ClearIntPending(GPDMA_CLEAR_INT type, GPDMA_CH channel);

/**
 * @brief      Generates a software DMA request for the specified peripheral connection.
 *
 * This function writes to the DMACSoftSReq register to initiate a DMA transfer on the given
 * peripheral connection line, without requiring a hardware event.
 *
 * @param[in]  line  Peripheral connection line (GPDMA_CONNECTION).
 *
 * @note
 * - Useful for starting DMA transfers from software.
 * - The `line` parameter must be a valid GPDMA_CONNECTION value.
 */
static __INLINE void DMA_SoftRequest(GPDMA_CONNECTION line) {
    if (line > 15) {
        LPC_GPDMA->DMACSoftSReq = GPDMA_DMACSoftBReq_Src((line - 8));
    } else {
        LPC_GPDMA->DMACSoftSReq = GPDMA_DMACSoftBReq_Src(line);
    }
}

/**
 * @brief      Generates a software burst DMA request for the specified peripheral connection.
 *
 * This function writes to the DMACSoftBReq register to initiate a DMA burst transfer
 * on the given peripheral connection line, without requiring a hardware event.
 *
 * @param[in]  line  Peripheral connection line (GPDMA_CONNECTION).
 *
 * @note
 * - Useful for starting DMA bursts from software.
 * - The `line` parameter must be a valid GPDMA_CONNECTION value.
 */
static __INLINE void DMA_SoftBurstRequest(GPDMA_CONNECTION line) {
    if (line > 15) {
        LPC_GPDMA->DMACSoftBReq = GPDMA_DMACSoftBReq_Src((line - 8));
    } else {
        LPC_GPDMA->DMACSoftBReq = GPDMA_DMACSoftBReq_Src(line);
    }
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LPC17XX_GPDMA_H_ */

/**
 * @}
 */

/* ------------------------------ End Of File ------------------------------- */
