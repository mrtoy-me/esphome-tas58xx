#pragma once

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

  enum MixerMode : uint8_t {
    STEREO = 0,
    STEREO_INVERSE,
    MONO,
    RIGHT,
    LEFT,
  };

  enum MixerGains : uint8_t {
    LEFT_2_LEFT_GAIN = 0,
    RIGHT_2_LEFT_GAIN,
    LEFT_2_RIGHT_GAIN,
    RIGHT_2_RIGHT_GAIN,
  };

  enum EqChannels : uint8_t {
	  LEFT_CHANNEL  = 0,
		RIGHT_CHANNEL,
  };

  enum EqMode : uint8_t {
    EQ_OFF        = 0,
    EQ_ON         = 1,
    EQ_15BAND_ON  = 1,
    EQ_BIAMP_ON   = 2,
    EQ_PRESETS_ON = 3,
  };

  static const char* const MIXER_MODE_TEXT[] = {"STEREO", "STEREO_INVERSE", "MONO", "RIGHT", "LEFT"};
  static const char* const EQ_MODE_TEXT[]   = {"Off", "EQ 15 Band", "EQ BIAMP 15 Band", "EQ Presets"};
  static const char* const EQ_CHANNEL_TEXT[] = {"Left", "Right"};

  struct Tas58xxConfiguration {
    uint8_t offset;
    uint8_t value;
  }__attribute__((packed));

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

  // Startup sequence constants
  static const uint8_t TAS58XX_CFG_META_DELAY        = 254;

  static const float TAS58XX_MIN_ANALOG_GAIN         = -15.5;
  static const float TAS58XX_MAX_ANALOG_GAIN         = 0.0;

  // set book and page registers
  static const uint8_t TAS58XX_PAGE_SET              = 0x00;
  static const uint8_t TAS58XX_BOOK_SET              = 0x7F;
  static const uint8_t TAS58XX_BOOK_CONTROL          = 0x00;
  static const uint8_t TAS58XX_PAGE_ZERO             = 0x00;

  // tas58x5m registers
  static const uint8_t TAS58XX_DEVICE_CTRL_1         = 0x02;
  static const uint8_t TAS58XX_DEVICE_CTRL_2         = 0x03;
  static const uint8_t TAS58XX_FS_MON                = 0x37;
  static const uint8_t TAS58XX_BCK_MON               = 0x38;
  static const uint8_t TAS58XX_DIG_VOL_CTRL          = 0x4C;
  static const uint8_t TAS58XX_ANA_CTRL              = 0x53;
  static const uint8_t TAS58XX_AGAIN                 = 0x54;
  #ifdef USE_TAS5805M_DAC
  static const uint8_t TAS5805M_DSP_MISC             = 0x66;
  #endif
  static const uint8_t TAS58XX_POWER_STATE           = 0x68;

  // TAS58XX FAULT constants
  static const uint8_t TAS58XX_CHAN_FAULT            = 0x70;
  static const uint8_t TAS58XX_GLOBAL_FAULT1         = 0x71;
  static const uint8_t TAS58XX_GLOBAL_FAULT2         = 0x72;
  static const uint8_t TAS58XX_OT_WARNING            = 0x73;
  static const uint8_t TAS58XX_FAULT_CLEAR           = 0x78;
  static const uint8_t TAS58XX_ANALOG_FAULT_CLEAR    = 0x80;

  // EQ constants equivalent to the indexes of the eq_modes
  // Off=Bypass EQ On + Ganged On; EQ 15 Band=Bypass EQ Off + Ganged On; EQ BIAMP 15 Band and EQ Presets=Bypass EQ Off + Ganged Off
  #ifdef USE_TAS5805M_DAC
  static const uint8_t   TAS5805M_CTRL_EQ[]           = {0b0111, 0b0110, 0b1110, 0b1110};
  #else
  static const uint8_t   TAS5825M_EQ_CTRL_BOOK        = 0x8C;
  static const uint8_t   TAS5825M_EQ_CTRL_PAGE        = 0x0B;
  static const uint8_t   TAS5825M_GANG_EQ             = 0x28;
  static const uint8_t   TAS5825M_BYPASS_EQ           = 0x2C;

  static const uint32_t  TAS5825M_CTRL_BYPASS_EQ[]    = {0x00000001, 0x00000000, 0x00000000, 0x00000000}; // 0x00000000 Bypass EQ = false ie EQ enabled
  static const uint32_t  TAS5825M_CTRL_GANGED_EQ[]    = {0x00000001, 0x00000001, 0x00000000, 0x00000000}; // 0x00000001 EQ Ganged ie L/R channel common coefficients
  #endif

  // Level meter constants

  // Mixer Gain and Channel Gain constants
  static const int8_t TAS58XX_CHANNEL_GAIN_MAX_DB      = 24;
  static const int8_t TAS58XX_CHANNEL_GAIN_MIN_DB      = -TAS58XX_CHANNEL_GAIN_MAX_DB;

  static const float TAS58XX_LINEAR_GAIN_MAX = 255.999999f;
  static const float TAS58XX_LINEAR_GAIN_MIN = -256.0f;

  static const uint8_t TAS58XX_MIXER_CHANNEL_GAINS_BOOK = 0x8C;

  #ifdef USE_TAS5805M_DAC
  // TAS5805M
  static const uint8_t TAS58XX_MIXER_GAIN_PAGE          = 0x29;
  static const uint8_t TAS58XX_MIXER_GAIN_OFFSET[]      = {0x18, 0x1c, 0x20, 0x24};
  static const uint8_t TAS58XX_CHANNEL_GAIN_PAGE        = 0x2A;
  static const uint8_t TAS58XX_CHANNEL_GAIN_OFFSET[]    = {0x24, 0x28};
  #else
  // TAS5825M
  static const uint8_t TAS58XX_MIXER_GAIN_PAGE          = 0x0B;
  static const uint8_t TAS58XX_MIXER_GAIN_OFFSET[]      = {0x14, 0x18, 0x1c, 0x20};
  static const uint8_t TAS58XX_CHANNEL_GAIN_PAGE        = 0x0B;
  static const uint8_t TAS58XX_CHANNEL_GAIN_OFFSET[]    = {0x0c, 0x10};
  #endif

  static const uint32_t TAS58XX_MIXER_VALUE_MUTE        = 0x00000000;
  static const uint32_t TAS58XX_MIXER_VALUE_0DB         = 0x00008000;
  static const uint32_t TAS58XX_MIXER_VALUE_MINUS6DB    = 0x00004000;

}  // namespace esphome::tas58xx
