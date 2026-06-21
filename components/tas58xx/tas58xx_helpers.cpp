#include "tas58xx_helpers.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx_helpers {
  // double precision used so coefficent values match well with TI Pure Path Console 3
  // whereas using float precision does not give close matches
  // speed optimisations are utilised where possible since double precision calculations on esp32 are much slower than float

  static constexpr const char* HELPER_TAG = "tas58xx.helper";

  constexpr double TWO_PI = 2.0 * std::numbers::pi;

  // used in low and high pass fileters where Q is fixed = 1 / sqrt(2)
  constexpr double INVERSE_SQRT2 = 0.7071067811865475244;

  // used in exp calculations which replaces pow function calls
  constexpr double LN10_DIV_20 = 0.11512925464970228420;  // ln(10) / 20
  constexpr double LN10_DIV_40 = 0.05756462732485114210;  // ln(10) / 40
  constexpr double LN10_DIV_80 = 0.02878231366242557105;  // ln(10) / 80
  // Auto-generated gain lookup table
// Range: -24dB to +24dB (49 entries)
// Values match JavaScript: Math.sqrt(Math.pow(10, gain/20)) exactly
// Indexed by gain + 24 to map [-24, +24] -> [0, 48]
// Memory: 49 x 16 bytes = 784 bytes (flash/.rodata)

struct GainCoeffs {
    double a;       // = sqrt(pow(10, gain/20))
    double sqrt_a;  // = sqrt(A)
};

