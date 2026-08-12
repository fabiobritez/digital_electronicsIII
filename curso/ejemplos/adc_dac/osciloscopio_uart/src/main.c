/*
 * Osciloscopio didactico para LPCXpresso LPC1769.
 *
 * Hardware:
 *   P0.26/AOUT -> P0.23/AD0.0
 *   P0.2/TXD0  -> RXD del CP2102
 *   GND        -> GND del CP2102
 *
 * El DAC se actualiza a 1 MS/s por GPDMA y genera una senoide de 20 kHz con
 * 50 valores por periodo. MAT0.1 dispara el ADC a 189,394 kS/s y el GPDMA
 * copia ADGDR a RAM sin atender una interrupcion por muestra.
 *
 * SCOPE_MODE=1: ventanas de 1024 muestras, 12 bits, con pausas de adquisicion.
 * SCOPE_MODE=2: adquisicion continua, decimada por 3 y enviada en 8 bits.
 */

#include <stdint.h>

#include "LPC17xx.h"

#define SCOPE_MODE_BLOCK_12BIT       1
#define SCOPE_MODE_STREAM_8BIT       2

#if SCOPE_MODE != SCOPE_MODE_BLOCK_12BIT && \
    SCOPE_MODE != SCOPE_MODE_STREAM_8BIT
#error "SCOPE_MODE debe ser 1 o 2"
#endif

#define CPU_CLOCK_HZ                 100000000u
#define UART_BAUD_CONFIGURED         921600u
#define ADC_CLOCK_HZ                 12500000u
#define ADC_CAPTURE_RATE_HZ          189394u
#define DAC_UPDATE_RATE_HZ           1000000u
#define DAC_SIGNAL_RATE_HZ           20000u
#define DAC_TABLE_SAMPLES            50u

#define BLOCK_RAW_SAMPLES            1024u
#define BLOCK_DMA_SAMPLES            (BLOCK_RAW_SAMPLES + 1u)
#define STREAM_RAW_SAMPLES           768u
#define STREAM_DECIMATION            3u
#define STREAM_OUTPUT_SAMPLES        (STREAM_RAW_SAMPLES / STREAM_DECIMATION)

#define FRAME_HEADER_SIZE            32u
#define FRAME_MAX_PAYLOAD            (BLOCK_RAW_SAMPLES * 2u)
#define FRAME_MAGIC_0                 'L'
#define FRAME_MAGIC_1                 'P'
#define FRAME_MAGIC_2                 'C'
#define FRAME_MAGIC_3                 'S'
#define FRAME_VERSION                 1u

#define FRAME_FLAG_CAPTURE_GAPS       (1u << 0)
#define FRAME_FLAG_ADC_OVERRUN        (1u << 1)
#define FRAME_FLAG_DMA_ERROR          (1u << 2)
#define FRAME_FLAG_DECIMATED          (1u << 3)
#define FRAME_FLAG_CAPTURE_LOSS       (1u << 4)

#define DMA_CHANNEL_ADC               0u
#define DMA_CHANNEL_DAC               1u
#define DMA_CONN_ADC                  4u
#define DMA_CONN_DAC                  7u

#define DMA_CTRL_TRANSFER_SIZE(n)     ((uint32_t) (n) & 0x0FFFu)
#define DMA_CTRL_SRC_WIDTH_WORD       (2u << 18)
#define DMA_CTRL_DST_WIDTH_WORD       (2u << 21)
#define DMA_CTRL_SRC_INCREMENT        (1u << 26)
#define DMA_CTRL_DST_INCREMENT        (1u << 27)
#define DMA_CTRL_TC_INTERRUPT         (1u << 31)

#define DMA_CFG_ENABLE                (1u << 0)
#define DMA_CFG_SRC_PERIPHERAL(n)     ((uint32_t) (n) << 1)
#define DMA_CFG_DST_PERIPHERAL(n)     ((uint32_t) (n) << 6)
#define DMA_CFG_M2P                   (1u << 11)
#define DMA_CFG_P2M                   (2u << 11)
#define DMA_CFG_ERROR_INTERRUPT       (1u << 14)
#define DMA_CFG_TC_INTERRUPT          (1u << 15)

