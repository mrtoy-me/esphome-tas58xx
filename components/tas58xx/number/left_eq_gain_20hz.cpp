#include "left_eq_gain_20hz.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static const char *const TAG = "tas58xx.number";

void LeftEqGain20hz::setup() {
  float value;
  this->pref_ = global_preferences->make_preference<float>(this->get_preference_hash());
  if (!this->pref_.load(&value)) value= 0.0;
  this->publish_state(value);
  this->parent_->set_eq_gain(BAND_20HZ, static_cast<int>(value));
}

void LeftEqGain20hz::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas58xx EQ Gain Numbers:");
  ESP_LOGCONFIG(TAG, "  20Hz Band '%s'", this->get_name().c_str());
}

void LeftEqGain20hz::control(float value) {
  this->publish_state(value);
  this->parent_->set_eq_gain(BAND_20HZ, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphomme::tas58xx
