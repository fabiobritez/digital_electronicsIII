#include "protocol.h"

#include <string.h>

#include "hardware.h"

#define COMMAND_SIZE          24u
#define STATUS_SIZE           36u
#define SCOPE_HEADER_SIZE     32u
#define SCOPE_PAYLOAD_SIZE    (HW_MONITOR_SAMPLES * 2u)

#define SCOPE_FLAG_GAPS       (1u << 0)
#define SCOPE_FLAG_OVERRUN    (1u << 1)

#define ADC_GDR_RESULT(word)  (((word) >> 4) & 0x0FFFu)
#define ADC_GDR_OVERRUN       (1u << 30)

static const uint8_t command_magic[4] = {'G', 'E', 'N', 'C'};
static uint8_t command_buffer[COMMAND_SIZE];
static uint32_t command_used;

static uint8_t tx_status[STATUS_SIZE]
    __attribute__((section(".ahbram0"), aligned(4)));
static uint8_t tx_scope[SCOPE_HEADER_SIZE + SCOPE_PAYLOAD_SIZE]
    __attribute__((section(".ahbram0"), aligned(4)));

static uint16_t get_u16_le(const uint8_t *source)
{
    return (uint16_t) source[0] | ((uint16_t) source[1] << 8);
}

static uint32_t get_u32_le(const uint8_t *source)
{
    return (uint32_t) source[0]
         | ((uint32_t) source[1] << 8)
         | ((uint32_t) source[2] << 16)
         | ((uint32_t) source[3] << 24);
}

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

static uint16_t crc16_update(uint16_t crc, const uint8_t *data,
                             uint32_t length)
{
    for (uint32_t i = 0u; i < length; i++) {
        crc ^= (uint16_t) data[i] << 8;
        for (uint32_t bit = 0u; bit < 8u; bit++) {
            crc = (crc & 0x8000u) != 0u
                ? (uint16_t) ((crc << 1) ^ 0x1021u)
                : (uint16_t) (crc << 1);
        }
    }
    return crc;
}

static int prefix_matches(void)
{
    uint32_t compared = command_used < sizeof(command_magic)
                      ? command_used : sizeof(command_magic);
    return memcmp(command_buffer, command_magic, compared) == 0;
}

static void append_parser_byte(uint8_t value)
{
    command_buffer[command_used++] = value;

    /* Si los primeros bytes no pueden formar GENC, se descarta uno y se
     * vuelve a probar. Esto permite recuperar sincronismo después de ruido. */
    while (command_used != 0u && !prefix_matches()) {
        memmove(command_buffer, &command_buffer[1], --command_used);
    }
}

static void decode_command(ProtocolCommand *command)
{
    command->command = command_buffer[5];
    command->config.waveform = (Waveform) command_buffer[6];
    command->config.monitor_enabled = command_buffer[7] != 0u;
    command->sequence = get_u32_le(&command_buffer[8]);
    command->config.frequency_hz = get_u32_le(&command_buffer[12]);
    command->config.vpp_mv = get_u16_le(&command_buffer[16]);
    command->config.offset_mv = get_u16_le(&command_buffer[18]);
}

ProtocolPollResult protocol_poll(ProtocolCommand *command,
                                 uint8_t *error_status)
{
    uint8_t value;
    while (hardware_uart_read_byte(&value)) {
        append_parser_byte(value);
        if (command_used != COMMAND_SIZE) {
            continue;
        }

        decode_command(command);
        uint16_t expected_crc = get_u16_le(&command_buffer[22]);
        uint16_t actual_crc = crc16_update(0xFFFFu, command_buffer, 22u);
        uint8_t version = command_buffer[4];
        command_used = 0u;

        if (expected_crc != actual_crc) {
            *error_status = PROTOCOL_STATUS_BAD_CRC;
            return PROTOCOL_INVALID_COMMAND;
        }
        if (version != 1u) {
            *error_status = PROTOCOL_STATUS_BAD_VERSION;
            return PROTOCOL_INVALID_COMMAND;
        }
        if (command->command != PROTOCOL_COMMAND_SET &&
            command->command != PROTOCOL_COMMAND_GET) {
            *error_status = PROTOCOL_STATUS_BAD_COMMAND;
            return PROTOCOL_INVALID_COMMAND;
        }
        return PROTOCOL_VALID_COMMAND;
    }
    return PROTOCOL_NO_COMMAND;
}

