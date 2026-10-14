#pragma once

#include "tas58xx_defs.h"
#include "tas58xx_helpers.h"

namespace esphome::tas58xx {

static constexpr const uint16_t EQ_BAND_FREQUENCY[NUMBER_EQ_BANDS] = {20, 32, 50, 80, 125, 200, 315, 500, 800, 1250, 2000, 3150, 5000, 8000, 16000};
//static constexpr const uint16_t EQ_BAND_FREQUENCY[NUMBER_EQ_BANDS] = {25, 40, 63, 100, 160, 250, 400, 630, 1000, 1600, 2500, 4000, 6300, 10000, 16000};
static constexpr const float EQ_BAND_QFACTOR[NUMBER_EQ_BANDS] = {2.0, 2.0, 1.5, 1.5, 1.0, 1.0, 0.9, 0.9, 0.8, 0.8, 0.7, 0.7, 0.6, 0.6, 0.5};
//static constexpr const float EQ_BAND_QFACTOR[NUMBER_EQ_BANDS] = {2.144908, 2.0, 1.5, 1.5, 1.0, 1.0, 0.9, 0.9, 0.8, 0.8, 0.7, 0.7, 0.6, 0.6, 0.5};

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
// Off=Bypass EQ On + Ganged On; EQ 15 Band=Bypass EQ Off + Ganged On; EQ BIAMP 15 Band and EQ Presets=Bypass EQ Off + Ganged Off
#ifdef USE_TAS5805M_DAC
static constexpr uint8_t TAS5805M_DSP_MISC = 0x66;
static constexpr uint8_t TAS5805M_CTRL_EQ[NUMBER_EQ_MODES] = {0b0111, 0b0110, 0b1110, 0b1110};
#else
static constexpr uint8_t TAS5825M_EQ_MODE_CTRL_PAGE = 0x0B;
static constexpr uint8_t TAS5825M_GANG_EQ = 0x28; // TAS5825M_BYPASS_EQ = 0x2C;

struct EqModeCoefficients {
  uint32_t gang_eq;
  uint32_t bypass_eq;
}__attribute__((packed));

// 0x00000001 EQ Ganged ie L/R channel common coefficients
// 0x00000000 Bypass EQ = false ie EQ enabled
static constexpr EqModeCoefficients TAS5825M_CTRL_EQ[NUMBER_EQ_MODES] = {
  {0x00000001, 0x00000001}, // EQ Off     = Ganged true  + Bypass EQ true
  {0x00000001, 0x00000000}, // EQ 15 Band = Ganged true  + Bypass EQ false
  {0x00000000, 0x00000000}, // EQ BIAMP   = Ganged false + Bypass EQ false
  {0x00000000, 0x00000000}, // EQ Presets = Ganged false + Bypass EQ false
};
#endif

#if defined(USE_TAS58XX_EQ_GAINS) || defined(USE_TAS58XX_EQ_PRESETS)
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
