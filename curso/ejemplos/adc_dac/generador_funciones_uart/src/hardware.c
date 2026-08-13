#include "hardware.h"

#include "LPC17xx.h"

#define CPU_CLOCK_HZ                 100000000u

#define DMA_CHANNEL_ADC              0u
#define DMA_CHANNEL_DAC              1u
#define DMA_CHANNEL_UART_TX          2u
#define DMA_CONN_ADC                 4u
#define DMA_CONN_DAC                 7u
#define DMA_CONN_UART0_TX            8u

#define DMA_CTRL_TRANSFER_SIZE(n)    ((uint32_t) (n) & 0x0FFFu)
#define DMA_CTRL_SRC_WIDTH_WORD      (2u << 18)
#define DMA_CTRL_DST_WIDTH_WORD      (2u << 21)
#define DMA_CTRL_SRC_INCREMENT       (1u << 26)
#define DMA_CTRL_DST_INCREMENT       (1u << 27)
#define DMA_CTRL_TC_INTERRUPT        (1u << 31)

#define DMA_CFG_ENABLE               (1u << 0)
#define DMA_CFG_SRC_PERIPHERAL(n)    ((uint32_t) (n) << 1)
#define DMA_CFG_DST_PERIPHERAL(n)    ((uint32_t) (n) << 6)
#define DMA_CFG_M2P                  (1u << 11)
#define DMA_CFG_P2M                  (2u << 11)
#define DMA_CFG_ERROR_INTERRUPT      (1u << 14)
#define DMA_CFG_TC_INTERRUPT         (1u << 15)

#define ADC_CR_START_MASK            (0x7u << 24)
#define ADC_CR_START_MAT01           (0x4u << 24)

#define UART_LSR_RDR                 (1u << 0)
#define UART_RX_BUFFER_SIZE          256u
#define UART_RX_BUFFER_MASK          (UART_RX_BUFFER_SIZE - 1u)

typedef struct {
    uint32_t source;
    uint32_t destination;
    uint32_t next;
    uint32_t control;
} DmaLli;

/* GPDMA no puede leer la SRAM principal. El descriptor del DAC vive en el
 * mismo banco AHB que las tablas generadas. */
static DmaLli dac_lli
    __attribute__((section(".ahbram1"), aligned(4)));

static volatile uint32_t adc_done;
static volatile uint32_t uart_tx_done;
static volatile uint32_t dma_errors;

static uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint32_t uart_rx_head;
static volatile uint32_t uart_rx_tail;
static volatile uint32_t uart_rx_dropped;

static void wait_for_interrupt_flag(volatile uint32_t *flag)
{
    for (;;) {
        uint32_t primask = __get_PRIMASK();
        __disable_irq();
        if (*flag != 0u) {
            __set_PRIMASK(primask);
            return;
        }

        /* Una IRQ que queda pendiente entre la prueba y WFI hace que WFI
         * vuelva inmediatamente. Al restaurar PRIMASK corre el handler. */
        __DSB();
        __WFI();
        __set_PRIMASK(primask);
    }
}

static void uart_push_received(uint8_t value)
{
    uint32_t next = (uart_rx_head + 1u) & UART_RX_BUFFER_MASK;
    if (next == uart_rx_tail) {
        uart_rx_dropped++;
        return;
    }
    uart_rx_buffer[uart_rx_head] = value;
    uart_rx_head = next;
}

static void uart0_init(void)
{
    LPC_SC->PCONP |= (1u << 3);
    LPC_SC->PCLKSEL0 &= ~(0x3u << 6);
    LPC_SC->PCLKSEL0 |=  (0x1u << 6);        /* PCLK_UART0 = 100 MHz */

    LPC_PINCON->PINSEL0 &= ~((0x3u << 4) | (0x3u << 6));
    LPC_PINCON->PINSEL0 |=  ((0x1u << 4) | (0x1u << 6));
                                                  /* P0.2 TXD0, P0.3 RXD0 */

    LPC_UART0->LCR = 0x03u | (1u << 7);
    LPC_UART0->DLM = 0u;
    LPC_UART0->DLL = 5u;
    LPC_UART0->FDR = (14u << 4) | 5u;       /* 921052,6 baud */
    LPC_UART0->LCR = 0x03u;
    LPC_UART0->FCR = 0x07u;
    LPC_UART0->TER = (1u << 7);
    LPC_UART0->IER = (1u << 0) | (1u << 2); /* dato recibido y error de línea */

    NVIC_SetPriority(UART0_IRQn, 2u);
    NVIC_ClearPendingIRQ(UART0_IRQn);
    NVIC_EnableIRQ(UART0_IRQn);
}

