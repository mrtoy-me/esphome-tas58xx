#include "tas58xx_helpers.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx_helpers {

  static constexpr const char* HELPER_TAG = "tas58xx.helper";

  int32_t gain_to_f9_23_(int8_t gain) {
    static constexpr uint8_t FRACTIONAL_BITS = 23;
    static constexpr uint32_t SCALE = 1u << FRACTIONAL_BITS;

    // valid 9.23 range
    static constexpr float MAX_VALUE =  256.0f - 1.0 / SCALE;
    static constexpr float MIN_VALUE = -256.0f;

    float linear = powf(10.0f, ((float)gain) / 20.0f);

    if (linear > MAX_VALUE) linear = MAX_VALUE;
    if (linear < MIN_VALUE) linear = MIN_VALUE;

    // scale to fixed 9.23
    int32_t fixed_9_23 = static_cast<int32_t>(linear * static_cast<float>(SCALE));

    // convert to 32 bit little endian
    int32_t little_endian = byteswap(fixed_9_23);

    //ESP_LOGD(HELPER_TAG, "Gain:%ddb >> Fixed 9.23: 0x%08X  Little Endian: 0x%08X", gain, fixed_9_23, little_endian);
    return little_endian;
  }

  inline static int32_t double_to_5_27(double x) {
    static constexpr uint8_t FRACTIONAL_BITS = 27;
    static constexpr uint32_t SCALE = 1u << FRACTIONAL_BITS;

    // valid 5.27 range
    static constexpr double MAX_VALUE =  256.0 - (1.0 / SCALE);
    static constexpr double MIN_VALUE = -256.0;

    if (x > MAX_VALUE) x = MAX_VALUE;
    if (x < MIN_VALUE) x = MIN_VALUE;

    // scale to fixed 5.27
    double scaled =  x * SCALE;

    // saturate to 32 bit
    int64_t long_fixed_5_27 = static_cast<int64_t>(scaled);
    if (long_fixed_5_27 >  INT_MAX) scaled =  INT_MAX;
    if (long_fixed_5_27 <  INT_MIN) scaled =  INT_MIN;

    int32_t fixed_5_27 = std::round(scaled);

    // convert to 32 bit little endian
    int32_t little_endian = byteswap(fixed_5_27);

    //ESP_LOGD(HELPER_TAG, "Biquad Coefficient >> Raw Double: %.16f  Fixed 5.27: 0x%08X  Little Endian: 0x%08X", x, fixed_5_27, little_endian);
    return little_endian;
  }

  // Equalizer Bandwidth filter calculation
  BiquadCoefficients equalizer_qfactor_calc(uint32_t sample_rate, uint16_t frequency, int16_t gain, float q_factor) {

    double linear_gain = std::pow(10.0, gain / 20.0);
    double t0 = 2.0 * std::numbers::pi * frequency / sample_rate;
    float q_factor_x2 = 2.0f * q_factor;

    double beta;
    // original linear_gain >= 1.0 <=> gain >= 0
    if (gain >= 0) {
      beta = t0 / q_factor_x2;
    } else {
      beta = t0 / (linear_gain * q_factor_x2);
    }

    // Simpify Original <=> -0.5 * (1 - beta) / (1 + beta)
    // Flip the sign into the numerator <=> 0.5 * (beta − 1) / (1 + beta)
    // Rewrite (beta - 1) as (1 + beta - 2) <=> 0.5 * ((1 + beta) − 2) / (1 + beta)
    // Split the fraction <=> (0.5 * (1 + beta)) / (1 + beta) − (1 / (1 + beta))
    // (1 + beta) cancels in the left term <=> 0.5 − (1 / (1 + beta))

    double a2 = 0.5 - (1.0 / (1.0 + beta)); // simpified equivalent

    double x = (linear_gain - 1.0) * (0.25 + (0.5 * a2));

    double a1 = (0.5 - a2) * std::cos(t0);

    // Original -> simpify and pass direct to double_to_5_27
    // b0 = x + 0.5;
    // b1 = -a1;
    // b2 = -x - a2;

    // b0 = 2.0 * b0;
    // b1 = 2.0 * b1;
    // b2 = 2.0 * b2;
    // a1 = -2.0 * a1;
    // a2 = -2.0 * a2;

    BiquadCoefficients result{};

    result.b0 = double_to_5_27( 1.0 + (2.0 * x) );
    result.b1 = double_to_5_27( -2.0 * a1 );
    result.b2 = double_to_5_27( -2.0 * (x + a2) );
    result.a1 = double_to_5_27( 2.0 * a1 ) ;
    result.a2 = double_to_5_27( 2.0 * a2 );

    return result;
  }


  BiquadCoefficients equalizer_lowshelf_calc(uint32_t sample_rate, uint16_t frequency, int16_t gain, float q_factor) {

    // originally
    // a = sqrt(pow(10.0, gain / 40.0)) <=> a = pow(10.0, gain / 40.0);

    // use equivalent of sqrt(a) to eliminate sqrt in beta calculation and replace with multiplication in calculating value of a
    double sqrt_a = std::pow(10.0, gain / 80.0);
    double a = sqrt_a * sqrt_a;

    double a_plus1 = a + 1.0;
    double a_minus1 = a - 1.0;

    double w0 = 2.0 * std::numbers::pi * frequency / sample_rate;
    double sinw0, cosw0;
    sincos(w0, &sinw0, &cosw0);

    double a_plus1_cosw0 = a_plus1 * cosw0;
    double a_minus1_cosw0 = a_minus1 * cosw0;

    double ap_p_amc = a_plus1 + a_minus1_cosw0;
    double ap_m_amc = a_plus1 - a_minus1_cosw0;

    // originally
    // alpha = sin(w0) / (2.0 * q_factor);
    // beta = 2.0 * sqrt(a) * sin(w0) / (2.0 * q_factor);
    double beta = sqrt_a * sinw0 / q_factor; // simplify

    double inverse_a0 = 1.0 / (ap_p_amc + beta);

    BiquadCoefficients result{};

    // originally used / a0 but multiplication of inverse more efficient than division
    result.b0 = double_to_5_27( a * (ap_m_amc + beta) * inverse_a0 );
    result.b1 = double_to_5_27( 2.0 * a * (a_minus1 - a_plus1_cosw0) * inverse_a0 );
    result.b2 = double_to_5_27( a * (ap_m_amc - beta) * inverse_a0 );
    result.a1 = double_to_5_27( 2.0 * (a_minus1 + a_plus1_cosw0) * inverse_a0 );
    result.a2 = double_to_5_27( (beta - ap_p_amc) * inverse_a0 ) ;

    return result;
};

