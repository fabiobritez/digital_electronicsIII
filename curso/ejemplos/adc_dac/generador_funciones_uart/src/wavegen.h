#ifndef WAVEGEN_H
#define WAVEGEN_H

#include <stdint.h>

#define WAVEGEN_TABLE_MAX          512u
#define WAVEGEN_VREF_MV            3300u
#define WAVEGEN_DAC_MAX_CODE       1023u
#define WAVEGEN_DAC_MAX_MV         3297u
#define WAVEGEN_PCLK_DAC_HZ        25000000u
#define WAVEGEN_MIN_DIVIDER        25u
#define WAVEGEN_MAX_DIVIDER        65536u
#define WAVEGEN_MIN_FREQUENCY_HZ   1u

typedef enum {
    WAVE_SINE = 0,
    WAVE_SQUARE = 1,
    WAVE_TRIANGLE = 2,
    WAVE_SAW_UP = 3,
    WAVE_SAW_DOWN = 4,
    WAVE_COUNT
} Waveform;

typedef struct {
    Waveform waveform;
    uint32_t frequency_hz;
    uint16_t vpp_mv;
    uint16_t offset_mv;
    uint8_t monitor_enabled;
} WavegenConfig;

typedef struct {
    uint16_t sample_count;
    uint16_t counter_value;
    uint32_t divider;
    uint32_t actual_frequency_millihz;
    uint32_t frequency_error_ppm;
    uint16_t low_code;
    uint16_t high_code;
} WavegenPlan;

typedef enum {
    WAVEGEN_OK = 0,
    WAVEGEN_BAD_WAVEFORM = 1,
    WAVEGEN_BAD_FREQUENCY = 2,
    WAVEGEN_BAD_LEVEL = 3,
    WAVEGEN_NO_TIMING = 4
} WavegenResult;

WavegenResult wavegen_build(const WavegenConfig *config,
                            uint32_t table[WAVEGEN_TABLE_MAX],
                            WavegenPlan *plan);

const char *wavegen_name(Waveform waveform);

#endif