static void gpdma_init(void)
{
    LPC_SC->PCONP |= (1u << 29);
    __DSB();

    LPC_GPDMACH0->DMACCConfig = 0u;
    LPC_GPDMACH1->DMACCConfig = 0u;
    LPC_GPDMACH2->DMACCConfig = 0u;
    /* La request 8 también puede representar MAT0.0. Con cero se selecciona
     * UART0 TX, que es el destino usado por el canal 2. */
    LPC_SC->DMAREQSEL &= ~(1u << 0);
    LPC_GPDMA->DMACIntTCClear = 0xFFu;
    LPC_GPDMA->DMACIntErrClr = 0xFFu;
    LPC_GPDMA->DMACConfig = 1u;
    while ((LPC_GPDMA->DMACConfig & 1u) == 0u) {
    }

    NVIC_SetPriority(DMA_IRQn, 1u);
    NVIC_ClearPendingIRQ(DMA_IRQn);
    NVIC_EnableIRQ(DMA_IRQn);
}

static void dac_init(void)
{
    LPC_SC->PCLKSEL0 &= ~(0x3u << 22);       /* PCLK_DAC = 25 MHz */
    LPC_PINCON->PINSEL1 &= ~(0x3u << 20);
    LPC_PINCON->PINSEL1 |=  (0x2u << 20);    /* P0.26 = AOUT */
    LPC_PINCON->PINMODE1 &= ~(0x3u << 20);
    LPC_PINCON->PINMODE1 |=  (0x2u << 20);
    LPC_DAC->DACR = 512u << 6;               /* 1,65 V hasta aplicar la tabla */
    LPC_DAC->DACCTRL = 0u;
}

static void adc_init(void)
{
    LPC_SC->PCONP |= (1u << 12);
    LPC_SC->PCLKSEL0 &= ~(0x3u << 24);       /* PCLK_ADC = 25 MHz */

    LPC_PINCON->PINSEL1 &= ~(0x3u << 14);
    LPC_PINCON->PINSEL1 |=  (0x1u << 14);    /* P0.23 = AD0.0 */
    LPC_PINCON->PINMODE1 &= ~(0x3u << 14);
    LPC_PINCON->PINMODE1 |=  (0x2u << 14);

    LPC_ADC->ADCR = (1u << 0) | (1u << 8) | (1u << 21);
    LPC_ADC->ADINTEN = 0u;
    NVIC_DisableIRQ(ADC_IRQn);
}

static void adc_timer_init(void)
{
    LPC_SC->PCONP |= (1u << 1);
    LPC_SC->PCLKSEL0 &= ~(0x3u << 2);        /* PCLK_TIMER0 = 25 MHz */
    LPC_TIM0->TCR = 2u;
    LPC_TIM0->PR = 0u;
    LPC_TIM0->MR1 = 65u;
    LPC_TIM0->MCR = (1u << 4);
    LPC_TIM0->EMR = (3u << 6);               /* MAT0.1 alterna */
    LPC_TIM0->TCR = 0u;
}

static void adc_trigger_start(void)
{
    LPC_TIM0->TCR = 2u;
    LPC_TIM0->EMR = (3u << 6);
    LPC_ADC->ADCR &= ~ADC_CR_START_MASK;
    LPC_ADC->ADCR |= ADC_CR_START_MAT01;
    LPC_TIM0->TCR = 1u;
}

