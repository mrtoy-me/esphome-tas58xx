#include "tas58xx_helpers.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include <cmath>

namespace esphome::tas58xx_helpers {

  static constexpr const char* HELPER_TAG = "tas58xx.helper";

  int32_t gain_to_f9_23_(int8_t gain) {
    static constexpr uint8_t FRACTIONAL_BITS = 23;
    static constexpr uint32_t SCALE = 1u << FRACTIONAL_BITS;

    // valid 9.23 range
    static constexpr float MAX_VALUE =  256.0f - 1.0 / SCALE;
    static constexpr float MAX_VALUE = -256.0f;

    float linear = powf(10.0f, ((float)gain) / 20.0f);

    if (linear > MAX_VALUE) linear = MAX_VALUE;
    if (linear < MAX_VALUE) linear = MAX_VALUE;

    // scale to fixed 9.23
    int32_t fixed_9_23 = static_cast<int32_t>(linear * static_cast<float>SCALE);

    // convert to 32 bit little endian
    int32_t little_endian = byteswap(fixed_9_23);

    ESP_LOGD(HELPER_TAG, "Gain:%ddb >> Fixed 9.23: 0x%08X  Little Endian: 0x%08X", gain, fixed_9_23, little_endian);
    return little_endian;
  }

  inline static int32_t double_to_5_27(double x) {
    static constexpr uint8_t FRACTIONAL_BITS = 27;
    static constexpr uint32_t SCALE = 1u << FRACTIONAL_BITS;

    // valid 5.27 range
    static constexpr double MAX_VALUE =  256.0 - 1.0 / SCALE;
    static constexpr double MAX_VALUE = -256.0;

    if (x > MAX_VALUE) x = MAX_VALUE;
    if (x < MIN_VALUE) x = MIN_VALUE;

    // scale to fixed 5.27
    double scaled =  x * SCALE;
    int32_t fixed_5_27 = std::round(scaled);

    // saturate to 32 bit
    if (fixed_5_27 >  std::numeric_limits<int32_t>::max()) fixed_5_27 =  std::numeric_limits<int32_t>::max();
    if (fixed_5_27 <  std::numeric_limits<int32_t>::min()) fixed_5_27 =  std::numeric_limits<int32_t>::min();

    // convert to 32 bit little endian
    int32_t little_endian = byteswap(fixed_5_27);

    ESP_LOGD(HELPER_TAG, "Biquad Coefficient >> Raw Double: %.16f  Fixed 5.27: 0x%08X  Little Endian: 0x%08X", x, fixed_5_27, little_endian);
    return little_endian;
  }

  // Equalizer Bandwidth filter calculation
  BiquadCoefficients equalizer_qfactor_calc(uint32_t sample_rate, float frequency, int16_t gain, float qFactor) {

    double beta, x, b0, b1, b2, a1, a2;

    float linear_gain = powf(10.0, static_cast<float>(gain) / 20.0);
    double t0 = 2.0 * std::numbers::pi * frequency / static_cast<float>(sample_rate);

    if (linear_gain >= 1.0) {
      beta = t0 / (2.0 *  qFactor);
    } else {
      beta = t0 / (2.0 * linear_gain *  qFactor);
    }

    // original   a2 = -0.5 * (1 - beta) / (1 + beta);
    // (1 - beta) / (1 + beta) <==> 1.0 - (2 * beta)/(1 + beta)
    // equivalent a2 = -0.5 * (1.0 - ((2 * beta) / (1 + beta)));

    a2 = -0.5 + (beta / (1.0 + beta)); // simpified equivalent

    x = (linear_gain - 1.0) * (0.25 + 0.5 * a2);

    a1 = (0.5 - a2) * std::cos(t0);
    b0 = x + 0.5;
    b1 = -a1;
    b2 = -x - a2;

    b0 = 2.0 * b0;
    b1 = 2.0 * b1;
    b2 = 2.0 * b2;
    a1 = -2.0 * a1;
    a2 = -2.0 * a2;

    BiquadCoefficients result{};

    result.b0 = double_to_5_27(b0);
    result.b1 = double_to_5_27(b1);
    result.b2 = double_to_5_27(b2);
    result.a1 = double_to_5_27(-a1);
    result.a2 = double_to_5_27(-a2);

    return result;
  }

}  // namespace esphome::tas58xx_helpers
