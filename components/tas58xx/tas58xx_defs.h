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

enum InputMixerMode : uint8_t {
  STEREO = 0,
  STEREO_INVERSE,
  MONO,
  RIGHT,
  LEFT,
};

static constexpr const char* INPUT_MIXER_MODE_TEXT[] = {"STEREO", "STEREO INVERSE", "MONO", "RIGHT", "LEFT"};

#ifdef USE_TAS58XX_BINARY_SENSOR
struct FaultBinarySensorProperties {
  binary_sensor::BinarySensor *fault_sensor{nullptr};
  uint8_t register_index{0};
  uint8_t bit_mask{0};
  bool last_state{false};
};

static constexpr size_t MAX_FAULT_SENSORS = 18; // maximum possible on TAS5825
#endif

static constexpr float TAS58XX_MIN_ANALOG_GAIN         = -15.5;
static constexpr float TAS58XX_MAX_ANALOG_GAIN         = 0.0;

// set book and page registers
static constexpr uint8_t TAS58XX_PAGE_SET              = 0x00;
static constexpr uint8_t TAS58XX_BOOK_SET              = 0x7F;
static constexpr uint8_t TAS58XX_BOOK_ZERO             = 0x00;
static constexpr uint8_t TAS58XX_PAGE_ZERO             = 0x00;

// tas58x5m registers
static constexpr uint8_t TAS58XX_DEVICE_CTRL_1         = 0x02;
static constexpr uint8_t TAS58XX_DEVICE_CTRL_2         = 0x03;
static constexpr uint8_t TAS58XX_FS_MON                = 0x37;
static constexpr uint8_t TAS58XX_BCK_MON               = 0x38;
static constexpr uint8_t TAS58XX_DIG_VOL_CTRL          = 0x4C;
static constexpr uint8_t TAS58XX_ANA_CTRL              = 0x53;
static constexpr uint8_t TAS58XX_AGAIN                 = 0x54;
static constexpr uint8_t TAS58XX_POWER_STATE           = 0x68;

// TAS58XX FAULT constants
static constexpr uint8_t TAS58XX_START_FAULT_REGISTERS = 0x70;
static constexpr uint8_t TAS58XX_FAULT_CLEAR           = 0x78;
static constexpr uint8_t TAS58XX_ANALOG_FAULT_CLEAR    = 0x80;

static constexpr uint8_t TAS58XX_AUDIO_CTRL_BOOK = 0x8C;

#ifdef USE_TAS5805M_DAC
// TAS5805M
static constexpr uint8_t TAS58XX_MIXER_GAIN_PAGE = 0x29;
static constexpr uint8_t TAS58XX_MIXER_GAIN_SUBADDR = 0x18; // Left to Left = 0x18, Right to Left = 0x1c, Left to Right = 0x20, Right to Right = 0x24
#else
// TAS5825M
static constexpr uint8_t TAS58XX_MIXER_GAIN_PAGE = 0x0B;
static constexpr uint8_t TAS58XX_MIXER_GAIN_SUBADDR = 0x14; // Left to Left = 0x14, Right to Left = 0x18, Left to Right = 0x1c, Right to Right = 0x20
#endif

// mixer gain coefficients converted to little endian
static constexpr uint32_t TAS58XX_MIXER_COEFF_MUTE = 0x00000000;
static constexpr uint32_t TAS58XX_MIXER_COEFF_0DB = 0x00008000;
static constexpr uint32_t TAS58XX_MIXER_COEFF_MINUS6DB = 0x00004000;


}  // namespace esphome::tas58xx
