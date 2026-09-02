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

struct Tas58xxFault {
  uint8_t channel_fault{0};                  // individual faults extracted when publishing
  uint8_t global_fault{0};                   // individual faults extracted when publishing excludes clock fault

  bool temperature_fault{false};
  bool temperature_warning{false};

#ifdef USE_TAS58XX_BINARY_SENSOR
  bool have_fault{false};                    // combined binary sensor - any fault found but does not include clock fault
#endif
};

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
static constexpr uint8_t TAS58XX_CHAN_FAULT            = 0x70;
static constexpr uint8_t TAS58XX_GLOBAL_FAULT1         = 0x71;
static constexpr uint8_t TAS58XX_GLOBAL_FAULT2         = 0x72;
static constexpr uint8_t TAS58XX_OT_WARNING            = 0x73;
static constexpr uint8_t TAS58XX_FAULT_CLEAR           = 0x78;
static constexpr uint8_t TAS58XX_ANALOG_FAULT_CLEAR    = 0x80;

static constexpr size_t BOOT_SOUND_BYTES = 16;

// 4 frames of silence at 16-bit stereo = 4 * 2 channels * 2 bytes = 16 bytes
// purpose is to toggle BCLK/LRCLK so the DAC sees a valid clock before
// the CTRL_STATE -> Play transition
static constexpr uint8_t PRIME_BUFFER[BOOT_SOUND_BYTES] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

}  // namespace esphome::tas58xx
