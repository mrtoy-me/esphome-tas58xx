#include "tas58xx_helpers.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include <cmath>

namespace esphome::tas58xx_helpers {

  static constexpr const char* HELPER_TAG = "tas58xx.helper";

  int32_t gain_to_f9_23_(int8_t gain) {
    static constexpr uint8_t FRACTIONAL_BITS = 23;
    static constexpr float SCALE = static_cast<float>(1u << FRACTIONAL_BITS);

    static constexpr float LINEAR_GAIN_MAX =  256.0f - 1.0 / SCALE;
    static constexpr float LINEAR_GAIN_MIN = -256.0f;

    float linear = powf(10.0f, ((float)gain) / 20.0f);
    if (linear > LINEAR_GAIN_MAX) linear = LINEAR_GAIN_MAX;
    if (linear < LINEAR_GAIN_MIN) linear = LINEAR_GAIN_MIN;

    int32_t fixed_9_23 = static_cast<int32_t>(linear * SCALE);
    int32_t little_endian = byteswap(fixed_9_23);

    ESP_LOGD(HELPER_TAG, "Gain:%ddb >> Fixed 9.23: 0x%08X  Little Endian: 0x%08X", gain, fixed_9_23, little_endian);
    return little_endian;
  }

  static int32_t double_to_5_27(double x) {
    static constexpr uint8_t FRACTIONAL_BITS = 27;
    static constexpr float SCALE = static_cast<float>(1u << FRACTIONAL_BITS);

    // Valid 9.23 range
    static constexpr double MAX_VALUE =  256.0 - 1.0 / SCALE;
    static constexpr double MIN_VALUE = -256.0;

    if (x > MAX_VALUE) x = MAX_VALUE;
    if (x < MIN_VALUE) x = MIN_VALUE;

    // Scale
    double scaled = x * SCALE;
    int64_t q = std::llround(scaled);

    // Saturate to 32 bit
    if (q >  std::numeric_limits<int32_t>::max()) q =  std::numeric_limits<int32_t>::max();
    if (q <  std::numeric_limits<int32_t>::min()) q =  std::numeric_limits<int32_t>::min();

    // convert to 32 bit little endian
    int32_t q_32bit = static_cast<int32_t>(q);
    int32_t q_little_endian = byteswap(q_32bit);

    ESP_LOGD(HELPER_TAG, "Biquad Coefficient >> Fixed 5.27: 0x%08X  Little Endian: 0x%08X", q_32bit, q_little_endian);
    return q_little_endian;
  }

  BiquadCoefficients butterworth2_(float fs, float fc, Butterworth2Type type) {
    static constexpr double Q = 1.0 / std::sqrt(2.0);
    const double w0 = 2.0 * std::numbers::pi * fc / fs;
    const double alpha = std::sin(w0) / (2.0 * Q);
    const double cosw0 = std::cos(w0);

    double b0, b1, b2, a0, a1, a2;

    if (type == Butterworth2Type::LOWPASS) {
        b0 = (1.0 - cosw0) * 0.5;
        b1 = 1.0 - cosw0;
        b2 = (1.0 - cosw0) * 0.5;
    } else if (type == Butterworth2Type::HIGHPASS) {
        b0 = (1.0 + cosw0) * 0.5;
        b1 = -(1.0 + cosw0);
        b2 = (1.0 + cosw0) * 0.5;
    }

    a0 = 1.0 + alpha;
    a1 = -2.0 * cosw0;
    a2 = 1.0 - alpha;

    // Normalize
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= -a0;
    a2 /= -a0;

    BiquadCoefficients result{};

    result.b0 = double_to_5_27(b0);
    result.b1 = double_to_5_27(b1);
    result.b2 = double_to_5_27(b2);
    result.a1 = double_to_5_27(a1);
    result.a2 = double_to_5_27(a2);

    return result;
  }

}  // namespace esphome::tas58xx_helpers