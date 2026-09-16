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

enum ModulationScheme : uint8_t {
  MODE_BD = 0,
  MODE_1SPW = 1,
};

struct FaultBinarySensorProperties {
  binary_sensor::BinarySensor *fault_sensor{nullptr};
  uint8_t register_index{0};
  uint8_t bit_mask{0};
  bool last_state{false};
};

static constexpr uint8_t MAX_FAULT_SENSORS = 18; // maximum possible on TAS5825

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



}  // namespace esphome::tas58xx
