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

enum class Butterworth2Type {
    HIGHPASS = 0,
    LOWPASS = 1,
};

int32_t gain_to_f9_23_(int8_t gain);

BiquadCoefficients butterworth2_(float fs, float fc, Butterworth2Type type);

}  // namespace esphome::tas58xx_helpers