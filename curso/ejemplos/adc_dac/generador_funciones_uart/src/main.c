/*
 * Generador de funciones didáctico para LPCXpresso LPC1769.
 *
 * La CPU calcula un período solamente cuando cambia la configuración. Después
 * GPDMA recorre esa tabla en forma circular y alimenta al DAC sin intervención
 * por muestra. Una captura ADC opcional permite verificar AOUT por loopback.
 *
 * Conexiones:
 *   P0.26/AOUT -> P0.23/AD0.0
 *   P0.2/TXD0  -> RXD del CP2102
 *   P0.3/RXD0  <- TXD del CP2102
 *   GND        -> GND del CP2102
 */

#include <stdint.h>

#include "LPC17xx.h"

#include "hardware.h"
#include "protocol.h"
#include "wavegen.h"

/* Se usan dos tablas para calcular la próxima onda sin tocar la que todavía
 * está leyendo GPDMA. El cambio efectivo dura solamente el rearme del canal. */
static uint32_t generator_tables[2][WAVEGEN_TABLE_MAX]
    __attribute__((section(".ahbram1"), aligned(4)));

/* GPDMA no puede escribir en la SRAM principal del LPC1769. */
static uint32_t monitor_raw[HW_MONITOR_DMA_SAMPLES]
    __attribute__((section(".ahbram0"), aligned(4)));

static uint16_t diagnostic_flags(void)
{
    uint16_t flags = 0u;
    if (hardware_dma_errors() != 0u) {
        flags |= PROTOCOL_FLAG_DMA_ERROR;
    }
    if (hardware_uart_rx_dropped() != 0u) {
        flags |= PROTOCOL_FLAG_UART_RX_DROPPED;
    }
    return flags;
}

int main(void)
{
    WavegenConfig active_config = {
        .waveform = WAVE_SINE,
        .frequency_hz = 20000u,
        .vpp_mv = 2800u,
        .offset_mv = 1650u,
        .monitor_enabled = 1u
    };
    WavegenPlan active_plan;
    uint32_t active_table = 0u;
    uint32_t generation = 1u;
    uint32_t scope_sequence = 0u;

    hardware_init();
    if (wavegen_build(&active_config, generator_tables[active_table],
                      &active_plan) != WAVEGEN_OK) {
        while (1) {
        }
    }
    hardware_generator_apply(generator_tables[active_table],
                             active_plan.sample_count,
                             active_plan.counter_value);

    while (1) {
        ProtocolCommand command;
        uint8_t parse_error = PROTOCOL_STATUS_OK;
        ProtocolPollResult poll;

        /* Vaciar todos los comandos pendientes antes de iniciar otra captura.
         * Así la respuesta de control no queda detrás de una trama nueva. */
        do {
            poll = protocol_poll(&command, &parse_error);
            if (poll == PROTOCOL_INVALID_COMMAND) {
                protocol_send_status(parse_error, &command, &active_config,
                                     &active_plan, generation,
                                     diagnostic_flags());
            } else if (poll == PROTOCOL_VALID_COMMAND) {
                uint8_t status = PROTOCOL_STATUS_OK;

                if (command.command == PROTOCOL_COMMAND_SET) {
                    uint32_t next_table = active_table ^ 1u;
                    WavegenPlan next_plan;
                    WavegenResult result = wavegen_build(
                        &command.config, generator_tables[next_table],
                        &next_plan);
                    status = protocol_status_from_wavegen(result);

                    if (result == WAVEGEN_OK) {
                        hardware_generator_apply(generator_tables[next_table],
                                                 next_plan.sample_count,
                                                 next_plan.counter_value);
                        active_table = next_table;
                        active_config = command.config;
                        active_plan = next_plan;
                        generation++;
                    }
                }

                protocol_send_status(status, &command, &active_config,
                                     &active_plan, generation,
                                     diagnostic_flags());
            }
        } while (poll != PROTOCOL_NO_COMMAND);

        if (active_config.monitor_enabled != 0u) {
            hardware_monitor_capture(monitor_raw);
            scope_sequence++;
            uint8_t scope_flags = hardware_dma_errors() != 0u ? (1u << 2) : 0u;
            uint32_t signal_rate_hz =
                (active_plan.actual_frequency_millihz + 500u) / 1000u;
            protocol_send_scope(&monitor_raw[1], HW_MONITOR_SAMPLES,
                                scope_sequence, signal_rate_hz, scope_flags);
        } else {
            __WFI();
        }
    }
}
