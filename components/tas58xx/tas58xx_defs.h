#pragma once

#include <cstdint>

namespace esphome::tas58xx {

enum ControlState : uint8_t {
    CTRL_DEEP_SLEEP = 0x00, // Deep Sleep
    CTRL_SLEEP      = 0x01, // Sleep
    CTRL_HI_Z       = 0x02, // Hi-Z
    CTRL_PLAY       = 0x03, // Play
   };

enum DacMode : uint8_t {
  BTL  = 0, // Bridge tied load
  PBTL = 1, // Parallel load
};

enum ModulationScheme : uint8_t {
  MODE_BD = 0,
  MODE_1SPW = 1,
};

enum ExcludeIgnoreMode : uint8_t {
    NONE        = 0,
    CLOCK_FAULT = 1,
};

enum Channels : uint8_t {
  LEFT_CHANNEL  = 0,
  RIGHT_CHANNEL,
};

enum EqMode : uint8_t {
  EQ_OFF      = 0,
  EQ_ON       = 1,  // Right=Left: left channel filters shared by both channels (ganged)
  EQ_LEFT_ON  = 2,  // separate filters for each channel, only the left channel filters are used
  EQ_RIGHT_ON = 3,  // separate filters for each channel, only the right channel filters are used
  EQ_BIAMP_ON = 4,  // separate filters for each channel, both are used
};

enum InputMixerMode : uint8_t {
  STEREO = 0,
  STEREO_INVERSE,
  MONO,
  RIGHT,
  LEFT,
};

// Future speaker configuration (subwoofer mixer and output crossbar), not used yet
// enum SubchannelMixerMode: uint8_t {
//   NO_SUB = 0,
//   LEFT_SUB,
//   RIGHT_SUB,
//   STEREO_SUB,
//   LEFT_EQ_SUB,
//   RIGHT_EQ_SUB,
// };

// enum CrossbarOutputs: uint8_t {
//   ANALOG_LEFT = 0,
//   ANALOG_RIGHT,
//   DIGITAL_LEFT,
//   DIGITAL_RIGHT,
// };

// enum CrossbarInputs: uint8_t {
//   FROM_LEFT = 0,
//   FROM_RIGHT,
//   FROM_SUB,
// };

struct Tas58xxFault {
  uint8_t channel_fault{0};                  // individual faults extracted when publishing
  uint8_t global_fault{0};                   // individual faults extracted when publishing

  bool clock_fault{false};
  bool temperature_fault{false};
  bool temperature_warning{false};

  bool is_fault_except_clock_fault{false};   // fault conditions combined except clock fault

#ifdef USE_TAS58XX_BINARY_SENSOR
  bool have_fault{false};                    // combined binary sensor - any fault found but does not include clock fault if excluded
#endif
};

static constexpr float TAS58XX_MIN_ANALOG_GAIN         = -15.5;
static constexpr float TAS58XX_MAX_ANALOG_GAIN         = 0.0;

static constexpr uint8_t TAS58XX_DIG_VOL_0DB           = 0x30;
static constexpr uint8_t TAS58XX_DIG_VOL_MINUS_103DB   = 0xFE;

// set book and page registers
static constexpr uint8_t TAS58XX_PAGE_SET              = 0x00;
static constexpr uint8_t TAS58XX_BOOK_SET              = 0x7F;
static constexpr uint8_t TAS58XX_BOOK_ZERO             = 0x00;
static constexpr uint8_t TAS58XX_PAGE_ZERO             = 0x00;

// tas58x5m registers
static constexpr uint8_t TAS58XX_DEVICE_CTRL_1         = 0x02;
static constexpr uint8_t TAS58XX_DEVICE_CTRL_2         = 0x03;
static constexpr uint8_t TAS58XX_DIG_VOL_CTRL          = 0x4C;
static constexpr uint8_t TAS58XX_AGAIN                 = 0x54;
static constexpr uint8_t TAS58XX_POWER_STATE           = 0x68;

// TAS58XX FAULT constants
static constexpr uint8_t TAS58XX_CHAN_FAULT            = 0x70;
static constexpr uint8_t TAS58XX_FAULT_CLEAR           = 0x78;
static constexpr uint8_t TAS58XX_ANALOG_FAULT_CLEAR    = 0x80;


// channels, EQ modes and mixer: counts and text
static constexpr uint8_t NUMBER_CHANNELS = 2;
static constexpr uint8_t NUMBER_EQ_BANDS = 15;
static constexpr uint8_t NUMBER_EQ_MODES = 5;
// static constexpr uint8_t NUMBER_OUTPUT_CROSSBAR = 4; // Analog Left, Analog Right, Digital Left, Digital Right

static constexpr const char* EQ_MODE_TEXT[NUMBER_EQ_MODES]   = {"EQ Off", "EQ On Right=Left", "EQ Left On", "EQ Right On", "EQ BIAMP On"};
static constexpr const char* LR_CHANNEL_TEXT[NUMBER_CHANNELS] = {"Left", "Right"};
static constexpr const char* INPUT_MIXER_MODE_TEXT[] = {"STEREO", "STEREO INVERSE", "MONO", "RIGHT", "LEFT"};
// static constexpr const char* SUBCHANNEL_MIXER_MODE_TEXT[] = {"NO SUB", "LEFT SUB", "RIGHT SUB", "STEREO SUB", "LEFT EQ SUB", "RIGHT EQ SUB"};
// static constexpr const char* CROSSBAR_INPUT_TEXT[] = {"FROM LEFT", "FROM RIGHT", "FROM SUB"};

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

// #ifdef USE_SPEAKER_CONFIG
// static constexpr uint8_t TAS5805M_SUB_CHANNEL_MIXER_GAIN_SUBADDR = 0x28; // Left to Sub = 0x28, Right to Sub = 0x2c, Left EQ to Sub = 0x30, Right EQ to Sub = 0x34
// static constexpr uint8_t TAS5805M_OUTPUT_CROSSBAR_PAGE = 0x2C;
// static constexpr uint8_t TAS5805M_OUTPUT_CROSSBAR_SUBADDR[NUMBER_OUTPUT_CROSSBAR] = {0x1C, 0x28, 0x34, 0x4c}; // Analog Left, Analog Right, Digital Left, Digital Right
// #endif

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

}  // namespace esphome::tas58xx