static constexpr GainCoeffs GAIN_TABLE[49] = {
    { 2.51188643150958013e-01, 5.01187233627272244e-01 },  // gain = -24 dB  [index  0]
    { 2.66072505979880969e-01, 5.15822165072305716e-01 },  // gain = -23 dB  [index  1]
    { 2.81838293126445372e-01, 5.30884444230988350e-01 },  // gain = -22 dB  [index  2]
    { 2.98538261891795942e-01, 5.46386549881854200e-01 },  // gain = -21 dB  [index  3]
    { 3.16227766016837941e-01, 5.62341325190349073e-01 },  // gain = -20 dB  [index  4]
    { 3.34965439157827671e-01, 5.78761988349120626e-01 },  // gain = -19 dB  [index  5]
    { 3.54813389233575471e-01, 5.95662143529010479e-01 },  // gain = -18 dB  [index  6]
    { 3.75837404288444199e-01, 6.13055792149820755e-01 },  // gain = -17 dB  [index  7]
    { 3.98107170553497258e-01, 6.30957344480193250e-01 },  // gain = -16 dB  [index  8]
    { 4.21696503428582281e-01, 6.49381631576211316e-01 },  // gain = -15 dB  [index  9]
    { 4.46683592150963149e-01, 6.68343917568614665e-01 },  // gain = -14 dB  [index 10]
    { 4.73151258961480470e-01, 6.87859912308807608e-01 },  // gain = -13 dB  [index 11]
    { 5.01187233627272244e-01, 7.07945784384137911e-01 },  // gain = -12 dB  [index 12]
    { 5.30884444230988350e-01, 7.28618174513227745e-01 },  // gain = -11 dB  [index 13]
    { 5.62341325190349073e-01, 7.49894209332455874e-01 },  // gain = -10 dB  [index 14]
    { 5.95662143529010479e-01, 7.71791515585012466e-01 },  // gain =  -9 dB  [index 15]
    { 6.30957344480193250e-01, 7.94328234724281490e-01 },  // gain =  -8 dB  [index 16]
    { 6.68343917568614665e-01, 8.17523037943649999e-01 },  // gain =  -7 dB  [index 17]
    { 7.07945784384137911e-01, 8.41395141645195133e-01 },  // gain =  -6 dB  [index 18]
    { 7.49894209332455874e-01, 8.65964323360065347e-01 },  // gain =  -5 dB  [index 19]
    { 7.94328234724281490e-01, 8.91250938133745563e-01 },  // gain =  -4 dB  [index 20]
    { 8.41395141645195133e-01, 9.17275935389779584e-01 },  // gain =  -3 dB  [index 21]
    { 8.91250938133745563e-01, 9.44060876285923389e-01 },  // gain =  -2 dB  [index 22]
    { 9.44060876285923389e-01, 9.71627951577106130e-01 },  // gain =  -1 dB  [index 23]
    { 1.00000000000000000e+00, 1.00000000000000000e+00 },  // gain =  +0 dB  [index 24]
    { 1.05925372517728889e+00, 1.02920052719442823e+00 },  // gain =  +1 dB  [index 25]
    { 1.12201845430196356e+00, 1.05925372517728889e+00 },  // gain =  +2 dB  [index 26]
    { 1.18850222743701850e+00, 1.09018449238512760e+00 },  // gain =  +3 dB  [index 27]
    { 1.25892541179416728e+00, 1.12201845430196356e+00 },  // gain =  +4 dB  [index 28]
    { 1.33352143216332397e+00, 1.15478198468945825e+00 },  // gain =  +5 dB  [index 29]
    { 1.41253754462275438e+00, 1.18850222743701850e+00 },  // gain =  +6 dB  [index 30]
    { 1.49623565609443343e+00, 1.22320711904993162e+00 },  // gain =  +7 dB  [index 31]
    { 1.58489319246111360e+00, 1.25892541179416728e+00 },  // gain =  +8 dB  [index 32]
    { 1.67880401812256030e+00, 1.29568669751701937e+00 },  // gain =  +9 dB  [index 33]
    { 1.77827941003892276e+00, 1.33352143216332397e+00 },  // gain = +10 dB  [index 34]
    { 1.88364908948980059e+00, 1.37246096100756199e+00 },  // gain = +11 dB  [index 35]
    { 1.99526231496887951e+00, 1.41253754462275438e+00 },  // gain = +12 dB  [index 36]
    { 2.11348903983664682e+00, 1.45378438560766177e+00 },  // gain = +13 dB  [index 37]
    { 2.23872113856833943e+00, 1.49623565609443343e+00 },  // gain = +14 dB  [index 38]
    { 2.37137370566165551e+00, 1.53992652605949210e+00 },  // gain = +15 dB  [index 39]
    { 2.51188643150958013e+00, 1.58489319246111360e+00 },  // gain = +16 dB  [index 40]
    { 2.66072505979880969e+00, 1.63117290922783842e+00 },  // gain = +17 dB  [index 41]
    { 2.81838293126445416e+00, 1.67880401812256053e+00 },  // gain = +18 dB  [index 42]
    { 2.98538261891795953e+00, 1.72782598050786329e+00 },  // gain = +19 dB  [index 43]
    { 3.16227766016837952e+00, 1.77827941003892276e+00 },  // gain = +20 dB  [index 44]
    { 3.34965439157827705e+00, 1.83020610631105618e+00 },  // gain = +21 dB  [index 45]
    { 3.54813389233575505e+00, 1.88364908948980059e+00 },  // gain = +22 dB  [index 46]
    { 3.75837404288444121e+00, 1.93865263595220716e+00 },  // gain = +23 dB  [index 47]
    { 3.98107170553497225e+00, 1.99526231496887951e+00 }   // gain = +24 dB  [index 48]
};

