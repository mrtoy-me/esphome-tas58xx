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
    { 0x03, 0x02 }, // Hi-Z
    { 0x01, 0x11 }, // Reset control port registers and Reset modules
    { 0x03, 0x02 }, // Hi-Z
    { TAS58XX_CFG_META_DELAY, 5 },
// test TAS5825 sequence follows for new component
    { 0x03, 0x00 }, // Deep Sleep
    {0x7d,	0x11}, // ? - same as tas5805 but differnet ordering
    {0x7e,	0xff}, // ? - same as tas5805 but differnet ordering
    {0x00,	0x01}, // Page 1 before net write - same as tas5805
    {0x51,	0x05}, // same as tas5805 but differnet ordering: right auto mute: value -> 1.065 not listed on tas5805 datasheet
    {0x00,	0x02}, // Page 2
    {0x1d,	0x00}, // ??
    {0x19,	0x80}, // ??
    {0x00,	0x00}, // Page 0 - same as tas5805 but differnet ordering but last page wrote value on both
    {0x46,	0x11}, // ??
    {0x02,	0x00}, // sets 00:BD MODE DAMP_PBTL set Damp to PBL
    {0x53,	0x01}, // ANA_CTRL PWM Phase Control – in phase – reserved bit in tas5805
    {0x77,	0x07}, // CBC_CONTROL enabling CBC function for warnings and faults

// proven TAS5805 sequence follows and also used for tas5825 in my tas58xx component
//     { 0x03, 0x00 }, // Deep Sleep
//     { 0x46, 0x01 }, // ??
//     { 0x03, 0x02 }, // Hi-Z
//     { 0x61, 0x0b }, // ADR as FAULTZ
//     { 0x60, 0x01 }, // ADR output
//     { 0x7d, 0x11 }, // ?
//     { 0x7e, 0xff }, // ?
//     { 0x00, 0x01 }, // Page 1
//     { 0x51, 0x05 }, // ?
// // Register Tuning
//     { 0x00, 0x00 }, // set book 0
//     { 0x7f, 0x00 }, // set page 0
//     { 0x02, 0x00 }, // DAMP_MOD 00:BD MODE 0: SET DAMP TO BTL MODE
//     { 0x30, 0x00 }, // SDOUT Sel - SDOUT DSP output (post-processing)
//     { 0x4c, 0x30 }, // digital volume
//     { 0x53, 0x00 }, // analog control 80khz
//     { 0x54, 0x00 }, // analog gain 0db
//     // { 0x03, 0x03 }, removed as implemented after sending boot sound
//     // { 0x78, 0x80 },
};

}  // namespace esphome::tas58xx