#define ADC_CR_START_MASK             (0x7u << 24)
#define ADC_CR_START_MAT01            (0x4u << 24)
#define ADC_GDR_RESULT(word)          (((word) >> 4) & 0x0FFFu)
#define ADC_GDR_OVERRUN               (1u << 30)

#define UART_LSR_THRE                 (1u << 5)
#define UART_FIFO_SIZE                16u

typedef struct {
    uint32_t source;
    uint32_t destination;
    uint32_t next;
    uint32_t control;
} DmaLli;

/* Amplitud 450 sobre un offset de 512: la salida queda aproximadamente entre
 * 0,20 V y 3,10 V. Esta tabla chica vive en Flash y la CPU la copia a AHB
 * SRAM al arrancar. GPDMA no puede leer ni Flash ni la SRAM principal. */
static const uint16_t dac_sine_values[DAC_TABLE_SAMPLES] = {
    512u, 568u, 624u, 678u, 729u,
    777u, 820u, 859u, 892u, 919u,
    940u, 954u, 961u, 961u, 954u,
    940u, 919u, 892u, 859u, 820u,
    777u, 729u, 678u, 624u, 568u,
    512u, 456u, 400u, 346u, 295u,
    247u, 204u, 165u, 132u, 105u,
    84u, 70u, 63u, 63u, 70u,
    84u, 105u, 132u, 165u, 204u,
    247u, 295u, 346u, 400u, 456u
};

static uint32_t dac_sine_table[DAC_TABLE_SAMPLES]
    __attribute__((section(".ahbram1"), aligned(4)));
static DmaLli dac_lli
    __attribute__((section(".ahbram1"), aligned(4)));

static uint32_t adc_buffer_a[BLOCK_DMA_SAMPLES]
    __attribute__((section(".ahbram0"), aligned(4)));

#if SCOPE_MODE == SCOPE_MODE_STREAM_8BIT
static uint32_t adc_buffer_b[STREAM_RAW_SAMPLES]
    __attribute__((section(".ahbram0"), aligned(4)));
static DmaLli adc_lli_a
    __attribute__((section(".ahbram0"), aligned(4)));
static DmaLli adc_lli_b
    __attribute__((section(".ahbram0"), aligned(4)));
#endif

static uint8_t tx_frame[FRAME_HEADER_SIZE + FRAME_MAX_PAYLOAD]
    __attribute__((aligned(4)));

static volatile uint32_t adc_block_done;
static volatile uint32_t dma_error_count;

#if SCOPE_MODE == SCOPE_MODE_STREAM_8BIT
static volatile uint32_t stream_ready;
static volatile uint32_t stream_ready_index;
static volatile uint32_t stream_ready_sequence;
static volatile uint32_t stream_next_completed;
static volatile uint32_t stream_completed_blocks;
static volatile uint32_t stream_lost_blocks;
#endif

static void put_u16_le(uint8_t *destination, uint16_t value)
{
    destination[0] = (uint8_t) value;
    destination[1] = (uint8_t) (value >> 8);
}

static void put_u32_le(uint8_t *destination, uint32_t value)
{
    destination[0] = (uint8_t) value;
    destination[1] = (uint8_t) (value >> 8);
    destination[2] = (uint8_t) (value >> 16);
    destination[3] = (uint8_t) (value >> 24);
}