void protocol_send_status(uint8_t status, const ProtocolCommand *command,
                          const WavegenConfig *active_config,
                          const WavegenPlan *active_plan,
                          uint32_t generation, uint16_t flags)
{
    memcpy(tx_status, "GENR", 4u);
    tx_status[4] = 1u;
    tx_status[5] = status;
    tx_status[6] = (uint8_t) active_config->waveform;
    tx_status[7] = active_config->monitor_enabled != 0u;
    put_u32_le(&tx_status[8], command->sequence);
    put_u32_le(&tx_status[12], active_config->frequency_hz);
    put_u32_le(&tx_status[16], active_plan->actual_frequency_millihz);
    put_u16_le(&tx_status[20], (uint16_t) active_config->vpp_mv);
    put_u16_le(&tx_status[22], (uint16_t) active_config->offset_mv);
    put_u16_le(&tx_status[24], active_plan->sample_count);
    put_u16_le(&tx_status[26], active_plan->counter_value);
    put_u32_le(&tx_status[28], generation);
    put_u16_le(&tx_status[32], flags);
    put_u16_le(&tx_status[34], crc16_update(0xFFFFu, tx_status, 34u));
    hardware_uart_write(tx_status, sizeof(tx_status));
}

void protocol_send_scope(const uint32_t *raw, uint16_t sample_count,
                         uint32_t sequence, uint32_t signal_rate_hz,
                         uint8_t initial_flags)
{
    uint8_t flags = initial_flags | SCOPE_FLAG_GAPS;
    uint16_t payload_length = (uint16_t) (sample_count * 2u);
    uint8_t *payload = &tx_scope[SCOPE_HEADER_SIZE];

    for (uint32_t i = 0u; i < sample_count; i++) {
        if ((raw[i] & ADC_GDR_OVERRUN) != 0u) {
            flags |= SCOPE_FLAG_OVERRUN;
        }
        put_u16_le(&payload[i * 2u], (uint16_t) ADC_GDR_RESULT(raw[i]));
    }

    memcpy(tx_scope, "LPCS", 4u);
    tx_scope[4] = 1u;
    tx_scope[5] = flags;
    put_u16_le(&tx_scope[6], SCOPE_HEADER_SIZE);
    put_u32_le(&tx_scope[8], sequence);
    put_u32_le(&tx_scope[12], HW_ADC_CAPTURE_RATE_HZ);
    put_u32_le(&tx_scope[16], HW_ADC_CAPTURE_RATE_HZ);
    put_u32_le(&tx_scope[20], signal_rate_hz);
    put_u16_le(&tx_scope[24], sample_count);
    tx_scope[26] = 12u;
    tx_scope[27] = 0u;
    put_u16_le(&tx_scope[28], payload_length);

    uint16_t crc = crc16_update(0xFFFFu, tx_scope, 30u);
    crc = crc16_update(crc, payload, payload_length);
    put_u16_le(&tx_scope[30], crc);
    hardware_uart_write(tx_scope, SCOPE_HEADER_SIZE + payload_length);
}

uint8_t protocol_status_from_wavegen(WavegenResult result)
{
    static const uint8_t statuses[] = {
        PROTOCOL_STATUS_OK,
        PROTOCOL_STATUS_BAD_WAVEFORM,
        PROTOCOL_STATUS_BAD_FREQUENCY,
        PROTOCOL_STATUS_BAD_LEVEL,
        PROTOCOL_STATUS_NO_TIMING
    };
    return (uint32_t) result < sizeof(statuses) ? statuses[result]
                                                : PROTOCOL_STATUS_NO_TIMING;
}
