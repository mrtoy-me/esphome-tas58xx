#include "tas58xx_helpers.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx_helpers {

  static constexpr const char* HELPER_TAG = "tas58xx.helper";
  constexpr double LN10_DIV_20 = 0.11512925464970228420;  // ln(10) / 20
  constexpr double LN10_DIV_40 = 0.05756462732485114210;  // ln(10) / 40
  constexpr double LN10_DIV_80 = 0.02878231366242557105;  // ln(10) / 80
  constexpr double INVERSE_SQRT2 = 0.7071067811865475244;

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

  inline int32_t double_to_5_27(double x) {
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

    ESP_LOGD(HELPER_TAG, "Biquad Coefficient >> Raw Double: %.16f  Fixed 5.27: 0x%08X  Little Endian: 0x%08X", x, fixed_5_27, little_endian);
    return little_endian;
  }

  BiquadCoefficients equalizer_qfactor_(uint32_t sample_rate, uint16_t frequency, int16_t gain, float q_factor) {
    // originally A = pow(10, gain / 20)
    // pow(10, gain / 20) <=> exp(gain * (ln(10) / 20)
    const double ag = std::exp(gain * LN10_DIV_20);

    const double t0 = 2.0 * std::numbers::pi * frequency / sample_rate;
    const float q_factor_x2 = 2.0f * q_factor;

    double beta;
    // original gain_a >= 1.0 <=> gain >= 0
    if (gain >= 0) {
      beta = t0 / q_factor_x2;
    } else {
      beta = t0 / (ag * q_factor_x2);
    }

    // Simpify Original <=> a2 = -0.5 * (1 - beta) / (1 + beta)
    // Flip the sign into the numerator <=> 0.5 * (beta − 1) / (1 + beta)
    // Rewrite (beta - 1) as (1 + beta - 2) <=> 0.5 * ((1 + beta) − 2) / (1 + beta)
    // Split the fraction <=> (0.5 * (1 + beta)) / (1 + beta) − (1 / (1 + beta))
    // (1 + beta) cancels in the left term <=> 0.5 − (1 / (1 + beta))

    const double a2 = 0.5 - (1.0 / (1.0 + beta)); // simpified equivalent

    const double precalc = (ag - 1.0) * (0.25 + (0.5 * a2));

    const double a1 = (0.5 - a2) * std::cos(t0);

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

    result.b0 = double_to_5_27( 1.0 + (2.0 * precalc) );
    result.b1 = double_to_5_27( -2.0 * a1 );
    result.b2 = double_to_5_27( -2.0 * (precalc + a2) );
    result.a1 = double_to_5_27( 2.0 * a1 ) ;
    result.a2 = double_to_5_27( 2.0 * a2 );

    return result;
  }

  BiquadCoefficients low_shelf_filter_(uint32_t sample_rate, uint16_t frequency, int16_t gain, float q_factor) {
    // originally A = sqrt(pow(10, gain / 40)) <=> sqrt(a) = pow(10, gain / 80);
    // sqrt(a) = pow(10, gain / 80) <=> exp(gain * (ln(10)/80)
    // use sqrt_ag to calculate value of ag replaces sqrt with multiplication also eliminates sqrt in "beta" calculation
    const double sqrt_ag = std::exp(gain * LN10_DIV_80);

    // used multple times - precompute once
    const double ag = sqrt_ag * sqrt_ag;
    const double ag_plus1 = ag + 1.0;
    const double ag_minus1 = ag - 1.0;

    const double w0 = 2.0 * std::numbers::pi * frequency / sample_rate;
    double sinw0, cosw0;
    sincos(w0, &sinw0, &cosw0);

    // used multple times - precompute once
    const double ag_plus1_cosw0 = ag_plus1 * cosw0;
    const double ag_minus1_cosw0 = ag_minus1 * cosw0;
    // originally
    // alpha = sin(w0) / (2 * q_factor);
    // beta = 2 * sqrt(a) * sin(w0) / (2 * q_factor);
    const double beta = sqrt_ag * sinw0 / q_factor; // simplified

    const double precalc_x = ag_plus1 + ag_minus1_cosw0;
    const double precalc_y = ag_plus1 - ag_minus1_cosw0;
    double inverse_a0 = 1.0 / (precalc_x + beta);

    // shared multipliers — precompute once
    const double ag_inv = ag * inverse_a0;       // saves recomputing 3× across b0, b1, b2
    const double ag_inv_y = ag_inv * precalc_y;  // shared between b0 and b2
    const double ag_inv_beta = ag_inv * beta;    // shared between b0 and b2

    BiquadCoefficients result{};
    result.b0 = double_to_5_27( ag_inv_y + ag_inv_beta );
    result.b1 = double_to_5_27( 2.0 * ag_inv * (ag_minus1 - ag_plus1_cosw0) );
    result.b2 = double_to_5_27( ag_inv_y - ag_inv_beta );
    result.a1 = double_to_5_27( 2.0 * (ag_minus1 + ag_plus1_cosw0) * inverse_a0 );
    result.a2 = double_to_5_27( (beta - precalc_x) * inverse_a0 );
    return result;
};

