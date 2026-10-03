#pragma once

#include "tas58xx.h"

namespace esphome::tas58xx {

struct Tas58xxConfiguration {
    uint8_t addr;
    uint8_t value;
  }__attribute__((packed));

// Startup sequence flag
static constexpr uint8_t TAS58XX_CFG_META_DELAY = 254;

static constexpr Tas58xxConfiguration TAS58XX_CONFIG[] = {
// RESET
    { 0x00, 0x00 }, // Page 0
    { 0x7f, 0x00 }, // Book 0
    { 0x03, 0x02 }, // set Hi-Z
    { 0x01, 0x11 }, // Reset control port registers and Reset modules
    { 0x03, 0x02 }, // set Hi-Z
    { TAS58XX_CFG_META_DELAY, 5 },
// Registers 0x46, 0x7D, 0x7E, page 1 register 0x51 and Page 2 registers 0x1D, 0x19 are not documented in the datasheet.
#ifdef USE_TAS5805M_DAC
    { 0x03, 0x00 },  // Deep Sleep
    { 0x46, 0x01 },
    { 0x03, 0x02 },  // Hi-Z
    { 0x61, 0x0b },  // ADR_PIN_CONFIG - ADR as FAULTZ
    { 0x60, 0x01 },  // ADR_OE - ADR is output
    { 0x7d, 0x11 },
    { 0x7e, 0xff },
    { 0x00, 0x01 },  // Page 1
    { 0x51, 0x05 },
// Register Tuning
    { 0x00, 0x00 },  // Page 0
    { 0x7f, 0x00 },  // Book 0
    { 0x02, 0x00 },  // DEVICE_CTRL_1 - BD MODE, DAMP_PBTL set Damp to PBL MODE
    { 0x30, 0x00 },  // SDOUT Sel - SDOUT DSP output (post-processing)
    { 0x4c, 0x30 },  // set digital volume 0dB
    { 0x53, 0x00 },  // ANA_CTRL 80kHz
    { 0x54, 0x00 },  // set AGAIN 0dB
    // { 0x03, 0x03 }, removed as implemented after sending boot sound
    // { 0x78, 0x80 },
#else
    {0x03, 0x00},  // Deep Sleep
    {0x7d, 0x11},
    {0x7e, 0xff},
    {0x00, 0x01},  // Page 1
    {0x51, 0x05},
    {0x00, 0x02},  // Page 2
    {0x1d, 0x00},
    {0x19, 0x80},
    {0x00, 0x00},  // Page 0
    {0x46, 0x11},
    {0x02, 0x00},  // DEVICE_CTRL_1 - BD MODE, DAMP_PBTL set Damp to PBL MODE
    {0x53, 0x01},  // ANA_CTRL PWM Phase Control – in phase and 100kHz
    {0x54 ,0x00},  // AGAIN 0dB
    {0x03, 0x02},  // Hi-Z
    {0x61, 0x0b},  // GPIO0_SEL - GPIO0 as FAULTZ output
    {0x60, 0x01},  // GPIO0_OE - GPIO0 is output
    {0x77, 0x07},  // CBC_CONTROL enabling CBC function for warnings and faults
#endif
};

}  // namespace esphome::tas58xx

