#pragma once

namespace esphome::tas58xx {

enum Channels : uint8_t {
  LEFT_CHANNEL  = 0,
  RIGHT_CHANNEL,
};

enum EqBandIndices : uint8_t {
  BAND_20HZ   = 0,
  BAND_31_5HZ = 1,
  BAND_50HZ   = 2,
  BAND_80HZ   = 3,
  BAND_125HZ  = 4,
  BAND_200HZ  = 5,
  BAND_315HZ  = 6,
  BAND_500HZ  = 7,
  BAND_800HZ  = 8,
  BAND_1250HZ = 9,
  BAND_2000HZ = 10,
  BAND_3150HZ = 11,
  BAND_5000HZ = 12,
  BAND_8000HZ = 13,
  BAND_16000HZ= 14,
};

enum EqMode : uint8_t {
  EQ_OFF        = 0,
  EQ_ON         = 1,
  EQ_15BAND_ON  = 1,
  EQ_BIAMP_ON   = 2,
  EQ_PRESETS_ON = 3,
};

enum MixerMode : uint8_t {
  STEREO = 0,
  STEREO_INVERSE,
  MONO,
  RIGHT,
  LEFT,
};

static constexpr uint8_t NUMBER_CHANNELS = 2;
static constexpr uint8_t NUMBER_EQ_BANDS = 15;
static constexpr uint8_t NUMBER_EQ_MODES = 4;

static constexpr const char* EQ_MODE_TEXT[NUMBER_EQ_MODES]   = {"Off", "EQ 15 Band", "EQ BIAMP 15 Band", "EQ Presets"};
static constexpr const char* LR_CHANNEL_TEXT[NUMBER_CHANNELS] = {"Left", "Right"};
static constexpr const char* MIXER_MODE_TEXT[] = {"STEREO", "STEREO_INVERSE", "MONO", "RIGHT", "LEFT"};

// EQ Gain constants
static constexpr int8_t  TAS58XX_EQ_MAX_DB = 15;
static constexpr int8_t  TAS58XX_EQ_MIN_DB = -TAS58XX_EQ_MAX_DB;

// biquad constants
static constexpr uint8_t COEFFICIENT_SIZE = 4;                // cooefficients = 32 bit
static constexpr uint8_t BIQUAD_SIZE = COEFFICIENT_SIZE * 5;  // 5 cooefficients per biquad ;

// biquad coefficient structure used by eq_bands.h and eq_presets.h
struct BiquadSequence {
	uint8_t coefficients[BIQUAD_SIZE];
}__attribute__((packed));

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

// channel volumes registers and coefficients
static constexpr int8_t TAS58XX_CHANNEL_VOLUME_MAX_DB = 24;
static constexpr int8_t TAS58XX_CHANNEL_VOLUME_MIN_DB = -TAS58XX_CHANNEL_VOLUME_MAX_DB;

static constexpr uint8_t TAS58XX_AUDIO_CTRL_BOOK = 0x8C;

#ifdef USE_TAS5805M_DAC
// TAS5805M
static constexpr uint8_t TAS58XX_MIXER_GAIN_PAGE = 0x29;
static constexpr uint8_t TAS58XX_MIXER_GAIN_SUBADDR = 0x18; // Left to Left = 0x18, Right to Left = 0x1c, Left to Right = 0x20, Right to Right = 0x24
static constexpr uint8_t TAS58XX_CHANNEL_VOLUME_PAGE = 0x2A;
static constexpr uint8_t TAS58XX_CHANNEL_VOLUME_SUBADDR[NUMBER_CHANNELS] = {0x24 , 0x28};  // Left channel = 0x24, Right Channel = 0x28
#else
// TAS5825M
static constexpr uint8_t TAS58XX_MIXER_GAIN_PAGE = 0x0B;
static constexpr uint8_t TAS58XX_MIXER_GAIN_SUBADDR = 0x14; // Left to Left = 0x14, Right to Left = 0x18, Left to Right = 0x1c, Right to Right = 0x20
static constexpr uint8_t TAS58XX_CHANNEL_VOLUME_PAGE = 0x0B;
static constexpr uint8_t TAS58XX_CHANNEL_VOLUME_SUBADDR[NUMBER_CHANNELS] = {0x0c, 0x10};  // Left channel = 0x0c, Right Channel = 0x10
#endif

// mixer gain coefficients converted to little endian
static constexpr uint32_t TAS58XX_MIXER_COEFF_MUTE = 0x00000000;
static constexpr uint32_t TAS58XX_MIXER_COEFF_0DB = 0x00008000;
static constexpr uint32_t TAS58XX_MIXER_COEFF_MINUS6DB = 0x00004000;

#if defined(USE_TAS58XX_EQ_GAINS) || defined(USE_TAS58XX_EQ_PRESETS)
// Biquad pages and subaddresses
struct AddressSequence {
	uint8_t page;
	uint8_t sub_addr;
}__attribute__((packed));

#ifdef USE_TAS5805M_DAC
  static constexpr AddressSequence TAS5805M_LEFT_EQ_ADDRESS[NUMBER_EQ_BANDS] = {
	    { 0x24, 0x18 }, // BQ1 Left  - Frequency: 20 Hz
	    { 0x24, 0x2c }, // BQ2 Left  - Frequency: 31.5 Hz
	    { 0x24, 0x40 }, // BQ3 Left  - Frequency: 50 Hz
	    { 0x24, 0x54 }, // BQ4 Left  - Frequency: 80 Hz
	    { 0x24, 0x68 }, // BQ5 Left  - Frequency: 125 Hz
	    { 0x24, 0x7c }, // BQ6 Left  - Frequency: 200 Hz
	    { 0x25, 0x18 }, // BQ7 Left  - Frequency: 315 Hz
	    { 0x25, 0x2c }, // BQ8 Left  - Frequency: 500 Hz
	    { 0x25, 0x40 }, // BQ9 Left  - Frequency: 800 Hz
	    { 0x25, 0x54 }, // BQ10 Left - Frequency: 1250 Hz
	    { 0x25, 0x68 }, // BQ11 Left - Frequency: 2000 Hz
	    { 0x25, 0x7c }, // BQ12 Left - Frequency: 3150 Hz
	    { 0x26, 0x18 }, // BQ13 Left - Frequency: 5000 Hz
	    { 0x26, 0x2c }, // BQ14 Left - Frequency: 8000 Hz
	    { 0x26, 0x40 }, // BQ15 Left - Frequency: 16000 Hz
  };
  static constexpr AddressSequence TAS5805M_RIGHT_EQ_ADDRESS[NUMBER_EQ_BANDS] = {
	    { 0x26, 0x54 }, // BQ1 Right  - Frequency: 20 Hz
	    { 0x26, 0x68 }, // BQ2 Right  - Frequency: 31.5 Hz
	    { 0x26, 0x7c }, // BQ3 Right  - Frequency: 50 Hz
	    { 0x27, 0x18 }, // BQ4 Right  - Frequency: 80 Hz
	    { 0x27, 0x2c }, // BQ5 Right  - Frequency: 125 Hz
	    { 0x27, 0x40 }, // BQ6 Right  - Frequency: 200 Hz
	    { 0x27, 0x54 }, // BQ7 Right  - Frequency: 315 Hz
	    { 0x27, 0x68 }, // BQ8 Right  - Frequency: 500 Hz
	    { 0x27, 0x7c }, // BQ9 Right  - Frequency: 800 Hz
	    { 0x28, 0x18 }, // BQ10 Right - Frequency: 1250 Hz
	    { 0x28, 0x2c }, // BQ11 Right - Frequency: 2000 Hz
	    { 0x28, 0x40 }, // BQ12 Right - Frequency: 3150 Hz
	    { 0x28, 0x54 }, // BQ13 Right - Frequency: 5000 Hz
	    { 0x28, 0x68 }, // BQ14 Right - Frequency: 8000 Hz
	    { 0x28, 0x7c }, // BQ15 Right - Frequency: 16000 Hz
  };

#else
	static constexpr AddressSequence TAS5825M_LEFT_EQ_ADDRESS[NUMBER_EQ_BANDS] = {
	    { 0x01, 0x30 }, // BQ1 Left  - Frequency: 20 Hz
	    { 0x01, 0x44 }, // BQ2 Left  - Frequency: 31.5 Hz
	    { 0x01, 0x58 }, // BQ3 Left  - Frequency: 50 Hz
	    { 0x01, 0x6c }, // BQ4 Left  - Frequency: 80 Hz
	    { 0x02, 0x08 }, // BQ5 Left  - Frequency: 125 Hz
	    { 0x02, 0x1c }, // BQ6 Left  - Frequency: 200 Hz
	    { 0x02, 0x30 }, // BQ7 Left  - Frequency: 315 Hz
	    { 0x02, 0x44 }, // BQ8 Left  - Frequency: 500 Hz
	    { 0x02, 0x58 }, // BQ9 Left  - Frequency: 800 Hz
	    { 0x02, 0x6c }, // BQ10 Left - Frequency: 1250 Hz
	    { 0x03, 0x08 }, // BQ11 Left - Frequency: 2000 Hz
	    { 0x03, 0x1c }, // BQ12 Left - Frequency: 3150 Hz
	    { 0x03, 0x30 }, // BQ13 Left - Frequency: 5000 Hz
	    { 0x03, 0x44 }, // BQ14 Left - Frequency: 8000 Hz
	    { 0x03, 0x58 }, // BQ15 Left - Frequency: 16000 Hz
  };