BiquadCoefficients high_shelf_filter_(uint32_t sample_rate, uint16_t frequency, int16_t gain, float q_factor) {
    // originally A = sqrt(pow(10, gain / 40)) <=> sqrt(A) = pow(10, gain / 80);
    // sqrt(A) = pow(10, gain / 80) <=> exp(gain * (ln(10)/80)
    // use sqrt(a) to calculate value of "a" replaces sqrt with multiplication also eliminates sqrt in "beta" calculation
    const double sqrt_ag = std::exp(gain * LN10_DIV_80);

    // used multple times - precompute once
    const double ag = sqrt_ag * sqrt_ag;
    const double ag_plus1 = ag + 1.0;
    const double ag_minus1 = ag - 1.0;

    const double w0 = 2.0 * std::numbers::pi * frequency / sample_rate;
    double sinw0, cosw0;
    sincos(w0, &sinw0, &cosw0);

    // used multple times - precompute once
    const double ag_plus1_cosw0 = ag_plus1 * cosw0;
    const double ag_minus1_cosw0 = ag_minus1 * cosw0;
    // originally
    // alpha = sin(w0) / (2.0 * q_factor);
    // beta = 2.0 * sqrt(A) * sin(w0) / (2.0 * q_factor);
    const double beta = sqrt_ag * sinw0 / q_factor;

    const double precalc_x = ag_plus1 + ag_minus1_cosw0;
    const double precalc_y = ag_plus1 - ag_minus1_cosw0;
    const double inverse_a0 = 1.0 / (precalc_y + beta);

    // shared multipliers — precompute once
    const double ag_inv = ag * inverse_a0;       // saves recomputing 3× across b0, b1, b2
    const double ag_inv_x = ag_inv * precalc_x;  // shared between b0 and b2
    const double ag_inv_beta = ag_inv * beta;    // shared between b0 and b2

    BiquadCoefficients result{};
    result.b0 = double_to_5_27( ag_inv_x + ag_inv_beta );
    result.b1 = double_to_5_27( -2.0 * ag_inv * (ag_minus1 + ag_plus1_cosw0) );
    result.b2 = double_to_5_27(  ag_inv_x - ag_inv_beta );
    result.a1 = double_to_5_27( -2.0 * (ag_minus1 - ag_plus1_cosw0) * inverse_a0 );
    result.a2 = double_to_5_27( (beta - precalc_y) * inverse_a0 );
    return result;
};

BiquadCoefficients low_pass_filter_(uint32_t sample_rate, uint16_t frequency, int16_t gain) {
// same results as low pass butterworth 2 filter in TI Pure Path Console 3
// derived from Cookbook formulae for audio EQ biquad filter coefficients by Robert Bristow-Johnson

  // originally A = pow(10, gain / 20))
  // pow(10, gain / 20) <=> exp(gain * (ln(10)/20)
  const double ag = std::exp(gain * LN10_DIV_20);

  // w0 = 2 * pi * f0 / Fs
  const double w0 = 2.0 * std::numbers::pi * frequency / sample_rate;
  double sin_w0, cos_w0;
  sincos(w0, &sin_w0, &cos_w0);

  // Q = 1 / sqrt(2)
  // alpha = sin(w0) / (2 * Q) <=> sin_w0 * sqrt(2) / 2 <=> sin_w0 / sqrt(2)
  const double alpha = sin_w0 * INVERSE_SQRT2;

  const double inverse_a0 =   1.0 / (1.0 + alpha);              // a0 =   1 + alpha

  const double b0 = (1.0 - cos_w0) * 0.5 * ag * inverse_a0;     // b0 =  (1 - cos(w0))/2 then gain adjustment and normalise

  BiquadCoefficients result{};

  result.b0 = double_to_5_27( b0 );
  result.b1 = double_to_5_27( 2.0 * b0 );                       // b1 = 1 - cos(w0)
  result.b2 = double_to_5_27( b0 );                             // b2 = (1 - cos(w0))/2
  result.a1 = double_to_5_27( (2.0 * cos_w0) * inverse_a0 );    // a1 =  2*cos(w0) then normalise and final multiply by -1 applied
  result.a2 = double_to_5_27( (-1.0 + alpha) * inverse_a0 );    // a2 =  1 - alpha then normalise and final multiply by -1 applied

  return result;
};