static uint16_t crc16_ccitt_update(uint16_t crc, const uint8_t *data,
                                   uint32_t length)
{
    for (uint32_t i = 0u; i < length; i++) {
        crc ^= (uint16_t) data[i] << 8;
        for (uint32_t bit = 0u; bit < 8u; bit++) {
            if ((crc & 0x8000u) != 0u) {
                crc = (uint16_t) ((crc << 1) ^ 0x1021u);
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

static void uart0_init(void)
{
    LPC_SC->PCONP |= (1u << 3);

    /* PCLK_UART0 = CCLK. DL=5, MULVAL=14 y DIVADDVAL=5 producen
     * 921052,6 baud, con un error de -0,06 %. */
    LPC_SC->PCLKSEL0 &= ~(0x3u << 6);
    LPC_SC->PCLKSEL0 |=  (0x1u << 6);

    LPC_PINCON->PINSEL0 &= ~(0x3u << 4);
    LPC_PINCON->PINSEL0 |=  (0x1u << 4);       /* P0.2 = TXD0 */

    LPC_UART0->LCR = 0x03u | (1u << 7);       /* 8N1, DLAB = 1 */
    LPC_UART0->DLM = 0u;
    LPC_UART0->DLL = 5u;
    LPC_UART0->FDR = (14u << 4) | 5u;
    LPC_UART0->LCR = 0x03u;
    LPC_UART0->IER = 0u;
    LPC_UART0->FCR = 0x07u;                   /* FIFO habilitada y limpia */
    LPC_UART0->TER = (1u << 7);
}

static void uart0_write(const uint8_t *data, uint32_t length)
{
    uint32_t sent = 0u;

    while (sent < length) {
        while ((LPC_UART0->LSR & UART_LSR_THRE) == 0u) {
        }

        uint32_t chunk = length - sent;
        if (chunk > UART_FIFO_SIZE) {
            chunk = UART_FIFO_SIZE;
        }

        for (uint32_t i = 0u; i < chunk; i++) {
            LPC_UART0->THR = data[sent + i];
        }
        sent += chunk;
    }
}

static void gpdma_init(void)
{
    LPC_SC->PCONP |= (1u << 29);
    __DSB();

    LPC_GPDMACH0->DMACCConfig = 0u;
    LPC_GPDMACH1->DMACCConfig = 0u;
    LPC_GPDMA->DMACIntTCClear = 0xFFu;
    LPC_GPDMA->DMACIntErrClr = 0xFFu;
    LPC_GPDMA->DMACConfig = 1u;
    while ((LPC_GPDMA->DMACConfig & 1u) == 0u) {
    }

    NVIC_DisableIRQ(DMA_IRQn);
    NVIC_ClearPendingIRQ(DMA_IRQn);
    NVIC_SetPriority(DMA_IRQn, 1u);
    NVIC_EnableIRQ(DMA_IRQn);
}

static void dac_start(void)
{
    /* PCLK_DAC = CCLK/4 = 25 MHz. */
    LPC_SC->PCLKSEL0 &= ~(0x3u << 22);

    LPC_PINCON->PINSEL1 &= ~(0x3u << 20);
    LPC_PINCON->PINSEL1 |=  (0x2u << 20);     /* P0.26 = AOUT */
    LPC_PINCON->PINMODE1 &= ~(0x3u << 20);
    LPC_PINCON->PINMODE1 |=  (0x2u << 20);   /* sin pull-up ni pull-down */

    for (uint32_t i = 0u; i < DAC_TABLE_SAMPLES; i++) {
        dac_sine_table[i] = (uint32_t) dac_sine_values[i] << 6;
    }

    dac_lli.source = (uint32_t) dac_sine_table;
    dac_lli.destination = (uint32_t) &LPC_DAC->DACR;
    dac_lli.next = (uint32_t) &dac_lli;
    dac_lli.control = DMA_CTRL_TRANSFER_SIZE(DAC_TABLE_SAMPLES)
                    | DMA_CTRL_SRC_WIDTH_WORD
                    | DMA_CTRL_DST_WIDTH_WORD
                    | DMA_CTRL_SRC_INCREMENT;

    LPC_GPDMACH1->DMACCConfig = 0u;
    LPC_GPDMACH1->DMACCSrcAddr = dac_lli.source;
    LPC_GPDMACH1->DMACCDestAddr = dac_lli.destination;
    LPC_GPDMACH1->DMACCLLI = dac_lli.next;
    LPC_GPDMACH1->DMACCControl = dac_lli.control;
    LPC_GPDMACH1->DMACCConfig = DMA_CFG_DST_PERIPHERAL(DMA_CONN_DAC)
                              | DMA_CFG_M2P
                              | DMA_CFG_ERROR_INTERRUPT
                              | DMA_CFG_ENABLE;

    LPC_DAC->DACR = dac_sine_table[0];       /* BIAS = 0: modo de 1 MS/s */
    /* El contador recorre VALUE...0: son VALUE + 1 clocks. Con 24 se obtiene
     * 25 MHz / (24 + 1) = 1 MS/s. */
    LPC_DAC->DACCNTVAL = 24u;
    LPC_DAC->DACCTRL = (1u << 2) | (1u << 3); /* contador y request DMA */
}

static void adc_init(void)
{
    LPC_SC->PCONP |= (1u << 12);

    /* PCLK_ADC = CCLK/4 = 25 MHz. CLKDIV=1 deja el ADC en 12,5 MHz. */
    LPC_SC->PCLKSEL0 &= ~(0x3u << 24);

    LPC_PINCON->PINSEL1 &= ~(0x3u << 14);
    LPC_PINCON->PINSEL1 |=  (0x1u << 14);     /* P0.23 = AD0.0 */
    LPC_PINCON->PINMODE1 &= ~(0x3u << 14);
    LPC_PINCON->PINMODE1 |=  (0x2u << 14);   /* sin pull-up ni pull-down */

    LPC_ADC->ADCR = (1u << 0)                /* seleccionar AD0.0 */
                  | (1u << 8)                /* CLKDIV = 1 */
                  | (1u << 21);              /* ADC encendido */

    /* ADGINTEN se habilita al armar cada captura. La IRQ del ADC queda
     * apagada porque la request la consume GPDMA. */
    LPC_ADC->ADINTEN = 0u;
    NVIC_DisableIRQ(ADC_IRQn);
}

static void adc_timer_init(void)
{
    LPC_SC->PCONP |= (1u << 1);               /* Timer 0 */
    LPC_SC->PCLKSEL0 &= ~(0x3u << 2);         /* PCLK = 25 MHz */

    LPC_TIM0->TCR = 2u;                       /* reset */
    LPC_TIM0->PR = 0u;
    LPC_TIM0->MR1 = 65u;
    LPC_TIM0->MCR = (1u << 4);                /* reset de TC al coincidir MR1 */

    /* MAT0.1 alterna en cada match. El ADC usa solamente los flancos
     * ascendentes: 25 MHz / (65 + 1) / 2 = 189393,94 muestras/s. */
    LPC_TIM0->EMR = (3u << 6);
    LPC_TIM0->TCR = 0u;
}

static void adc_trigger_start(void)
{
    LPC_TIM0->TCR = 2u;
    LPC_TIM0->EMR = (3u << 6);                /* MAT0.1 arranca en cero */
    LPC_ADC->ADCR &= ~ADC_CR_START_MASK;
    LPC_ADC->ADCR |= ADC_CR_START_MAT01;      /* flanco ascendente */
    LPC_TIM0->TCR = 1u;
}

#if SCOPE_MODE == SCOPE_MODE_BLOCK_12BIT
static void adc_trigger_stop(void)
{
    LPC_TIM0->TCR = 0u;
    LPC_ADC->ADCR &= ~ADC_CR_START_MASK;
    LPC_ADC->ADINTEN = 0u;
    (void) LPC_ADC->ADGDR;                   /* limpiar DONE/OVERRUN residuales */
}
#endif

static uint32_t adc_dma_control(uint32_t samples)
{
    /* Burst de origen 1 y burst de destino 1. El driver viejo de NXP usa
     * burst 4 para ADC, pero el manual indica 1 cuando hay un solo canal. */
    return DMA_CTRL_TRANSFER_SIZE(samples)
         | DMA_CTRL_SRC_WIDTH_WORD
         | DMA_CTRL_DST_WIDTH_WORD
         | DMA_CTRL_DST_INCREMENT
         | DMA_CTRL_TC_INTERRUPT;
}

static uint32_t build_frame(const uint32_t *raw, uint32_t raw_samples,
                            uint32_t decimation, uint8_t sample_bits,
                            uint32_t sequence, uint8_t initial_flags)
{
    uint32_t output_samples = raw_samples / decimation;
    uint32_t payload_length = output_samples * ((sample_bits == 12u) ? 2u : 1u);
    uint8_t flags = initial_flags;
    uint8_t *payload = &tx_frame[FRAME_HEADER_SIZE];

    for (uint32_t out = 0u; out < output_samples; out++) {
        uint32_t word = raw[out * decimation];
        uint16_t sample = (uint16_t) ADC_GDR_RESULT(word);

        if ((word & ADC_GDR_OVERRUN) != 0u) {
            flags |= FRAME_FLAG_ADC_OVERRUN;
        }

        if (sample_bits == 12u) {
            put_u16_le(&payload[out * 2u], sample);
        } else {
            payload[out] = (uint8_t) (sample >> 4);
        }
    }

    if (dma_error_count != 0u) {
        flags |= FRAME_FLAG_DMA_ERROR;
    }

    tx_frame[0] = FRAME_MAGIC_0;
    tx_frame[1] = FRAME_MAGIC_1;
    tx_frame[2] = FRAME_MAGIC_2;
    tx_frame[3] = FRAME_MAGIC_3;
    tx_frame[4] = FRAME_VERSION;
    tx_frame[5] = flags;
    put_u16_le(&tx_frame[6], FRAME_HEADER_SIZE);
    put_u32_le(&tx_frame[8], sequence);
    put_u32_le(&tx_frame[12], ADC_CAPTURE_RATE_HZ);
    put_u32_le(&tx_frame[16], ADC_CAPTURE_RATE_HZ / decimation);
    put_u32_le(&tx_frame[20], DAC_SIGNAL_RATE_HZ);
    put_u16_le(&tx_frame[24], (uint16_t) output_samples);
    tx_frame[26] = sample_bits;
    tx_frame[27] = 0u;                     /* AD0.0 */
    put_u16_le(&tx_frame[28], (uint16_t) payload_length);

    uint16_t crc = crc16_ccitt_update(0xFFFFu, tx_frame, 30u);
    crc = crc16_ccitt_update(crc, payload, payload_length);
    put_u16_le(&tx_frame[30], crc);

    return FRAME_HEADER_SIZE + payload_length;
}

void DMA_IRQHandler(void)
{
    uint32_t terminal = LPC_GPDMA->DMACIntTCStat;
    uint32_t errors = LPC_GPDMA->DMACIntErrStat;

    if ((terminal & (1u << DMA_CHANNEL_ADC)) != 0u) {
        LPC_GPDMA->DMACIntTCClear = (1u << DMA_CHANNEL_ADC);

#if SCOPE_MODE == SCOPE_MODE_BLOCK_12BIT
        adc_block_done = 1u;
#else
        uint32_t completed = stream_next_completed;
        stream_next_completed ^= 1u;
        stream_completed_blocks++;

        if (stream_ready != 0u) {
            stream_lost_blocks++;
        }
        stream_ready_index = completed;
        stream_ready_sequence = stream_completed_blocks;
        stream_ready = 1u;
#endif
    }

    if (errors != 0u) {
        LPC_GPDMA->DMACIntErrClr = errors;
        dma_error_count++;
    }
}

#if SCOPE_MODE == SCOPE_MODE_BLOCK_12BIT

static void adc_capture_block(void)
{
    adc_trigger_stop();
    LPC_GPDMACH0->DMACCConfig = 0u;
    LPC_GPDMA->DMACIntTCClear = (1u << DMA_CHANNEL_ADC);
    LPC_GPDMA->DMACIntErrClr = (1u << DMA_CHANNEL_ADC);

    LPC_GPDMACH0->DMACCSrcAddr = (uint32_t) &LPC_ADC->ADGDR;
    LPC_GPDMACH0->DMACCDestAddr = (uint32_t) adc_buffer_a;
    LPC_GPDMACH0->DMACCLLI = 0u;
    /* La primera request después de rearmar el ADC puede traer el OVERRUN
     * residual de la transición anterior. Se captura una muestra extra y se
     * descarta al construir la trama. */
    LPC_GPDMACH0->DMACCControl = adc_dma_control(BLOCK_DMA_SAMPLES);

    adc_block_done = 0u;
    LPC_GPDMACH0->DMACCConfig = DMA_CFG_SRC_PERIPHERAL(DMA_CONN_ADC)
                              | DMA_CFG_P2M
                              | DMA_CFG_ERROR_INTERRUPT
                              | DMA_CFG_TC_INTERRUPT
                              | DMA_CFG_ENABLE;
    __DSB();
    LPC_ADC->ADINTEN = (1u << 8);
    adc_trigger_start();

    while (adc_block_done == 0u) {
        __WFI();
    }
    adc_trigger_stop();
}

#else

static void adc_stream_start(void)
{
    uint32_t control = adc_dma_control(STREAM_RAW_SAMPLES);

    adc_lli_a.source = (uint32_t) &LPC_ADC->ADGDR;
    adc_lli_a.destination = (uint32_t) adc_buffer_a;
    adc_lli_a.next = (uint32_t) &adc_lli_b;
    adc_lli_a.control = control;

    adc_lli_b.source = (uint32_t) &LPC_ADC->ADGDR;
    adc_lli_b.destination = (uint32_t) adc_buffer_b;
    adc_lli_b.next = (uint32_t) &adc_lli_a;
    adc_lli_b.control = control;

    stream_ready = 0u;
    stream_next_completed = 0u;
    stream_completed_blocks = 0u;
    stream_lost_blocks = 0u;

    LPC_GPDMACH0->DMACCConfig = 0u;
    LPC_GPDMA->DMACIntTCClear = (1u << DMA_CHANNEL_ADC);
    LPC_GPDMA->DMACIntErrClr = (1u << DMA_CHANNEL_ADC);
    LPC_GPDMACH0->DMACCSrcAddr = adc_lli_a.source;
    LPC_GPDMACH0->DMACCDestAddr = adc_lli_a.destination;
    LPC_GPDMACH0->DMACCLLI = adc_lli_a.next;
    LPC_GPDMACH0->DMACCControl = adc_lli_a.control;
    LPC_GPDMACH0->DMACCConfig = DMA_CFG_SRC_PERIPHERAL(DMA_CONN_ADC)
                              | DMA_CFG_P2M
                              | DMA_CFG_ERROR_INTERRUPT
                              | DMA_CFG_TC_INTERRUPT
                              | DMA_CFG_ENABLE;
    __DSB();
    LPC_ADC->ADINTEN = (1u << 8);
    adc_trigger_start();
}

static void stream_take_ready(uint32_t *index, uint32_t *sequence,
                              uint32_t *lost)
{
    for (;;) {
        __disable_irq();
        if (stream_ready != 0u) {
            *index = stream_ready_index;
            *sequence = stream_ready_sequence;
            *lost = stream_lost_blocks;
            stream_ready = 0u;
            stream_lost_blocks = 0u;
            __enable_irq();
            return;
        }

        /* Con PRIMASK activo, una IRQ pendiente despierta WFI pero no ejecuta
         * el handler hasta volver a habilitar interrupciones. Así no queda una
         * ventana entre comprobar la bandera y dormir. */
        __DSB();
        __WFI();
        __enable_irq();
    }
}

#endif

int main(void)
{
    /* Las frecuencias configuradas arriba dependen de SystemInit() y del core
     * a 100 MHz que usa la plantilla del curso. */
    if (SystemCoreClock != CPU_CLOCK_HZ) {
        while (1) {
        }
    }

    uart0_init();
    gpdma_init();
    adc_init();
    adc_timer_init();
    dac_start();

    /* Dejar que el DAC complete varios periodos antes de la primera captura. */
    for (volatile uint32_t wait = 0u; wait < 100000u; wait++) {
        __NOP();
    }

#if SCOPE_MODE == SCOPE_MODE_BLOCK_12BIT
    uint32_t sequence = 0u;

    while (1) {
        adc_capture_block();
        sequence++;
        uint32_t frame_length = build_frame(&adc_buffer_a[1], BLOCK_RAW_SAMPLES,
                                            1u, 12u, sequence,
                                            FRAME_FLAG_CAPTURE_GAPS);
        uart0_write(tx_frame, frame_length);
    }
#else
    adc_stream_start();

    while (1) {
        uint32_t index;
        uint32_t sequence;
        uint32_t lost;
        stream_take_ready(&index, &sequence, &lost);

        /* El primer resultado puede conservar el OVERRUN del arranque. Desde
         * el segundo bloque el flujo Timer -> ADC -> DMA ya está estable. */
        if (sequence == 1u) {
            continue;
        }

        const uint32_t *raw = (index == 0u) ? adc_buffer_a : adc_buffer_b;
        uint8_t flags = FRAME_FLAG_DECIMATED;
        if (lost != 0u) {
            flags |= FRAME_FLAG_CAPTURE_LOSS;
        }

        uint32_t frame_length = build_frame(raw, STREAM_RAW_SAMPLES,
                                            STREAM_DECIMATION, 8u,
                                            sequence, flags);
        uart0_write(tx_frame, frame_length);
    }
#endif
}
