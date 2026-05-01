#pragma once

#include "esphome/core/hal.h"

namespace esphome::tas58xx_helpers {

struct BiquadCoefficients {
    int32_t b0;
    int32_t b1;
    int32_t b2;
    int32_t a1;
    int32_t a2;
};

int32_t gain_to_f9_23_(int8_t gain);

BiquadCoefficients equalizer_qfactor_(uint32_t sample_rate, uint16_t frequency, int16_t gain, float q_factor);

BiquadCoefficients low_shelf_filter_(uint32_t sample_rate, uint16_t frequency, int16_t gain, float q_factor);

BiquadCoefficients high_shelf_filter_(uint32_t sample_rate, uint16_t frequency, int16_t gain, float q_factor);

BiquadCoefficients low_pass_filter_(uint32_t sample_rate, uint16_t frequency, int16_t gain);

BiquadCoefficients high_pass_filter_(uint32_t sample_rate, uint16_t frequency, int16_t gain);

}  // namespace esphome::tas58xx_helpers