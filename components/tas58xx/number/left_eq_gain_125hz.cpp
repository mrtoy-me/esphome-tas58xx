#include "left_eq_gain_125hz.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static const char *const TAG = "tas58xx.number";

void LeftEqGain125hz::setup() {
  float value;
  this->pref_ = global_preferences->make_preference<float>(this->get_preference_hash());
  if (!this->pref_.load(&value)) value = 0.0; // no saved gain so set to 0dB
  this->publish_state(value);
  this->parent_->set_eq_gain(EQ_CHANNEL_LEFT, BAND_125HZ, static_cast<int>(value));
}

void LeftEqGain125hz::dump_config() {
  ESP_LOGCONFIG(TAG, "  125Hz Band '%s'", this->get_name().c_str());
}

void LeftEqGain125hz::control(float value) {
  this->publish_state(value);
  this->parent_->set_eq_gain(EQ_CHANNEL_LEFT, BAND_125HZ, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphome::tas58xx
