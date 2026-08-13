#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#include "wavegen.h"

#define PROTOCOL_COMMAND_SET          1u
#define PROTOCOL_COMMAND_GET          2u

#define PROTOCOL_STATUS_OK            0u
#define PROTOCOL_STATUS_BAD_CRC       1u
#define PROTOCOL_STATUS_BAD_VERSION   2u
#define PROTOCOL_STATUS_BAD_COMMAND   3u
#define PROTOCOL_STATUS_BAD_WAVEFORM  10u
#define PROTOCOL_STATUS_BAD_FREQUENCY 11u
#define PROTOCOL_STATUS_BAD_LEVEL     12u
#define PROTOCOL_STATUS_NO_TIMING     13u

#define PROTOCOL_FLAG_DMA_ERROR       (1u << 0)
#define PROTOCOL_FLAG_UART_RX_DROPPED (1u << 1)

typedef struct {
    uint8_t command;
    WavegenConfig config;
    uint32_t sequence;
} ProtocolCommand;

typedef enum {
    PROTOCOL_NO_COMMAND = 0,
    PROTOCOL_VALID_COMMAND,
    PROTOCOL_INVALID_COMMAND
} ProtocolPollResult;

/* Busca comandos completos dentro del flujo de UART. Ante un CRC incorrecto
 * conserva la secuencia para que la PC pueda relacionar el error. */
ProtocolPollResult protocol_poll(ProtocolCommand *command,
                                 uint8_t *error_status);

void protocol_send_status(uint8_t status, const ProtocolCommand *command,
                          const WavegenConfig *active_config,
                          const WavegenPlan *active_plan,
                          uint32_t generation, uint16_t flags);

/* raw contiene palabras ADGDR. La primera ya debe haberse descartado. */
void protocol_send_scope(const uint32_t *raw, uint16_t sample_count,
                         uint32_t sequence, uint32_t signal_rate_hz,
                         uint8_t initial_flags);

uint8_t protocol_status_from_wavegen(WavegenResult result);

#endif
