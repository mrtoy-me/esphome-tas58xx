#include "right_eq_gain_3150hz.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static const char *const TAG = "tas58xx.number";

void RightEqGain3150hz::setup() {
  float value;
  this->pref_ = global_preferences->make_preference<float>(this->get_preference_hash());
  if (!this->pref_.load(&value)) value= 0.0;
  this->publish_state(value);
  this->parent_->set_eq_gain(EQ_CHANNEL_RIGHT, BAND_3150HZ, static_cast<int>(value));
}

void RightEqGain3150hz::dump_config() {
  ESP_LOGCONFIG(TAG, "  3150Hz Band '%s'", this->get_name().c_str());
}

void RightEqGain3150hz::control(float value) {
  this->publish_state(value);
  this->parent_->set_eq_gain(EQ_CHANNEL_RIGHT, BAND_3150HZ, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphome::tas58xx