static void adc_trigger_stop(void)
{
    LPC_TIM0->TCR = 0u;
    LPC_ADC->ADCR &= ~ADC_CR_START_MASK;
    LPC_ADC->ADINTEN = 0u;
    (void) LPC_ADC->ADGDR;
}

void hardware_init(void)
{
    if (SystemCoreClock != CPU_CLOCK_HZ) {
        while (1) {
        }
    }

    uart0_init();
    gpdma_init();
    dac_init();
    adc_init();
    adc_timer_init();
}

void hardware_generator_apply(const uint32_t *table, uint16_t sample_count,
                              uint16_t dac_counter_value)
{
    /* Cada request mueve una sola palabra, por lo que no queda un burst largo
     * en vuelo. Se deshabilita primero el canal y después el request del DAC.
     * HALT no sirve acá: en una lista circular ACTIVE puede permanecer en uno
     * indefinidamente y bloquear un cambio de configuración. */
    LPC_GPDMACH1->DMACCConfig = 0u;
    LPC_DAC->DACCTRL = 0u;
    __DSB();

    dac_lli.source = (uint32_t) table;
    dac_lli.destination = (uint32_t) &LPC_DAC->DACR;
    dac_lli.next = (uint32_t) &dac_lli;
    dac_lli.control = DMA_CTRL_TRANSFER_SIZE(sample_count)
                    | DMA_CTRL_SRC_WIDTH_WORD
                    | DMA_CTRL_DST_WIDTH_WORD
                    | DMA_CTRL_SRC_INCREMENT;

    LPC_GPDMA->DMACIntTCClear = (1u << DMA_CHANNEL_DAC);
    LPC_GPDMA->DMACIntErrClr = (1u << DMA_CHANNEL_DAC);
    LPC_GPDMACH1->DMACCSrcAddr = dac_lli.source;
    LPC_GPDMACH1->DMACCDestAddr = dac_lli.destination;
    LPC_GPDMACH1->DMACCLLI = dac_lli.next;
    LPC_GPDMACH1->DMACCControl = dac_lli.control;

    LPC_DAC->DACR = table[0];
    LPC_DAC->DACCNTVAL = dac_counter_value;
    __DMB();

    LPC_GPDMACH1->DMACCConfig = DMA_CFG_DST_PERIPHERAL(DMA_CONN_DAC)
                              | DMA_CFG_M2P
                              | DMA_CFG_ERROR_INTERRUPT
                              | DMA_CFG_ENABLE;
    LPC_DAC->DACCTRL = (1u << 2) | (1u << 3);
}

void hardware_monitor_capture(uint32_t destination[HW_MONITOR_DMA_SAMPLES])
{
    adc_trigger_stop();
    LPC_GPDMACH0->DMACCConfig = 0u;
    LPC_GPDMA->DMACIntTCClear = (1u << DMA_CHANNEL_ADC);
    LPC_GPDMA->DMACIntErrClr = (1u << DMA_CHANNEL_ADC);

    LPC_GPDMACH0->DMACCSrcAddr = (uint32_t) &LPC_ADC->ADGDR;
    LPC_GPDMACH0->DMACCDestAddr = (uint32_t) destination;
    LPC_GPDMACH0->DMACCLLI = 0u;
    LPC_GPDMACH0->DMACCControl = DMA_CTRL_TRANSFER_SIZE(HW_MONITOR_DMA_SAMPLES)
                               | DMA_CTRL_SRC_WIDTH_WORD
                               | DMA_CTRL_DST_WIDTH_WORD
                               | DMA_CTRL_DST_INCREMENT
                               | DMA_CTRL_TC_INTERRUPT;

    adc_done = 0u;
    LPC_GPDMACH0->DMACCConfig = DMA_CFG_SRC_PERIPHERAL(DMA_CONN_ADC)
                              | DMA_CFG_P2M
                              | DMA_CFG_ERROR_INTERRUPT
                              | DMA_CFG_TC_INTERRUPT
                              | DMA_CFG_ENABLE;
    __DSB();
    LPC_ADC->ADINTEN = (1u << 8);
    adc_trigger_start();

    wait_for_interrupt_flag(&adc_done);
    adc_trigger_stop();
}