	static constexpr AddressSequence TAS5825M_RIGHT_EQ_ADDRESS[NUMBER_EQ_BANDS] = {
		{ 0x03, 0x6c }, // BQ1 Right  - Frequency: 20 Hz
		{ 0x04, 0x08 }, // BQ2 Right  - Frequency: 31.5 Hz
		{ 0x04, 0x1c }, // BQ3 Right  - Frequency: 50 Hz
		{ 0x04, 0x30 }, // BQ4 Right  - Frequency: 80 Hz
		{ 0x04, 0x44 }, // BQ5 Right  - Frequency: 125 Hz
		{ 0x04, 0x58 }, // BQ6 Right  - Frequency: 200 Hz
		{ 0x04, 0x6c }, // BQ7 Right  - Frequency: 315 Hz
		{ 0x05, 0x08 }, // BQ8 Right  - Frequency: 500 Hz
		{ 0x05, 0x1c }, // BQ9 Right  - Frequency: 800 Hz
		{ 0x05, 0x30 }, // BQ10 Right - Frequency: 1250 Hz
		{ 0x05, 0x44 }, // BQ11 Right - Frequency: 2000 Hz
		{ 0x05, 0x58 }, // BQ12 Right - Frequency: 3150 Hz
		{ 0x05, 0x6c }, // BQ13 Right - Frequency: 5000 Hz
		{ 0x06, 0x08 }, // BQ14 Right - Frequency: 8000 Hz
		{ 0x06, 0x1c }, // BQ15 Right - Frequency: 16000 Hz
  };
#endif
#endif

}  // namespace esphome::tas58xx