#include "eq_preset_right_select.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static constexpr const char* TAG = "tas58xx.select";

void EqPresetRightSelect::setup() {
  this->traits.set_options({"Flat","LF 60Hz", "LF 70Hz", "LF 80Hz", "LF 90Hz", "LF 100Hz", "LF 110Hz", "LF 120Hz", "LF 130Hz", "LF 140Hz", "LF 150Hz",
                            "HF 60Hz", "HF 70Hz", "HF 80Hz", "HF 90Hz", "HF 100Hz", "HF 110Hz", "HF 120Hz", "HF 130Hz", "HF 140Hz", "HF 150Hz"});

  size_t restored_index;

  // load saved mixer mode index
  this->pref_ = this->make_entity_preference<size_t>();
  if (!this->pref_.load(&restored_index)) {
    restored_index = 0;
  }

  this->publish_state(restored_index);
  this->parent_->set_eq_preset(Channels::RIGHT_CHANNEL, static_cast<uint8_t>(restored_index));
}

void EqPresetRightSelect::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas58xx Select:");
  LOG_SELECT("  ", "EQ Preset Cutoff - Right Channel", this);
}

void EqPresetRightSelect::control(size_t index) {
  this->publish_state(index);
  this->pref_.save(&index);
  this->parent_->set_eq_preset(Channels::RIGHT_CHANNEL, static_cast<uint8_t>(index));
}

}  // namespace esphome::tas58xx