void hardware_uart_write(const uint8_t *data, uint32_t length)
{
    /* Los buffers que llegan acá viven en AHB SRAM. GPDMA no puede leer la
     * SRAM principal del LPC1769. La función espera la trama completa, pero
     * WFI deja libre a la CPU durante el tiempo impuesto por UART. */
    LPC_GPDMACH2->DMACCConfig = 0u;
    LPC_GPDMA->DMACIntTCClear = (1u << DMA_CHANNEL_UART_TX);
    LPC_GPDMA->DMACIntErrClr = (1u << DMA_CHANNEL_UART_TX);
    LPC_GPDMACH2->DMACCSrcAddr = (uint32_t) data;
    LPC_GPDMACH2->DMACCDestAddr = (uint32_t) &LPC_UART0->THR;
    LPC_GPDMACH2->DMACCLLI = 0u;
    LPC_GPDMACH2->DMACCControl = DMA_CTRL_TRANSFER_SIZE(length)
                               | DMA_CTRL_SRC_INCREMENT
                               | DMA_CTRL_TC_INTERRUPT;
    uart_tx_done = 0u;
    LPC_GPDMACH2->DMACCConfig = DMA_CFG_DST_PERIPHERAL(DMA_CONN_UART0_TX)
                              | DMA_CFG_M2P
                              | DMA_CFG_ERROR_INTERRUPT
                              | DMA_CFG_TC_INTERRUPT
                              | DMA_CFG_ENABLE;
    __DSB();
    wait_for_interrupt_flag(&uart_tx_done);
}

int hardware_uart_read_byte(uint8_t *value)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    if (uart_rx_head == uart_rx_tail) {
        __set_PRIMASK(primask);
        return 0;
    }

    *value = uart_rx_buffer[uart_rx_tail];
    uart_rx_tail = (uart_rx_tail + 1u) & UART_RX_BUFFER_MASK;
    __set_PRIMASK(primask);
    return 1;
}

uint32_t hardware_dma_errors(void)
{
    return dma_errors;
}

uint32_t hardware_uart_rx_dropped(void)
{
    return uart_rx_dropped;
}

void DMA_IRQHandler(void)
{
    uint32_t terminal = LPC_GPDMA->DMACIntTCStat;
    uint32_t errors = LPC_GPDMA->DMACIntErrStat;

    if ((terminal & (1u << DMA_CHANNEL_ADC)) != 0u) {
        LPC_GPDMA->DMACIntTCClear = (1u << DMA_CHANNEL_ADC);
        adc_done = 1u;
    }
    if ((terminal & (1u << DMA_CHANNEL_DAC)) != 0u) {
        LPC_GPDMA->DMACIntTCClear = (1u << DMA_CHANNEL_DAC);
    }
    if ((terminal & (1u << DMA_CHANNEL_UART_TX)) != 0u) {
        LPC_GPDMA->DMACIntTCClear = (1u << DMA_CHANNEL_UART_TX);
        uart_tx_done = 1u;
    }
    if (errors != 0u) {
        LPC_GPDMA->DMACIntErrClr = errors;
        dma_errors++;
        if ((errors & (1u << DMA_CHANNEL_ADC)) != 0u) {
            adc_done = 1u;
        }
        if ((errors & (1u << DMA_CHANNEL_UART_TX)) != 0u) {
            uart_tx_done = 1u;
        }
    }
}

void UART0_IRQHandler(void)
{
    while ((LPC_UART0->IIR & 1u) == 0u) {
        uint32_t identifier = LPC_UART0->IIR & 0x0Eu;

        if (identifier == 0x04u || identifier == 0x0Cu || identifier == 0x06u) {
            uint32_t status = LPC_UART0->LSR;
            while ((status & UART_LSR_RDR) != 0u) {
                uart_push_received((uint8_t) LPC_UART0->RBR);
                status = LPC_UART0->LSR;
            }
        } else {
            break;
        }
    }
}
