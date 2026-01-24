#include "left_eq_gain_800hz.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static const char *const TAG = "tas58xx.number";

void LeftEqGain800hz::setup() {
  float value;
  this->pref_ = global_preferences->make_preference<float>(this->get_object_id_hash());
  if (!this->pref_.load(&value)) value = 0.0; // no saved gain so set to 0dB
  this->publish_state(value);
  this->parent_->set_eq_gain(BAND_800HZ, static_cast<int>(value));
}

void LeftEqGain800hz::dump_config() {
  ESP_LOGCONFIG(TAG, "  800Hz Band '%s'", this->get_name().c_str());
}

void LeftEqGain800hz::control(float value) {
  this->publish_state(value);
  this->parent_->set_eq_gain(BAND_800HZ, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphome::tas58xx
