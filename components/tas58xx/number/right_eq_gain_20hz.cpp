#include "right_eq_gain_20hz.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static constexpr const char* TAG = "tas58xx.number";

void RightEqGain20hz::setup() {
  float value;
  this->pref_ = this->make_entity_preference<float>();
  if (!this->pref_.load(&value)) value= 0.0;
  this->publish_state(value);
  this->parent_->set_eq_gain(RIGHT_CHANNEL, BAND_20HZ, static_cast<int>(value));
}

void RightEqGain20hz::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas58xx Right EQ Gain Numbers:");
  ESP_LOGCONFIG(TAG, "  20Hz Band '%s'", this->get_name().c_str());
}

void RightEqGain20hz::control(float value) {
  this->publish_state(value);
  this->parent_->set_eq_gain(RIGHT_CHANNEL, BAND_20HZ, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphomme::tas58xx
