#pragma once

#include "tas58xx_defs.h"
#include "tas58xx_helpers.h"

namespace esphome::tas58xx {

// The filter of each EQ gain number band. The gain is set by the number. The hub uses a constant table for each
// channel, which can be replaced (Tas58xxComponent::config_eq_bands) by a table generated from YAML.
struct EqBand {
  tas58xx_helpers::FilterType type;
  uint16_t frequency;
  float q_factor;
};

static constexpr EqBand EQ_DEFAULT_BANDS[NUMBER_EQ_BANDS] = {
  { tas58xx_helpers::FILTER_TYPE_EQUALIZER, 20,    2.0f },
  { tas58xx_helpers::FILTER_TYPE_EQUALIZER, 32,    2.0f },
  { tas58xx_helpers::FILTER_TYPE_EQUALIZER, 50,    1.5f },
  { tas58xx_helpers::FILTER_TYPE_EQUALIZER, 80,    1.5f },
  { tas58xx_helpers::FILTER_TYPE_EQUALIZER, 125,   1.0f },
  { tas58xx_helpers::FILTER_TYPE_EQUALIZER, 200,   1.0f },
  { tas58xx_helpers::FILTER_TYPE_EQUALIZER, 315,   0.9f },
  { tas58xx_helpers::FILTER_TYPE_EQUALIZER, 500,   0.9f },
  { tas58xx_helpers::FILTER_TYPE_EQUALIZER, 800,   0.8f },
  { tas58xx_helpers::FILTER_TYPE_EQUALIZER, 1250,  0.8f },
  { tas58xx_helpers::FILTER_TYPE_EQUALIZER, 2000,  0.7f },
  { tas58xx_helpers::FILTER_TYPE_EQUALIZER, 3150,  0.7f },
  { tas58xx_helpers::FILTER_TYPE_EQUALIZER, 5000,  0.6f },
  { tas58xx_helpers::FILTER_TYPE_EQUALIZER, 8000,  0.6f },
  { tas58xx_helpers::FILTER_TYPE_EQUALIZER, 16000, 0.5f },
};
// alternative frequencies {25, 40, 63, 100, 160, 250, 400, 630, 1000, 1600, 2500, 4000, 6300, 10000, 16000}
// alternative q factors {2.144908, 2.0, 1.5, 1.5, 1.0, 1.0, 0.9, 0.9, 0.8, 0.8, 0.7, 0.7, 0.6, 0.6, 0.5}

// EQ Gain constants
static constexpr int8_t  TAS58XX_EQ_MAX_DB = 15;
static constexpr int8_t  TAS58XX_EQ_MIN_DB = -TAS58XX_EQ_MAX_DB;

// biquad constants
static constexpr uint8_t COEFFICIENT_SIZE = 4;                // cooefficients = 32 bit
static constexpr uint8_t BIQUAD_SIZE = COEFFICIENT_SIZE * 5;  // 5 cooefficients per biquad ;

// sample rate the DSP processes at, used to calculate the biquad coefficients
static constexpr uint32_t EQ_SAMPLE_RATE = 96000;

#ifdef USE_TAS58XX_EQ_PRESETS
// EQ presets, selected by index: 0 = Flat, 1 to 10 = LF 60Hz to 150Hz, 11 to 20 = HF 60Hz to 150Hz
// 3 biquads are used, the first 2 are the same filter and together make a 4th order filter
//   BQ1 and BQ2: low pass (LF presets) or high pass (HF presets) filter at the cutoff frequency
//   BQ3: high shelf at shelf_frequency, to make up the level lost with the high pass filter (HF presets)
//        all pass when shelf_frequency is 0 (Flat and LF presets)
static constexpr uint8_t EQ_PROFILE_MAXIMUM_INDEX = 20;
static constexpr uint8_t EQ_PROFILE_BIQUADS = 3;
static constexpr int8_t  EQ_PRESET_SHELF_GAIN = 1;
static constexpr float   EQ_PRESET_SHELF_QFACTOR = 0.5f;

struct EqPreset {
  tas58xx_helpers::FilterType filter;
  uint16_t cutoff_frequency;
  uint16_t shelf_frequency;
};

static constexpr EqPreset EQ_PRESETS[EQ_PROFILE_MAXIMUM_INDEX + 1] = {
  { tas58xx_helpers::FILTER_TYPE_ALL_PASS,  0,   0 },    // Flat
  { tas58xx_helpers::FILTER_TYPE_LOW_PASS,  60,  0 },    // LF 60Hz
  { tas58xx_helpers::FILTER_TYPE_LOW_PASS,  70,  0 },    // LF 70Hz
  { tas58xx_helpers::FILTER_TYPE_LOW_PASS,  80,  0 },    // LF 80Hz
  { tas58xx_helpers::FILTER_TYPE_LOW_PASS,  90,  0 },    // LF 90Hz
  { tas58xx_helpers::FILTER_TYPE_LOW_PASS,  100, 0 },    // LF 100Hz
  { tas58xx_helpers::FILTER_TYPE_LOW_PASS,  110, 0 },    // LF 110Hz
  { tas58xx_helpers::FILTER_TYPE_LOW_PASS,  120, 0 },    // LF 120Hz
  { tas58xx_helpers::FILTER_TYPE_LOW_PASS,  130, 0 },    // LF 130Hz
  { tas58xx_helpers::FILTER_TYPE_LOW_PASS,  140, 0 },    // LF 140Hz
  { tas58xx_helpers::FILTER_TYPE_LOW_PASS,  150, 0 },    // LF 150Hz
  { tas58xx_helpers::FILTER_TYPE_HIGH_PASS, 60,  200 },  // HF 60Hz
  { tas58xx_helpers::FILTER_TYPE_HIGH_PASS, 70,  200 },  // HF 70Hz
  { tas58xx_helpers::FILTER_TYPE_HIGH_PASS, 80,  200 },  // HF 80Hz
  { tas58xx_helpers::FILTER_TYPE_HIGH_PASS, 90,  200 },  // HF 90Hz
  { tas58xx_helpers::FILTER_TYPE_HIGH_PASS, 100, 200 },  // HF 100Hz
  { tas58xx_helpers::FILTER_TYPE_HIGH_PASS, 110, 220 },  // HF 110Hz
  { tas58xx_helpers::FILTER_TYPE_HIGH_PASS, 120, 240 },  // HF 120Hz
  { tas58xx_helpers::FILTER_TYPE_HIGH_PASS, 130, 260 },  // HF 130Hz
  { tas58xx_helpers::FILTER_TYPE_HIGH_PASS, 140, 280 },  // HF 140Hz
  { tas58xx_helpers::FILTER_TYPE_HIGH_PASS, 150, 300 },  // HF 150Hz
};
#endif

// EQ Modes registers and coefficients
static constexpr uint8_t TAS58XX_EQ_CTRL_BOOK = 0xAA;

// EQ mode coefficients equivalent to the indexes of the eq_modes
// EQ Off = bypass EQ; EQ On = ganged (left filters used by both channels)
// EQ Left On, EQ Right On and EQ BIAMP On = not ganged, the same registers, the filters of a channel that is not
// used are set flat by the software
#ifdef USE_TAS5805M_DAC
static constexpr uint8_t TAS5805M_DSP_MISC = 0x66;
static constexpr uint8_t TAS5805M_CTRL_EQ[NUMBER_EQ_MODES] = {0b0111, 0b0110, 0b1110, 0b1110, 0b1110};
#else
static constexpr uint8_t TAS5825M_EQ_MODE_CTRL_PAGE = 0x0B;
static constexpr uint8_t TAS5825M_GANG_EQ = 0x28; // TAS5825M_BYPASS_EQ = 0x2C;

struct EqModeCoefficients {
  uint32_t gang_eq;
  uint32_t bypass_eq;
}__attribute__((packed));

// Gang EQ and Bypass EQ are 32.0 flags, 1 = true. The DSP is big endian, so 0x00000001 is stored byte swapped
// (0x01000000) to be sent as 00 00 00 01, the same as the other DSP values (eg TAS58XX_MIXER_COEFF_0DB)
static constexpr uint32_t TAS5825M_EQ_FLAG_TRUE = 0x01000000;
static constexpr uint32_t TAS5825M_EQ_FLAG_FALSE = 0x00000000;

static constexpr EqModeCoefficients TAS5825M_CTRL_EQ[NUMBER_EQ_MODES] = {
  {TAS5825M_EQ_FLAG_TRUE,  TAS5825M_EQ_FLAG_TRUE},  // EQ Off      = Ganged true  + Bypass EQ true
  {TAS5825M_EQ_FLAG_TRUE,  TAS5825M_EQ_FLAG_FALSE}, // EQ On       = Ganged true  + Bypass EQ false
  {TAS5825M_EQ_FLAG_FALSE, TAS5825M_EQ_FLAG_FALSE}, // EQ Left On  = Ganged false + Bypass EQ false
  {TAS5825M_EQ_FLAG_FALSE, TAS5825M_EQ_FLAG_FALSE}, // EQ Right On = Ganged false + Bypass EQ false
  {TAS5825M_EQ_FLAG_FALSE, TAS5825M_EQ_FLAG_FALSE}, // EQ BIAMP On = Ganged false + Bypass EQ false
};
#endif

#if defined(USE_TAS58XX_EQ_GAINS) || defined(USE_TAS58XX_EQ_PRESETS)
// the registers gang the EQ (right channel uses the left channel filters) for EQ Off and EQ On Left=Right
constexpr bool eq_mode_is_ganged(EqMode mode) { return mode == EQ_OFF || mode == EQ_ON; }

// what an EQ mode needs in the biquads of a channel
enum EqChannelNeed : uint8_t {
  EQ_CHANNEL_ANY = 0,  // not used by the EQ mode, left as it is
  EQ_CHANNEL_FILTERS,  // the gains or preset of the channel
  EQ_CHANNEL_FLAT,     // flat, so the channel is not filtered
};

constexpr EqChannelNeed eq_channel_need(EqMode mode, Channels channel) {
  switch (mode) {
    case EQ_ON:
      return channel == LEFT_CHANNEL ? EQ_CHANNEL_FILTERS : EQ_CHANNEL_ANY;  // ganged, right uses left
    case EQ_LEFT_ON:
      return channel == LEFT_CHANNEL ? EQ_CHANNEL_FILTERS : EQ_CHANNEL_FLAT;
    case EQ_RIGHT_ON:
      return channel == RIGHT_CHANNEL ? EQ_CHANNEL_FILTERS : EQ_CHANNEL_FLAT;
    case EQ_BIAMP_ON:
      return EQ_CHANNEL_FILTERS;
    default:
      return EQ_CHANNEL_ANY;  // EQ Off, bypassed
  }
}

// page and subaddress of a biquad
struct AddressSequence {
  uint8_t page;
  uint8_t sub_addr;
};

// The 15 left biquads are followed by the 15 right biquads in the DSP memory, each biquad is 20 bytes.
// A page holds 0x78 bytes from subaddress 0x08, so a biquad can continue on the next page.
#ifdef USE_TAS5805M_DAC
static constexpr AddressSequence EQ_FIRST_BIQUAD = { 0x24, 0x18 };
#else
static constexpr AddressSequence EQ_FIRST_BIQUAD = { 0x01, 0x30 };
#endif

constexpr AddressSequence eq_biquad_address(Channels channel, uint8_t biquad_index) {
  constexpr uint8_t PAGE_FIRST_SUBADDRESS = 0x08;
  constexpr uint8_t PAGE_BYTES = 0x78;
  const uint16_t offset = (EQ_FIRST_BIQUAD.sub_addr - PAGE_FIRST_SUBADDRESS) +
                          (channel * NUMBER_EQ_BANDS + biquad_index) * BIQUAD_SIZE;
  return { static_cast<uint8_t>(EQ_FIRST_BIQUAD.page + offset / PAGE_BYTES),
           static_cast<uint8_t>(PAGE_FIRST_SUBADDRESS + offset % PAGE_BYTES) };
}
#endif

}  // namespace esphome::tas58xx
