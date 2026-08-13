#include "wavegen.h"

#include <limits.h>

#define ACCEPTABLE_ERROR_PPM 500u
#define SHAPED_MIN_SAMPLES   8u

static uint32_t frequency_error_ppm(uint32_t requested_hz,
                                    uint32_t samples, uint32_t divider)
{
    uint64_t target_product = (uint64_t) requested_hz * samples * divider;
    uint64_t difference = (target_product > WAVEGEN_PCLK_DAC_HZ)
                        ? (target_product - WAVEGEN_PCLK_DAC_HZ)
                        : (WAVEGEN_PCLK_DAC_HZ - target_product);
    return (uint32_t) ((difference * 1000000u + target_product / 2u)
                    / target_product);
}

static uint32_t rounded_divider(uint32_t requested_hz, uint32_t samples)
{
    uint64_t denominator = (uint64_t) requested_hz * samples;
    return (uint32_t) ((WAVEGEN_PCLK_DAC_HZ + denominator / 2u) / denominator);
}

static int valid_sample_count(Waveform waveform, uint32_t samples)
{
    if (waveform == WAVE_SQUARE) {
        return samples >= 2u && (samples & 1u) == 0u;
    }
    return samples >= SHAPED_MIN_SAMPLES;
}

static int prefer_candidate(Waveform waveform, uint32_t samples,
                            uint32_t error_ppm, uint32_t best_samples,
                            uint32_t best_error_ppm)
{
    int acceptable = error_ppm <= ACCEPTABLE_ERROR_PPM;
    int best_acceptable = best_error_ppm <= ACCEPTABLE_ERROR_PPM;

    if (acceptable != best_acceptable) {
        return acceptable;
    }

    if (acceptable) {
        if (waveform == WAVE_SQUARE) {
            return samples < best_samples;
        }
        return samples > best_samples;
    }

    if (error_ppm != best_error_ppm) {
        return error_ppm < best_error_ppm;
    }
    if (waveform == WAVE_SQUARE) {
        return samples < best_samples;
    }
    return samples > best_samples;
}

static int select_timing(const WavegenConfig *config, WavegenPlan *plan)
{
    uint32_t best_samples = 0u;
    uint32_t best_divider = 0u;
    uint32_t best_error = UINT32_MAX;

    for (uint32_t samples = 2u; samples <= WAVEGEN_TABLE_MAX; samples++) {
        if (!valid_sample_count(config->waveform, samples)) {
            continue;
        }

        uint32_t divider = rounded_divider(config->frequency_hz, samples);
        if (divider < WAVEGEN_MIN_DIVIDER || divider > WAVEGEN_MAX_DIVIDER) {
            continue;
        }

        uint32_t error = frequency_error_ppm(config->frequency_hz,
                                             samples, divider);
        if (best_samples == 0u ||
            prefer_candidate(config->waveform, samples, error,
                             best_samples, best_error)) {
            best_samples = samples;
            best_divider = divider;
            best_error = error;
        }
    }

    if (best_samples == 0u) {
        return 0;
    }

    uint64_t denominator = (uint64_t) best_samples * best_divider;
    plan->sample_count = (uint16_t) best_samples;
    plan->divider = best_divider;
    plan->counter_value = (uint16_t) (best_divider - 1u);
    plan->actual_frequency_millihz =
        (uint32_t) (((uint64_t) WAVEGEN_PCLK_DAC_HZ * 1000u + denominator / 2u)
                  / denominator);
    plan->frequency_error_ppm = best_error;
    return 1;
}

static uint16_t millivolts_to_code(uint32_t millivolts)
{
    uint32_t code = (millivolts * 1024u + WAVEGEN_VREF_MV / 2u)
                  / WAVEGEN_VREF_MV;
    return (uint16_t) ((code > WAVEGEN_DAC_MAX_CODE)
                     ? WAVEGEN_DAC_MAX_CODE : code);
}

/* Aproximación de Bhaskara I en punto fijo. Calcula un seno Q15 sin tabla,
 * float, libm ni FPU. El error máximo es pequeño frente a los 10 bits del DAC. */
