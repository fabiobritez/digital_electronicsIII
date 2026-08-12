#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>

#define HW_UART_BAUD                921600u
#define HW_ADC_CAPTURE_RATE_HZ      189394u
#define HW_MONITOR_SAMPLES          512u
#define HW_MONITOR_DMA_SAMPLES      (HW_MONITOR_SAMPLES + 1u)

void hardware_init(void);

void hardware_generator_apply(const uint32_t *table, uint16_t sample_count,
                              uint16_t dac_counter_value);

void hardware_monitor_capture(uint32_t destination[HW_MONITOR_DMA_SAMPLES]);

void hardware_uart_write(const uint8_t *data, uint32_t length);
int hardware_uart_read_byte(uint8_t *value);

uint32_t hardware_dma_errors(void);
uint32_t hardware_uart_rx_dropped(void);

#endif