// // Accessor
// // Asserts on out-of-range gain in debug builds; clamps in release
// inline void gain_coeffs(int8_t gain, double& A, double& sqrt_A) {
//     assert(gain >= -24 && gain <= 24);
//     const int index     = static_cast<int>(gain) + 24;
//     const GainCoeffs& g = GAIN_TABLE[index];
//     A      = g.A;
//     sqrt_A = g.sqrt_A;
// }

  int32_t gain_to_f9_23_(int8_t gain) {
    static constexpr uint8_t FRACTIONAL_BITS = 23;
    static constexpr uint32_t SCALE = 1u << FRACTIONAL_BITS;

    // valid 9.23 range
    static constexpr float MAX_VALUE =  256.0f - 1.0 / SCALE;
    static constexpr float MIN_VALUE = -256.0f;

    float linear = powf(10.0f, (gain) / 20.0f);
    //float linear = std::exp(gain * LN10_DIV_20);

    if (linear > MAX_VALUE) linear = MAX_VALUE;
    if (linear < MIN_VALUE) linear = MIN_VALUE;

    // scale to fixed 9.23
    int32_t fixed_9_23 = static_cast<int32_t>(linear * static_cast<float>(SCALE));

    // convert to 32 bit little endian
    int32_t little_endian = byteswap(fixed_9_23);

    ESP_LOGD(HELPER_TAG, "Gain:%ddb >> Fixed 9.23: 0x%08X  Little Endian: 0x%08X", gain, fixed_9_23, little_endian);
    return little_endian;
  }

  inline int32_t double_to_5_27(double x) {
    static constexpr uint8_t FRACTIONAL_BITS = 27;
    static constexpr int64_t SCALE = 1LL << FRACTIONAL_BITS;

    // valid 5.27 range
    static constexpr double MAX_VALUE =  256.0 - (1.0 / SCALE);
    static constexpr double MIN_VALUE = -256.0;

    // clamp to valid 5.27 range
    if (x > MAX_VALUE) x = MAX_VALUE;
    if (x < MIN_VALUE) x = MIN_VALUE;

    // scale to fixed 5.27
    const double scaled =  x * static_cast<double>(SCALE);

    // saturate to 32 bit
    int64_t long_fixed_5_27 = static_cast<int64_t>(std::round(scaled));
    if (long_fixed_5_27 >  INT_MAX) long_fixed_5_27 =  INT_MAX;
    if (long_fixed_5_27 <  INT_MIN) long_fixed_5_27 =  INT_MIN;

    const int32_t fixed_5_27 = static_cast<int32_t>(long_fixed_5_27);

    // convert to 32 bit little endian
    const int32_t little_endian = byteswap(fixed_5_27);

    ESP_LOGD(HELPER_TAG, "Biquad Coefficient >> Raw Double: %.16f  Fixed 5.27: 0x%08X  Little Endian: 0x%08X", x, fixed_5_27, little_endian);
    return little_endian;
  }

  BiquadCoefficients equalizer_qfactor_(uint32_t sample_rate, uint16_t frequency, int8_t gain, float q_factor) {
    // derived from biquad.model.js

    // originally A = pow(10, gain / 20)
    // pow(10, gain / 20) <=> exp(gain * (ln(10) / 20)
    const double ag = std::exp(gain * LN10_DIV_20);

    const double t0 = TWO_PI * frequency / sample_rate;
    const float q_factor_x2 = 2.0f * q_factor;

    double beta;
    // original ag >= 1.0 <=> gain >= 0
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

  BiquadCoefficients low_shelf_filter_(uint32_t sample_rate, uint16_t frequency, int8_t gain, float q_factor) {
    // derived from biquad.model.js

    // A = sqrt(pow(10, (gain / 20)) = pow(10, (gain / 40)) = exp(ln(10) * gain / 40)
    // sqrt(a) = pow(10, gain / 80) <=> exp(gain * ln(10) / 80)
    // calculating ag using multiplication sqrt_ag * sqrt_ag eliminates sqrt here and in "beta" calculation
    // const double sqrt_ag = std::exp(gain * LN10_DIV_80);
    // const double ag = sqrt_ag * sqrt_ag;
    const int index = static_cast<int>(gain) + 24;
    const GainCoeffs &gain_coeff = GAIN_TABLE[index];
    const double ag = gain_coeff.a;
    const double sqrt_ag = gain_coeff.sqrt_a;

    // used multple times - precompute once
    const double ag_plus1 = ag + 1.0;
    const double ag_minus1 = ag - 1.0;

    // Half-angle approach suggested by Claude to reduce calculation errors at higher frequencies
    // cosw0-dependent terms are rewritten using sin²(w0/2) and cos²(w0/2)
    // to eliminate where cos(w0) approaches -1 and large nearly-equal values cancel, losing precision
    const double half_w0 = std::numbers::pi * static_cast<double>(frequency) / static_cast<double>(sample_rate);
    double sin_half, cos_half;
    sincos(half_w0, &sin_half, &cos_half);
    const double sin2_half = sin_half * sin_half;
    const double cos2_half = cos_half * cos_half;

    // sin(w0) = 2·sin(w0/2)·cos(w0/2)
    const double sinw0 = 2.0 * sin_half * cos_half;

    // used multple times - precompute once
    // stable replacements using half-angle variables
    const double precalc_x = 2.0 * (ag * cos2_half + sin2_half);
    const double precalc_y = 2.0 * (ag * sin2_half + cos2_half);

    // originally
    // alpha = sin(w0) / (2 * q_factor);
    // beta = 2 * sqrt(a) * sin(w0) / (2 * q_factor);
    const double beta = sqrt_ag * sinw0 /static_cast<double>(q_factor); // simplified

    // multiply is faster than divide
    // a0 (denominator) = precalc_x + beta
    const double inverse_a0 = 1.0 / (precalc_x + beta);

    // shared multipliers — precompute once across b0, b1, b2
    const double ag_inv = ag * inverse_a0;       // saves recomputing 3× across b0, b1, b2
    const double ag_inv_y = ag_inv * precalc_y;  // shared between b0 and b2
    const double ag_inv_beta = ag_inv * beta;    // shared between b0 and b2

    BiquadCoefficients result{};
    // stable replacements using half-angle variables
    // b1 replace ag_minus1 - ag_plus1 * cos(w0) with more stable 2.0 * (ag_plus1 * sin²(w0/2) - 1)
    // a1 replace ag_minus1 + ag_plus1 * cos(w0) with more stable 2.0 * (ag - ag_plus1 * sin²(w0/2))
    result.b0 = double_to_5_27( ag_inv_y + ag_inv_beta );
    result.b1 = double_to_5_27( 4.0 * ag_inv * (ag_plus1 * sin2_half - 1.0) );
    result.b2 = double_to_5_27( ag_inv_y - ag_inv_beta );
    result.a1 = double_to_5_27( 4.0 * (ag - ag_plus1 * sin2_half) * inverse_a0 );
    result.a2 = double_to_5_27( (beta - precalc_x) * inverse_a0 );
    return result;
};

BiquadCoefficients high_shelf_filter_(uint32_t sample_rate, uint16_t frequency, int8_t gain, float q_factor) {
    // derived from biquad.model.js

    // A = sqrt(pow(10, (gain / 20)) = pow(10, (gain / 40)) = exp(ln(10) * gain / 40)
    // sqrt(a) = pow(10, gain / 80) <=> exp(gain * ln(10) / 80)
    // calculating ag using multiplication sqrt_ag * sqrt_ag eliminates sqrt here and in "beta" calculation
    const double sqrt_ag = std::exp(gain * LN10_DIV_80);
    const double ag = sqrt_ag * sqrt_ag;

    // used multple times - precompute once
    const double ag_plus1 = ag + 1.0;
    const double ag_minus1 = ag - 1.0;

    const double w0 = TWO_PI * frequency / sample_rate;
    double sinw0, cosw0;
    sincos(w0, &sinw0, &cosw0);

    // used multple times - precompute once
    const double ag_plus1_cosw0 = ag_plus1 * cosw0;
    const double ag_minus1_cosw0 = ag_minus1 * cosw0;

    // originally
    // alpha = sin(w0) / (2.0 * q_factor);
    // beta = 2.0 * sqrt(A) * sin(w0) / (2.0 * q_factor);
    const double beta = sqrt_ag * sinw0 / q_factor; // simplified

    const double precalc_x = ag_plus1 + ag_minus1_cosw0;
    const double precalc_y = ag_plus1 - ag_minus1_cosw0;

    // multiply is faster than divide
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

BiquadCoefficients low_pass_filter_(uint32_t sample_rate, uint16_t frequency, int8_t gain) {
// derived from Cookbook formulae for audio EQ biquad filter coefficients by Robert Bristow-Johnson
// easier to optimise
// gives same coefficient values as low pass butterworth 2 filter in TI Pure Path Console 3

  // originally A = pow(10, gain / 20))
  // pow(10, gain / 20) <=> exp(gain * (ln(10) / 20)
  const double ag = std::exp(gain * LN10_DIV_20);

  // w0 = 2 * pi * f0 / Fs
  const double w0 = TWO_PI * frequency / sample_rate;
  double sin_w0, cos_w0;
  sincos(w0, &sin_w0, &cos_w0);

  // Q = 1 / sqrt(2)
  // alpha = sin(w0) / (2 * Q) <=> sin_w0 * sqrt(2) / 2 <=> sin_w0 / sqrt(2)
  const double alpha = sin_w0 * INVERSE_SQRT2;

  // multiply is faster than divide
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

BiquadCoefficients high_pass_filter_(uint32_t sample_rate, uint16_t frequency, int8_t gain) {
// derived from Cookbook formulae for audio EQ biquad filter coefficients by Robert Bristow-Johnson
// easier to optimise
// gives same coefficient values as low pass butterworth 2 filter in TI Pure Path Console 3

  // originally A = pow(10, gain / 20))
  // pow(10, gain / 20) <=> exp(gain * (ln(10) / 20)
  const double ag = std::exp(gain * LN10_DIV_20);

  // w0 = 2 * pi * f0 / Fs
  const double w0 = TWO_PI * frequency / sample_rate;
  double sin_w0, cos_w0;
  sincos(w0, &sin_w0, &cos_w0);

  // Q = 1 / sqrt(2)
  // alpha = sin(w0) / (2 * Q) <=> sin_w0 * sqrt(2) / 2 <=> sin_w0 / sqrt(2)
  const double alpha = sin_w0 * INVERSE_SQRT2;

  // multiply is faster than divide
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

BiquadCoefficients peaking_eq_(uint32_t sample_rate, uint16_t frequency, int8_t gain, float q_factor) {
  // derived from biquad.model.js

  // A = sqrt(pow(10, (gain / 20)) = pow(10, (gain / 40)) = exp(ln(10) * gain / 40)
  const double ag = std::exp(gain * LN10_DIV_40);

  const double w0 = TWO_PI * frequency / sample_rate;
  double sin_w0, cos_w0;
  sincos(w0, &sin_w0, &cos_w0);

  // use multiple times - precompute once
  const double alpha = sin_w0 / (2.0 * q_factor);
  const double alpha_divide_ag = alpha / ag;

  // a0 = 1 + alpha / A
  // multiply is faster than divide
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
  // derived from biquad.model.js

  const double pi_inverse_sample_rate = std::numbers::pi / sample_rate;
  const double wf = 2.0 * pi_inverse_sample_rate * frequency;   // (Wu+Wl)/2
  const double wb_half = pi_inverse_sample_rate * bandwidth;    // (Wu-Wl)/2
  const double wc = std::sqrt((wf * wf) - (wb_half * wb_half)); // Wc = sqrt(Wu*Wl) — without computing Wu/Wl separately

  // C = tan(Wc/2) via sincos - avoids tan
  double sin_wch, cos_wch;
  sincos(wc * 0.5, &sin_wch, &cos_wch);
  const double c = cos_wch / cos_wch;

  double sin_bw, cos_bw;
  sincos(wb_half, &sin_bw, &cos_bw);

  // k and alpha share sin/cos of wb_half - one sincos replaces two tan calls
  const double k = c * cos_bw / sin_bw;                         // c / tan(wb)

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

};

BiquadCoefficients notch_filter_(uint32_t sample_rate, uint16_t frequency, uint16_t bandwidth) {
  // derived from biquad.model.js

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

