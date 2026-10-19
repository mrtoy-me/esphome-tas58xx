#include "tas58xx_helpers.h"

#include <climits>
#include <cmath>
#include <numbers>

#include "esphome/core/helpers.h"

namespace esphome::tas58xx {

// Double precision is used so coefficient values match TI PurePath Console 3, float precision does not give
// close matches. Double calculations on the ESP32 are much slower than float, so common terms are calculated
// once, exp replaces pow and multiplying by an inverse replaces dividing.

static constexpr double TWO_PI = 2.0 * std::numbers::pi;

// Q of the low pass and high pass filters = 1 / sqrt(2)
static constexpr double INVERSE_SQRT2 = 0.7071067811865475244;

// 10^(gain / N) = exp(gain * ln(10) / N)
static constexpr double LN10_DIV_20 = 0.11512925464970228420;
static constexpr double LN10_DIV_40 = 0.05756462732485114210;
static constexpr double LN10_DIV_80 = 0.02878231366242557105;

// 5.27 fixed point in the byte order written to the DSP, saturated to the 32 bit range
static int32_t double_to_fixed_5_27(double x) {
  static constexpr double SCALE = 134217728.0;  // 2^27
  // the 32 bit limits as doubles
  static constexpr double INT32_MAX_DOUBLE = 2147483647.0;
  static constexpr double INT32_MIN_DOUBLE = -2147483648.0;

  const double scaled = x * SCALE;

  int32_t fixed_5_27;
  if (scaled >= INT32_MAX_DOUBLE) {
    fixed_5_27 = INT32_MAX;
  } else if (scaled > INT32_MIN_DOUBLE) {
    fixed_5_27 = static_cast<int32_t>(std::round(scaled));
  } else {
    fixed_5_27 = INT32_MIN;
  }

  return byteswap(fixed_5_27);
}

// 10^(gain / N) with the exp constant ln(10) / N, exp(0) = 1 is not calculated
static double linear_gain(int8_t gain, double ln10_div_n) {
  if (gain == 0) return 1.0;
  return std::exp(static_cast<double>(gain) * ln10_div_n);
}

// w0 = 2 * pi * f0 / Fs
static double angular_frequency(uint32_t sample_rate, uint16_t frequency) {
  return TWO_PI * static_cast<double>(frequency) / static_cast<double>(sample_rate);
}

// Derived from biquad.model.js in TI Pure Path Console 3
static BiquadCoefficients equalizer_filter(uint32_t sample_rate, uint16_t frequency, int8_t gain, double q_factor) {
  const double ag = linear_gain(gain, LN10_DIV_20);
  const double t0 = angular_frequency(sample_rate, frequency);

  // the original uses ag >= 1.0, ie gain >= 0
  double beta;
  if (gain < 0) {
    beta = t0 / (ag * 2.0 * q_factor);
  } else {
    beta = t0 / (2.0 * q_factor);
  }

  // simplified from a2 = -0.5 * (1 - beta) / (1 + beta)
  const double a2 = 0.5 - (1.0 / (1.0 + beta));
  const double precalc = (ag - 1.0) * (0.25 + (0.5 * a2));
  const double a1 = (0.5 - a2) * std::cos(t0);

  // the original calculates b0, b1, b2 then doubles every coefficient and negates a1 and a2,
  // all of that is applied here directly
  BiquadCoefficients result{};
  result.b0 = double_to_fixed_5_27(1.0 + (2.0 * precalc));
  result.b1 = double_to_fixed_5_27(-2.0 * a1);
  result.b2 = double_to_fixed_5_27(-2.0 * (precalc + a2));
  result.a1 = double_to_fixed_5_27(2.0 * a1);
  result.a2 = double_to_fixed_5_27(2.0 * a2);
  return result;
}

// Derived from biquad.model.js in TI Pure Path Console 3
// A high shelf is a low shelf with cos(w0) negated and the signs of b1 and a1 flipped
static BiquadCoefficients shelf_filter(bool high, uint32_t sample_rate, uint16_t frequency, int8_t gain,
                                       double q_factor) {
  // sqrt(A) = 10^(gain / 80) and A = sqrt(A) * sqrt(A), which avoids a sqrt
  const double sqrt_ag = linear_gain(gain, LN10_DIV_80);
  const double ag = sqrt_ag * sqrt_ag;
  const double ag_plus1 = ag + 1.0;
  const double ag_minus1 = ag - 1.0;

  const double w0 = angular_frequency(sample_rate, frequency);
  const double sign = high ? -1.0 : 1.0;
  const double cos_w0 = sign * std::cos(w0);
  const double ag_plus1_cosw0 = ag_plus1 * cos_w0;
  const double ag_minus1_cosw0 = ag_minus1 * cos_w0;

  // the original alpha = sin(w0) / (2 * q_factor) is only used as 2 * sqrt(A) * alpha
  const double beta = sqrt_ag * std::sin(w0) / q_factor;

  const double precalc_x = ag_plus1 + ag_minus1_cosw0;
  const double precalc_y = ag_plus1 - ag_minus1_cosw0;

  const double inverse_a0 = 1.0 / (precalc_x + beta);

  const double ag_inv = ag * inverse_a0;
  const double ag_inv_y = ag_inv * precalc_y;
  const double ag_inv_beta = ag_inv * beta;

  BiquadCoefficients result{};
  result.b0 = double_to_fixed_5_27(ag_inv_y + ag_inv_beta);
  result.b1 = double_to_fixed_5_27(sign * (2.0 * ag_inv * (ag_minus1 - ag_plus1_cosw0)));
  result.b2 = double_to_fixed_5_27(ag_inv_y - ag_inv_beta);
  result.a1 = double_to_fixed_5_27(sign * (2.0 * (ag_minus1 + ag_plus1_cosw0) * inverse_a0));
  result.a2 = double_to_fixed_5_27((beta - precalc_x) * inverse_a0);
  return result;
}

// Derived from the Cookbook formulae for audio EQ biquad filter coefficients by Robert Bristow-Johnson,
// which are easier to optimise. Gives the same coefficients as the Butterworth 2 filters in TI Pure Path Console 3.
// A high pass is a low pass with cos(w0) negated and the sign of b1 flipped.
static BiquadCoefficients pass_filter(bool high, uint32_t sample_rate, uint16_t frequency, int8_t gain) {
  const double ag = linear_gain(gain, LN10_DIV_20);
  const double w0 = angular_frequency(sample_rate, frequency);
  const double sign = high ? -1.0 : 1.0;

  // alpha = sin(w0) / (2 * Q) with Q = 1 / sqrt(2)
  const double alpha = std::sin(w0) * INVERSE_SQRT2;
  const double inverse_a0 = 1.0 / (1.0 + alpha);
  const double cos_w0 = std::cos(w0);

  // b0 = (1 -/+ cos(w0)) / 2, then the gain and normalised by a0
  const double b0 = (1.0 - sign * cos_w0) * 0.5 * ag * inverse_a0;

  // a1 and a2 are multiplied by -1 as the DSP expects them
  BiquadCoefficients result{};
  result.b0 = double_to_fixed_5_27(b0);
  result.b1 = double_to_fixed_5_27(sign * 2.0 * b0);
  result.b2 = double_to_fixed_5_27(b0);
  result.a1 = double_to_fixed_5_27((2.0 * cos_w0) * inverse_a0);
  result.a2 = double_to_fixed_5_27((-1.0 + alpha) * inverse_a0);
  return result;
}

// 2nd order Chebyshev low pass / high pass as in TI Pure Path Console 3. The frequency is the edge of the ripple
// band, not the -3 dB point. ripple > 0: the ripple peaks are 0 dB and the DC (low pass) or Nyquist (high pass) gain
// is -ripple dB. ripple < 0: that gain is 0 dB and the peaks are +|ripple| dB. ripple = 0 gives all pass.
// Checked against PPC3 exports at 0.5 dB ripple: within 1 LSB (chebyshev-study.md, esphome dev_eq_test branch).
//
// TI's maths simplified with u = 10^(|ripple| / 20), epsilon = sqrt(u^2 - 1), e^(2 alpha) = sqrt((u + 1) / (u - 1)):
//   b = cosh(2 alpha) / 2 = u / (2 epsilon)           pole product in units of wc^2
//   p = -a = sqrt(2) sinh(alpha) = sqrt(2 b - 1)      pole sum in units of wc
//   c = b / u = 1 / (2 epsilon) for ripple > 0, else c = b
// With T = tan(pi f / Fs), k = wc / T, dividing every term by wc^2 and multiplying by T^2 removes wc:
//   low pass:  D = 1 + p T + b T^2, b0 = c T^2 / D, a1 = (2 - 2 b T^2) / D, a2 = (p T - 1 - b T^2) / D
//   high pass: D = b + p T + T^2,   b0 = c / D,     a1 = (2 b - 2 T^2) / D, a2 = (p T - b - T^2) / D
// b1 = +/-2 b0 and b2 = b0; a1 and a2 are already in the sign the DSP expects.
static BiquadCoefficients chebyshev_filter(bool high, uint32_t sample_rate, uint16_t frequency, int8_t gain,
                                           double ripple) {
  if (ripple == 0.0) return ALL_PASS;

  const double u = std::exp(std::fabs(ripple) * LN10_DIV_20);
  const double inverse_epsilon = 1.0 / std::sqrt((u - 1.0) * (u + 1.0));
  const double b = 0.5 * u * inverse_epsilon;
  const double c = ripple > 0.0 ? 0.5 * inverse_epsilon : b;
  const double p = std::sqrt((u * inverse_epsilon) - 1.0);

  const double t = std::tan(0.5 * angular_frequency(sample_rate, frequency));
  const double t2 = t * t;
  const double pt = p * t;
  const double ag = linear_gain(gain, LN10_DIV_20);

  BiquadCoefficients result{};
  double b0;
  if (high) {
    const double inverse_d = 1.0 / (b + pt + t2);
    b0 = c * inverse_d * ag;
    result.b1 = double_to_fixed_5_27(-2.0 * b0);
    result.a1 = double_to_fixed_5_27((2.0 * (b - t2)) * inverse_d);
    result.a2 = double_to_fixed_5_27((pt - b - t2) * inverse_d);
  } else {
    const double bt2 = b * t2;
    const double inverse_d = 1.0 / (1.0 + pt + bt2);
    b0 = c * t2 * inverse_d * ag;
    result.b1 = double_to_fixed_5_27(2.0 * b0);
    result.a1 = double_to_fixed_5_27((2.0 - (2.0 * bt2)) * inverse_d);
    result.a2 = double_to_fixed_5_27((pt - 1.0 - bt2) * inverse_d);
  }
  result.b0 = double_to_fixed_5_27(b0);
  result.b2 = result.b0;
  return result;
}

// Derived from biquad.model.js in TI Pure Path Console 3
static BiquadCoefficients peaking_filter(uint32_t sample_rate, uint16_t frequency, int8_t gain, double q_factor) {
  // A = sqrt(10^(gain / 20)) = 10^(gain / 40)
  const double ag = linear_gain(gain, LN10_DIV_40);
  const double w0 = angular_frequency(sample_rate, frequency);

  const double alpha = std::sin(w0) / (2.0 * q_factor);
  const double alpha_divide_ag = alpha / ag;

  const double inverse_a0 = 1.0 / (1.0 + alpha_divide_ag);
  const double aag_inverse_a0 = alpha * ag * inverse_a0;
  const double b1 = -2.0 * std::cos(w0) * inverse_a0;

  BiquadCoefficients result{};
  result.b0 = double_to_fixed_5_27(inverse_a0 + aag_inverse_a0);
  result.b1 = double_to_fixed_5_27(b1);
  result.b2 = double_to_fixed_5_27(inverse_a0 - aag_inverse_a0);
  result.a1 = double_to_fixed_5_27(-b1);
  result.a2 = double_to_fixed_5_27((-1.0 + alpha_divide_ag) * inverse_a0);
  return result;
}

// Derived from biquad.model.js in TI Pure Path Console 3 (bandPassCalc and notchCalc), which take a bandwidth.
// Here bandwidth = frequency / Q, so t = tan(pi * bandwidth / Fs) = tan(w0 / (2 * Q)).
// TI's band pass calculates C = tan(Wc / 2) from the band edges, but C cancels out of every coefficient, so it
// is not calculated; its alpha = cos(w0) / cos(pi * bandwidth / Fs) uses 1 / cos(x) = sqrt(1 + tan(x)^2).
static BiquadCoefficients bandwidth_filter(bool notch, uint32_t sample_rate, uint16_t frequency, double q_factor) {
  const double w0 = angular_frequency(sample_rate, frequency);
  const double t = std::tan(0.5 * w0 / q_factor);
  const double inverse_a0 = 1.0 / (1.0 + t);
  const double cos_w0 = std::cos(w0);

  BiquadCoefficients result{};
  if (notch) {
    const double b1 = -2.0 * cos_w0 * inverse_a0;
    result.b0 = double_to_fixed_5_27(inverse_a0);
    result.b1 = double_to_fixed_5_27(b1);
    result.b2 = result.b0;
    result.a1 = double_to_fixed_5_27(-b1);
  } else {
    const double b0 = t * inverse_a0;
    result.b0 = double_to_fixed_5_27(b0);
    result.b1 = double_to_fixed_5_27(0.0);
    result.b2 = double_to_fixed_5_27(-b0);
    result.a1 = double_to_fixed_5_27(2.0 * cos_w0 * inverse_a0 * std::sqrt(1.0 + t * t));
  }
  result.a2 = double_to_fixed_5_27((t - 1.0) * inverse_a0);
  return result;
}


int32_t gain_to_fixed_9_23(int8_t gain) {
  static constexpr float SCALE = 8388608.0f;  // 2^23

  // 9.23 range is -256 to just under 256, float cannot hold the upper limit so the largest value below 256 is used
  static constexpr float MAX_VALUE = 255.99998f;
  static constexpr float MIN_VALUE = -256.0f;

  float linear = std::exp(static_cast<float>(gain) * static_cast<float>(LN10_DIV_20));
  if (linear > MAX_VALUE) linear = MAX_VALUE;
  if (linear < MIN_VALUE) linear = MIN_VALUE;

  const int32_t fixed_9_23 = static_cast<int32_t>(linear * SCALE);
  return byteswap(fixed_9_23);
}

BiquadCoefficients make_biquad(FilterType type, uint32_t sample_rate, uint16_t frequency, int8_t gain,
                               double q_factor) {
  switch (type) {
    case FILTER_TYPE_EQUALIZER:
      return equalizer_filter(sample_rate, frequency, gain, q_factor);
    case FILTER_TYPE_PEAKING:
      return peaking_filter(sample_rate, frequency, gain, q_factor);
    case FILTER_TYPE_LOW_SHELF:
      return shelf_filter(false, sample_rate, frequency, gain, q_factor);
    case FILTER_TYPE_HIGH_SHELF:
      return shelf_filter(true, sample_rate, frequency, gain, q_factor);
    case FILTER_TYPE_LOW_PASS:
      return pass_filter(false, sample_rate, frequency, gain);
    case FILTER_TYPE_HIGH_PASS:
      return pass_filter(true, sample_rate, frequency, gain);
    case FILTER_TYPE_NOTCH:
      return bandwidth_filter(true, sample_rate, frequency, q_factor);
    case FILTER_TYPE_BAND_PASS:
      return bandwidth_filter(false, sample_rate, frequency, q_factor);
    case FILTER_TYPE_LOW_PASS_CHEBYSHEV:
      return chebyshev_filter(false, sample_rate, frequency, gain, q_factor);
    case FILTER_TYPE_HIGH_PASS_CHEBYSHEV:
      return chebyshev_filter(true, sample_rate, frequency, gain, q_factor);
    case FILTER_TYPE_ALL_PASS:
    default:
      return ALL_PASS;
  }
}

}  // namespace esphome::tas58xx
