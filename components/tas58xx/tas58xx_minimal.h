#pragma once

#include "tas58xx.h"

namespace esphome::tas58xx {

struct Tas58xxConfiguration {
    uint8_t addr;
    uint8_t value;
  }__attribute__((packed));


static constexpr uint8_t TAS58XX_CFG_META_DELAY = 254;

// Registers 0x46, 0x7D, 0x7E, page 1 register 0x51 and Page 2 registers 0x1D, 0x19 are not documented in the datasheet
// Startup sequences are derived from TI Pure Path Console (PPC) register dumps

static constexpr Tas58xxConfiguration TAS58XX_CONFIG[] = {
// initial startup sequence(reset) derived from PPC TAS5805M 3-Band DRC 2.0 96k
    { 0x00, 0x00 }, // Page 0
    { 0x7F, 0x00 }, // Book 0
    { 0x03, 0x02 }, // set Hi-Z
    { 0x01, 0x11 }, // Reset control port registers and modules
    { 0x03, 0x02 }, // set Hi-Z
    { TAS58XX_CFG_META_DELAY, 5 },

#ifdef USE_TAS5805M_DAC
// continue PPC startup sequence for TAS5805M 3-Band DRC 2.0 96k
    { 0x03, 0x00 },  // Deep Sleep
    { 0x46, 0x01 },
    { 0x03, 0x02 },  // Hi-Z
    { 0x61, 0x0B },  // ADR_PIN_CONFIG - ADR as FAULTZ
    { 0x60, 0x01 },  // ADR_OE - ADR is output
    { 0x7D, 0x11 },
    { 0x7E, 0xFF },
    { 0x00, 0x01 },  // Page 1
    { 0x51, 0x05 },
    { 0x00, 0x00 },  // Page 0

// addition register configuration
    { 0x53, 0x60 },  // ANA_CTRL 175kHz - high performance

#else
// Change to deep sleep since startup sequence for PPC TAS5825 2-Band DRC&AGL 2.0 96k
// continues its initial startup sequence immediately after reset of control port registers and modules;
// after this reset, the DAC defaults to Deep Sleep
// but for consistency with TAS5825, the initial startup sequence finishes in Hi-Z
    {0x03, 0x00},  // Deep Sleep

// continue PPC startup sequence for TAS5825M 2-Band DRC&AGL 2.0 96k
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
    {0x53, 0x01},  // ANA_CTRL PWM Phase Control – in phase and 100kHz high performace
//  {0x54 ,0x00},  // set AGAIN included in PPC TAS5825 startup sequence but not needed as AGAIN initialised in set_up()
    {0x03, 0x02},  // Hi-Z

// addition register configurations
    {0x61, 0x0b},  // GPIO0_SEL - GPIO0 as FAULTZ output
    {0x60, 0x01},  // GPIO0_OE - GPIO0 is output
    {0x77, 0x07},  // CBC_CONTROL enabling CBC function for warnings and faults
#endif
};

}  // namespace esphome::tas58xx

