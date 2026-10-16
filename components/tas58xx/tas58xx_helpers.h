#pragma once

#include <cstdint>

namespace esphome::tas58xx {

// 5 coefficients in 5.27 fixed point, each converted to the byte order written to the DSP (20 bytes)
struct BiquadCoefficients {
  int32_t b0;
  int32_t b1;
  int32_t b2;
  int32_t a1;
  int32_t a2;
};

static_assert(sizeof(BiquadCoefficients) == 20, "a biquad is written to the DSP as 20 bytes");

// b0 = 1.0 and all other coefficients 0.0, which is also the DSP power on state of every biquad
static constexpr BiquadCoefficients ALL_PASS = {0x00000008, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

enum FilterType : uint8_t {
  FILTER_TYPE_ALL_PASS,   // flat, the DSP power on state
  FILTER_TYPE_EQUALIZER,  // as the TI PurePath Console 3 "Equalizer" filter, used by the EQ gain numbers
  FILTER_TYPE_PEAKING,
  FILTER_TYPE_LOW_SHELF,
  FILTER_TYPE_HIGH_SHELF,
  FILTER_TYPE_LOW_PASS,   // 2nd order Butterworth, Q is fixed
  FILTER_TYPE_HIGH_PASS,  // 2nd order Butterworth, Q is fixed
  FILTER_TYPE_NOTCH,      // no gain, bandwidth = frequency / Q
  FILTER_TYPE_BAND_PASS,  // no gain, bandwidth = frequency / Q
};

// gain in dB as 9.23 fixed point in the byte order written to the DSP
int32_t gain_to_fixed_9_23(int8_t gain);

// gain is in dB and is not used by all pass, notch and band pass
// q_factor is not used by all pass, low pass and high pass; for notch and band pass it sets bandwidth = frequency / Q
BiquadCoefficients make_biquad(FilterType type, uint32_t sample_rate, uint16_t frequency, int8_t gain, double q_factor);

}  // namespace esphome::tas58xx
