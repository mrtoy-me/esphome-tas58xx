#include "left_eq_gain_3150hz.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static const char *const TAG = "tas58xx.number";

void LeftEqGain3150hz::setup() {
  float value;
  this->pref_ = global_preferences->make_preference<float>(this->get_object_id_hash());
  if (!this->pref_.load(&value)) value= 0.0;
  this->publish_state(value);
  this->parent_->set_eq_gain(BAND_3150HZ, static_cast<int>(value));
}

void LeftEqGain3150hz::dump_config() {
  ESP_LOGCONFIG(TAG, "  3150Hz Band '%s'", this->get_name().c_str());
}

void LeftEqGain3150hz::control(float value) {
  this->publish_state(value);
  this->parent_->set_eq_gain(BAND_3150HZ, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphome::tas58xx