BiquadCoefficients equalizer_highshelf_calc(uint32_t sample_rate, uint16_t frequency, int16_t gain, float q_factor) {

    // originally
    // a = sqrt(powf(10.0, gain / 40.0)) <=> a = powf(10.0, gain / 40.0);

    // use equivalent of sqrt(a) to eliminate sqrt in beta calculation and replace with multiplication in calculating value of a
    double sqrt_a = std::pow(10.0, gain / 80.0);
    double a = sqrt_a * sqrt_a;

    double a_plus1 = a + 1.0;
    double a_minus1 = a - 1.0;

    double w0 = 2.0 * std::numbers::pi * frequency / sample_rate;
    double sinw0, cosw0;
    sincos(w0, &sinw0, &cosw0);

    double a_plus1_cosw0 = a_plus1 * cosw0;
    double a_minus1_cosw0 = a_minus1 * cosw0;

    double ap_p_amc = a_plus1 + a_minus1_cosw0;
    double ap_m_amc = a_plus1 - a_minus1_cosw0;

    // originally
    // alpha = sin(w0) / (2.0 * q_factor);
    // beta = 2.0 * sqrt(a) * sin(w0) / (2.0 * q_factor);
    double beta = sqrt_a * sinw0 / q_factor;

    // originally a0 = ap_m_amc + beta but multiplication more efficient than division
    double inverse_a0 = 1.0 / (ap_m_amc + beta);

    BiquadCoefficients result{};

    result.b0 = double_to_5_27( a * (ap_p_amc + beta) * inverse_a0 );
    result.b1 = double_to_5_27( -2.0 * a * (a_minus1 + a_plus1_cosw0) * inverse_a0 );
    result.b2 = double_to_5_27( a * (ap_p_amc - beta) * inverse_a0 );
    result.a1 = double_to_5_27( -2.0 * (a_minus1 - a_plus1_cosw0) * inverse_a0 );
    result.a2 = double_to_5_27( (beta - ap_m_amc) * inverse_a0 ) ;

    return result;
};

