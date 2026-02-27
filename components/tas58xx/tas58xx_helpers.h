#pragma once

#include "esphome/core/hal.h"

namespace esphome::tas58xx_helpers {

int32_t gain_to_f9_23_(int8_t gain);

}  // namespace esphome::tas58xx_helpers