static int32_t sine_q15(uint32_t index, uint32_t count)
{
    uint32_t phase = (uint32_t) (((uint64_t) index * 65536u) / count);
    int32_t sign = 1;
    uint32_t x;

    if (phase < 32768u) {
        x = phase;
    } else {
        x = phase - 32768u;
        sign = -1;
    }

    uint64_t product = (uint64_t) x * (32768u - x);
    uint64_t numerator = 16u * product;
    uint64_t denominator = (uint64_t) 5u * 32768u * 32768u - 4u * product;
    int32_t magnitude = (int32_t) ((numerator * 32767u + denominator / 2u)
                                / denominator);
    return sign * magnitude;
}

static uint16_t interpolate_code(uint16_t low, uint16_t high,
                                 uint32_t numerator, uint32_t denominator)
{
    uint32_t range = (uint32_t) high - low;
    return (uint16_t) (low + (range * numerator + denominator / 2u)
                     / denominator);
}

static uint16_t interpolate_code_down(uint16_t high, uint16_t low,
                                      uint32_t numerator, uint32_t denominator)
{
    uint32_t range = (uint32_t) high - low;
    return (uint16_t) (high - (range * numerator + denominator / 2u)
                      / denominator);
}

static uint16_t waveform_sample(Waveform waveform, uint32_t index,
                                uint32_t count, uint16_t low, uint16_t high)
{
    uint32_t half = count / 2u;

    switch (waveform) {
    case WAVE_SINE: {
        int32_t center = ((int32_t) low + high) / 2;
        int32_t range = (int32_t) high - low;
        int32_t value = center + (int32_t) (((int64_t) sine_q15(index, count)
                                           * range) / (2 * 32767));
        if (value < low) {
            value = low;
        } else if (value > high) {
            value = high;
        }
        return (uint16_t) value;
    }

    case WAVE_SQUARE:
        return (index < half) ? high : low;

    case WAVE_TRIANGLE:
        if (index < half) {
            return interpolate_code(low, high, index, half);
        }
        return interpolate_code_down(high, low, index - half, count - half);

    case WAVE_SAW_UP:
        return interpolate_code(low, high, index, count - 1u);

    case WAVE_SAW_DOWN:
        return interpolate_code_down(high, low, index, count - 1u);

    default:
        return low;
    }
}

WavegenResult wavegen_build(const WavegenConfig *config,
                            uint32_t table[WAVEGEN_TABLE_MAX],
                            WavegenPlan *plan)
{
    if ((uint32_t) config->waveform >= WAVE_COUNT) {
        return WAVEGEN_BAD_WAVEFORM;
    }

    uint32_t maximum_frequency = (config->waveform == WAVE_SQUARE)
                               ? 250000u : 100000u;
    if (config->frequency_hz < WAVEGEN_MIN_FREQUENCY_HZ ||
        config->frequency_hz > maximum_frequency) {
        return WAVEGEN_BAD_FREQUENCY;
    }

    uint32_t lower_half_mv = config->vpp_mv / 2u;
    uint32_t upper_half_mv = (config->vpp_mv + 1u) / 2u;
    if (config->offset_mv > WAVEGEN_DAC_MAX_MV ||
        config->vpp_mv > 2u * WAVEGEN_DAC_MAX_MV ||
        config->offset_mv < lower_half_mv ||
        upper_half_mv > WAVEGEN_DAC_MAX_MV - config->offset_mv) {
        return WAVEGEN_BAD_LEVEL;
    }

    uint32_t lower_mv = config->offset_mv - lower_half_mv;
    uint32_t upper_mv = config->offset_mv + upper_half_mv;
    plan->low_code = millivolts_to_code(lower_mv);
    plan->high_code = millivolts_to_code(upper_mv);

    if (!select_timing(config, plan)) {
        return WAVEGEN_NO_TIMING;
    }

    for (uint32_t i = 0u; i < plan->sample_count; i++) {
        uint16_t code = waveform_sample(config->waveform, i,
                                        plan->sample_count,
                                        plan->low_code, plan->high_code);
        table[i] = (uint32_t) code << 6;
    }

    return WAVEGEN_OK;
}

const char *wavegen_name(Waveform waveform)
{
    static const char *const names[WAVE_COUNT] = {
        "senoidal", "cuadrada", "triangular", "serrucho ascendente",
        "serrucho descendente"
    };
    return ((uint32_t) waveform < WAVE_COUNT) ? names[waveform] : "desconocida";
}