BiquadCoefficients low_pass_butterworth2_calc(uint32_t sample_rate, uint16_t frequency, int16_t gain) {

const double linear_gain = std::pow(10.0, gain / 20.0);
// w0 = 2 * pi * f0/Fs
const double w0 = 2.0 * std::numbers::pi * frequency / sample_rate;
double sinw0, cosw0;
sincos(w0, &sin_w0, &cos_w0);

// Q = 1 / sqrt(2)
// alpha = sin(w0)/(2*Q)
const double alpha = sin_w0 * std::numbers::sqrt2 * 0.5;

const double inverse_a0 =   1.0 / (1.0 + alpha);

double b0 = (1.0 - cos_w0) * 0.5 * linear_gain * inverse_a0;

BiquadCoefficients result{};

result.b0 = double_to_5_27( b0 );
result.b1 = double_to_5_27( 2.0 * b0 );
result.b2 = double_to_5_27( b0 );
result.a1 = double_to_5_27( (-2.0 * cos_w0) * inverse_a0 );
result.a2 = double_to_5_27( (1.0 - alpha) * inverse_a0 );


  // static constexpr double qfactor = 1.0 / std::sqrt(2.0);

  // double pi_freq = M_PI * frequency;
  // double wc = 2.0 * pi_freq;
  // double capital_a1 = wc / qfactor;
  // // <==>double capital_a1 = wc * std::numbers::sqrt2;
  // double wc_squared = wc * wc;
  // double k = wc / tan(pi_freq / sample_rate);
  // double inverse_denominator = 1 / ((k * (k + capital_a1)) + wc_squared);

  // double k_squared = k * k;
  // double linear_gain = pow(10.0, gain / 20.0);

  // double b0 = wc_squared * linear_gain * inverse_denominator;

  // result.b0 = double_to_5_27( b0 );
  // result.b1 = double_to_5_27( 2.0 * b0 );
  // result.b2 = double_to_5_27( b0 );
  // result.a1 = double_to_5_27( 2.0 * (k_squared - wc_squared) * inverse_denominator );
  // result.a2 = double_to_5_27( ((capital_a1 * k) - k_squared - wc_squared) * inverse_denominator );

  return result;
};

}  // namespace esphome::tas58xx_helpers

// wc = 2 * Math.PI * freq;
//                 qFactor = 1 / Math.sqrt(2);
//                 A1 = wc / qFactor;
//                 A2 = Math.pow(wc, 2);
//                 k = wc / Math.tan(Math.PI * freq / fs);
//                 denominator = Math.pow(k, 2) + A2 + A1 * k;

//                 b0 = Math.pow(wc, 2) / denominator;
//                 b1 = 2 * Math.pow(wc, 2) / denominator;
//                 b2 = Math.pow(wc, 2) / denominator;
//                 a1 = (2 * Math.pow(k, 2) - 2 * A2) / denominator;
//                 a2 = (A1 * k - Math.pow(k, 2) - A2) / denominator;



// }  // namespace esphome::tas58xx_helpers
// gain = Math.pow(10, (gain/20));
//                 b0 *= gain;
//                 b1 *= gain;
//                 b2 *= gain;