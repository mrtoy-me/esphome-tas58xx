#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <cmath>

namespace esphome::tas58xx_helpers {

#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
  static constexpr const char* HELPER_TAG = "tas58xx.helper";
#endif

  int32_t gain_to_f9_23_(int8_t gain) {
    static constexpr float TAS58XX_LINEAR_GAIN_MAX = 255.999999f;
    static constexpr float TAS58XX_LINEAR_GAIN_MIN = -256.0f;

    float linear = powf(10.0f, ((float)gain) / 20.0f);
    if (linear > TAS58XX_LINEAR_GAIN_MAX) linear = TAS58XX_LINEAR_GAIN_MAX;
    if (linear < TAS58XX_LINEAR_GAIN_MIN) linear = TAS58XX_LINEAR_GAIN_MIN;

    int32_t fixed_9_23 = static_cast<int32_t>(linear * (1 << 23));
    int32_t little_endian = byteswap(fixed_9_23);

    ESP_LOGV(HELPER_TAG, "Gain:%ddb  Fixed 9.23: 0x%08X  Little Endian: 0x%08X", gain, fixed_9_23, little_endian);
    return little_endian;
  }

}  // namespace esphome::tas58xx_helpers