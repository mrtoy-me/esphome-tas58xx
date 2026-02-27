#include "left_eq_gain_31p5hz.h"
#include "esphome/core/log.h"


namespace esphome::tas58xx {

static constexpr const char* TAG = "tas58xx.number";

void LeftEqGain31p5hz::setup() {
  float value;
  this->pref_ = this->make_entity_preference<float>();
  if (!this->pref_.load(&value)) value= 0.0;
  this->publish_state(value);
  this->parent_->set_eq_gain(LEFT_CHANNEL, BAND_31_5HZ, static_cast<int>(value));
}

void LeftEqGain31p5hz::dump_config() {
  ESP_LOGCONFIG(TAG, "  31.5Hz Band '%s'", this->get_name().c_str());
}

void LeftEqGain31p5hz::control(float value) {
  this->publish_state(value);
  this->parent_->set_eq_gain(LEFT_CHANNEL, BAND_31_5HZ, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphome::tas58xx