BiquadCoefficients high_pass_filter_(uint32_t sample_rate, uint16_t frequency, int16_t gain) {
// same results as high pass butterworth 2 filter in TI Pure Path Console 3
// derived from Cookbook formulae for audio EQ biquad filter coefficients by Robert Bristow-Johnson

  // originally linear_gain = pow(10, gain / 20))
  // pow(10, gain / 20) <=> exp(gain * (ln(10)/20)
  const double ag = std::exp(gain * LN10_DIV_20);

  // w0 = 2 * pi * f0 / Fs
  const double w0 = 2.0 * std::numbers::pi * frequency / sample_rate;
  double sin_w0, cos_w0;
  sincos(w0, &sin_w0, &cos_w0);

  // Q = 1 / sqrt(2)
  // alpha = sin(w0) / (2 * Q) <=> sin_w0 * sqrt(2) / 2 <=> sin_w0 / sqrt(2)
  const double alpha = sin_w0 * INVERSE_SQRT2;

  const double inverse_a0 =   1.0 / (1.0 + alpha);              // a0 =   1 + alpha

  const double b0 = (1.0 + cos_w0) * 0.5 * ag * inverse_a0;     // b0 =  (1 + cos(w0))/2 then gain adjustment and normalise

  BiquadCoefficients result{};

  result.b0 = double_to_5_27( b0 );
  result.b1 = double_to_5_27( -2.0 * b0 );                      // b1 = -(1 + cos(w0))
  result.b2 = double_to_5_27( b0 );                             // b2 =  (1 + cos(w0))/2
  result.a1 = double_to_5_27( (2.0 * cos_w0) * inverse_a0 );    // a1 =  -2*cos(w0) then normalise and final multiply by -1 applied
  result.a2 = double_to_5_27( (-1.0 + alpha) * inverse_a0 );    // a2 =   1 - alpha then normalise and final multiply by -1 applied

  return result;
};

BiquadCoefficients peaking_eq_(uint32_t sample_rate, uint16_t frequency, int16_t gain, float q_factor) {
  // originally A = sqrt(Math.pow(10, gain / 20)) <==> exp(gain * ln(10)/40)
  const double ag = std::exp(gain * LN10_DIV_40);

  const double w0 = 2.0 * std::numbers::pi * frequency / sample_rate;
  double sin_w0, cos_w0;
  sincos(w0, &sin_w0, &cos_w0);

  // use multiple times - precompute once
  const double alpha = sin_w0 / (2.0 * q_factor);
  const double alpha_divide_ag = alpha / ag;

  // a0 = 1 + alpha / A
  // use multiple times - precompute once
  const double inverse_a0 = 1.0 / (1.0 +  alpha_divide_ag);

  // shared between b0 and b2 - precompute once
  const double aag_inverse_a0 = alpha * ag * inverse_a0;

  const double b1 = -2.0 * cos_w0 * inverse_a0;                        // b1 = -2 * cos(w0) then normalise

  BiquadCoefficients result{};

  result.b0 = double_to_5_27( inverse_a0 + aag_inverse_a0);            // b0 = 1 + alpha * A then normalise
  result.b1 = double_to_5_27( b1 );
  result.b2 = double_to_5_27( inverse_a0 - aag_inverse_a0 );           // b2 = 1 - alpha * A then normalise
  result.a1 = double_to_5_27( -b1 );                                   // a1 = b1 and apply multiply by -1
  result.a2 = double_to_5_27( (-1.0 + alpha_divide_ag) * inverse_a0 ); // a2 = 1 - alpha / A then normalise and apply multiply by -1

  return result;
};

