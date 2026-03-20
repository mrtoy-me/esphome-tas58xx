#pragma once

#include "tas58xx.h"

namespace esphome::tas58xx {

struct Tas58xxConfiguration {
    uint8_t addr;
    uint8_t value;
  }__attribute__((packed));

// Startup sequence flag
static constexpr uint8_t TAS58XX_CFG_META_DELAY = 254;

static constexpr uint8_t SR_48KHZ = 0x11;
static constexpr uint8_t SR_96KHZ = 0x01;

#ifdef USE_SPEAKER_CONFIG
static constexpr uint8_t INTERNAL_SAMPLE_RATE = SR_48KHZ;
#endif

#ifndef USE_SPEAKER_CONFIG
static constexpr uint8_t INTERNAL_SAMPLE_RATE = SR_96KHZ;
#endif

static constexpr Tas58xxConfiguration TAS58XX_CONFIG[] = {
// RESET
    { 0x00, 0x00 }, //
    { 0x7f, 0x00 },
    { 0x03, 0x02 },
    { 0x01, 0x11 },
    { 0x03, 0x02 },
    { TAS58XX_CFG_META_DELAY, 5 },
    { 0x03, 0x00 },
    { 0x46, INTERNAL_SAMPLE_RATE }, // undocumented register 0x46 = Set Internal Sample Rate
    { 0x03, 0x02 },
    { 0x61, 0x0b },
    { 0x60, 0x01 },
    { 0x7d, 0x11 },
    { 0x7e, 0xff },
    { 0x00, 0x01 },
    { 0x51, 0x05 },
// Register Tuning
    { 0x00, 0x00 },
    { 0x7f, 0x00 },
    { 0x02, 0x00 },
    { 0x30, 0x00 },
    { 0x4c, 0x30 },
    { 0x53, 0x00 },
    { 0x54, 0x00 }, // analog gain 0db
    { 0x03, 0x03 },
    { 0x78, 0x80 },
};

}  // namespace esphome::tas58xx
