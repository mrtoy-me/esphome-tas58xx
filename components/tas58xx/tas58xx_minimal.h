#pragma once

#include "tas58xx.h"

namespace esphome::tas58xx {

struct Tas58xxConfiguration {
    uint8_t addr;
    uint8_t value;
  }__attribute__((packed));


static constexpr uint8_t TAS58XX_CFG_META_DELAY = 254;

// Startup sequences are derived from TI PurePath Console (PPC) register dumps. Register 0x00 selects the page.
// Registers 0x46, 0x7D, 0x7E, page 1 register 0x51 and page 2 registers 0x1D, 0x19 are not documented in datasheet.

static constexpr Tas58xxConfiguration TAS58XX_CONFIG[] = {
// Header of every TI PurePath Console export: select book 0 in case the MCU restarted without a PDN toggle,
// silence the output with Hi-Z, reset the DSP and the control registers, then return to Hi-Z
    {0x00, 0x00},  // Page 0
    {0x7F, 0x00},  // Book 0
    {0x03, 0x02},  // DEVICE_CTRL_2: Hi-Z
    {0x01, 0x11},  // RESET_CTRL: reset control port registers and modules
    {0x03, 0x02},  // DEVICE_CTRL_2: Hi-Z
    {TAS58XX_CFG_META_DELAY, 5},

#ifdef USE_TAS5805M_DAC
// Remainder of the startup sequence from TI PurePath Console, run after the reset.
// Derived from the PPC "TAS5805M 3-Band DRC 2.0 96k" startup sequence.
    {0x03, 0x00},  // DEVICE_CTRL_2: deep sleep
    {0x46, 0x01},
    {0x03, 0x02},  // DEVICE_CTRL_2: Hi-Z
    // The I2C address is latched at power up, after which the ADR pin can report faults
    {0x61, 0x0B},  // ADR_PIN_CONFIG: FAULTZ
    {0x60, 0x01},  // ADR_PIN_CTRL: output
    {0x7D, 0x11},
    {0x7E, 0xFF},
    {0x00, 0x01},  // Page 1
    {0x51, 0x05},
    {0x00, 0x00},  // Page 0

    // Additional register configuration
    {0x53, 0x60},  // ANA_CTRL: for high audio performance use 175kHz bandwidth with Fsw=768kHz

#else
// Remainder of the startup sequence, run after the reset above.
// Derived from the TI PurePath Console (PPC) "TAS5825M 2-Band DRC&AGL 2.0 96k" startup sequence.

    // The actual PPC TAS5825M sequence continues after reset of the control port registers and modules.
    // After reset, the default state is deep sleep. The initial reset sequence above ends in Hi-Z.
    // Deep sleep is set here, so TAS5825M has the same state as PPC sequence before further registers are written.
    {0x03, 0x00},  // DEVICE_CTRL_2: deep sleep

    // Remainder of the startup sequence
    {0x7D, 0x11},
    {0x7E, 0xFF},
    {0x00, 0x01},  // Page 1
    {0x51, 0x05},
    {0x00, 0x02},  // Page 2
    {0x1D, 0x00},
    {0x19, 0x80},
    {0x00, 0x00},  // Page 0
    {0x46, 0x11},
    // {0x02, 0x00} written here by PPC; but omitted as setup later writes DEVICE_CTRL_1 using YAML dac_mode
    {0x53, 0x01},  // ANA_CTRL: PWM phase control in phase, 100kHz high performance
    // {0x54, 0x00} written here by PPC; but omitted as setup later writes AGAIN using YAML analog_gain
    {0x03, 0x02},  // DEVICE_CTRL_2: Hi-Z

    // Additional register configuration
    // GPIO1 is a dedicated pin on TAS5825M (separate from ADR) which is commonly used for reporting faults
    {0x62, 0x0B},  // GPIO1_SEL: GPIO0 as FAULTZ
    {0x60, 0x10},  // GPIO1_CTRL: GPIO1 is output
    {0x77, 0x07},  // CBC_CONTROL: enable cycle-by-cycle current limit for warnings and faults
#endif
};

}  // namespace esphome::tas58xx