BiquadCoefficients band_pass_filter_(uint32_t sample_rate, uint16_t frequency, uint16_t bandwidth) {
  const double pi_inverse_sample_rate = std::numbers::pi / sample_rate;

  const double wf = 2.0 * pi_inverse_sample_rate * frequency;
  const double wb_half = pi_inverse_sample_rate * bandwidth;
  const double wc = std::sqrt((wf * wf) - (wb_half * wb_half));

  double sin_wch, cos_wch;
  sincos(wc * 0.5, &sin_wch, &cos_wch);
  const double c = cos_wch / cos_wch;

  double sin_bw, cos_bw;
  sincos(wb_half, &sin_bw, &cos_bw);

  const double k = c * cos_bw / sin_bw;  // c / tan(wb)
  const double alpha = std::cos(wf) / cos_bw;

  const double inverse_kpc = 1.0 / (k + c);
  const double c_x = c * inverse_kpc;
  const double k_x = k * inverse_kpc;

  BiquadCoefficients result{};
  result.b0 = double_to_5_27( c_x );
  result.b1 = double_to_5_27( 0.0 );
  result.b2 = double_to_5_27( -c_x );
  result.a1 = double_to_5_27( 2.0 * alpha * k_x );
  result.a2 = double_to_5_27( c_x - k_x );

  return result;
  //claude from js

  // const double pi_isr    = std::numbers::pi / sample_rate;
  //   const double w_freq    = 2.0 * pi_isr * frequency;          // (Wu+Wl)/2
  //   const double w_bw_half = pi_isr * bandwidth;                // (Wu-Wl)/2

  //   // Wc = sqrt(Wu*Wl) — derived without computing Wu/Wl separately
  //   const double wc        = std::sqrt(w_freq*w_freq - w_bw_half*w_bw_half);

  //   // C = tan(Wc/2) via sincos — avoids tan, consistent with caller convention
  //   double sin_wc2, cos_wc2;
  //   sincos(wc * 0.5, &sin_wc2, &cos_wc2);
  //   const double c         = sin_wc2 / cos_wc2;

  //   // K and alpha share sin/cos of w_bw_half — one sincos replaces two tan/cos calls
  //   double sin_bw, cos_bw;
  //   sincos(w_bw_half, &sin_bw, &cos_bw);
  //   const double k         = c * cos_bw / sin_bw;              // c / tan(w_bw_half)
  //   const double alpha     = std::cos(w_freq) / cos_bw;        // reuses cos_bw

  //   // shared divisor
  //   const double inv_kpc   = 1.0 / (k + c);
  //   const double c_n       = c * inv_kpc;
  //   const double k_n       = k * inv_kpc;

  //   BiquadCoefficients result{};
  //   result.b0 = double_to_5_27(  c_n );
  //   result.b1 = double_to_5_27(  0.0 );
  //   result.b2 = double_to_5_27( -c_n );                        // reuses c_n
  //   result.a1 = double_to_5_27(  2.0 * alpha * k_n );
  //   result.a2 = double_to_5_27(  c_n - k_n );                  // reuses both
  //   return result;

};

BiquadCoefficients notch_filter_(uint32_t sample_rate, uint16_t frequency, uint16_t bandwidth) {
  const double pi_inverse_sample_rate = std::numbers::pi / sample_rate;

  const double w0 = 2.0 * pi_inverse_sample_rate * frequency;
  const double interim = std::tan(pi_inverse_sample_rate * bandwidth);
  const double alpha = (1 - interim) / (1 + interim);
  const double cos_w0 = std::cos(w0);

  const double b0 = (1.0 + alpha) * 0.5;
  const double b1 = -cos_w0 * (1.0 + alpha);

  BiquadCoefficients result{};
  result.b0 = double_to_5_27( b0 );
  result.b1 = double_to_5_27( b1 );
  result.b2 = double_to_5_27( b0 );
  result.a1 = double_to_5_27( -b1 );
  result.a2 = double_to_5_27( -alpha );
  return result;
};

}  // namespace esphome::tas58xx_helpers

