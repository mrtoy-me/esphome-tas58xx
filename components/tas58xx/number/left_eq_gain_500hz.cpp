#include "left_eq_gain_500hz.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static constexpr const char* TAG = "tas58xx.number";

void LeftEqGain500hz::setup() {
  float value;
  this->pref_ = this->make_entity_preference<float>();
  if (!this->pref_.load(&value)) value = 0.0; // no saved gain so set to 0dB
  this->publish_state(value);
  this->parent_->set_eq_gain(LEFT_CHANNEL, BAND_500HZ, static_cast<int>(value));
}

void LeftEqGain500hz::dump_config() {
  ESP_LOGCONFIG(TAG, "  500Hz Band '%s'", this->get_name().c_str());
}

void LeftEqGain500hz::control(float value) {
  this->publish_state(value);
  this->parent_->set_eq_gain(LEFT_CHANNEL, BAND_500HZ, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphome::tas58xx
