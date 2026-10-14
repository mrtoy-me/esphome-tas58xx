#pragma once

#include "esphome/core/hal.h"

namespace esphome::tas58xx_helpers {

// 5 coefficients in 5.27 fixed point, each converted to the byte order written to the DSP (20 bytes)
struct BiquadCoefficients {
    int32_t b0;
    int32_t b1;
    int32_t b2;
    int32_t a1;
    int32_t a2;
};

static_assert(sizeof(BiquadCoefficients) == 20, "a biquad is written to the DSP as 20 bytes");

enum FilterType : uint8_t {
  FILTER_TYPE_ALL_PASS,    // flat, the DSP power on state
  FILTER_TYPE_EQUALIZER,   // as the TI PurePath Console 3 "Equalizer" filter, used by the EQ gain numbers
  FILTER_TYPE_PEAKING,
  FILTER_TYPE_LOW_SHELF,
  FILTER_TYPE_HIGH_SHELF,
  FILTER_TYPE_LOW_PASS,    // 2nd order Butterworth, Q is fixed
  FILTER_TYPE_HIGH_PASS,   // 2nd order Butterworth, Q is fixed
};

int32_t gain_to_f9_23_(int8_t gain);

// gain is in dB and is not used by all pass, q_factor is not used by all pass, low pass and high pass
BiquadCoefficients make_biquad(FilterType type, uint32_t sample_rate, uint16_t frequency, int8_t gain, float q_factor);

BiquadCoefficients all_pass_();

BiquadCoefficients equalizer_qfactor_(uint32_t sample_rate, uint16_t frequency, int8_t gain, float q_factor);

BiquadCoefficients low_shelf_filter_(uint32_t sample_rate, uint16_t frequency, int8_t gain, float q_factor);

BiquadCoefficients high_shelf_filter_(uint32_t sample_rate, uint16_t frequency, int8_t gain, float q_factor);

BiquadCoefficients low_pass_filter_(uint32_t sample_rate, uint16_t frequency, int8_t gain);

BiquadCoefficients high_pass_filter_(uint32_t sample_rate, uint16_t frequency, int8_t gain);

BiquadCoefficients peaking_eq_(uint32_t sample_rate, uint16_t frequency, int8_t gain, float q_factor);

}  // namespace esphome::tas58xx_helpers
