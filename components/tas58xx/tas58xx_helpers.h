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

enum Butterworth2Type {
    HIGHPASS = 0,
    LOWPASS = 1,
};

int32_t gain_to_f9_23_(int8_t gain);

BiquadCoefficients butterworth2_(uint16_t fs, uint16_t fc, Butterworth2Type type);

BiquadCoefficients equalizer_bandwidth_calc(int16_t gain, uint16_t bandwidth, uint32_t sample_rate, uint16_t frequency);

BiquadCoefficients equalizer_qfactor_calc(int16_t gain, uint16_t bandwidth, uint32_t sample_rate, uint16_t frequency, float qFactor);

}  // namespace esphome::tas58xx_helpers