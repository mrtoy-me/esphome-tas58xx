#include "right_eq_gain_80hz.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static constexpr const char* TAG = "tas58xx.number";

void RightEqGain80hz::setup() {
  float value;
  this->pref_ = this->make_entity_preference<float>();
  if (!this->pref_.load(&value)) value = 0.0; // no saved gain so set to 0dB
  this->publish_state(value);
  this->parent_->set_eq_gain(RIGHT_CHANNEL, BAND_80HZ, static_cast<int>(value));
}

void RightEqGain80hz::dump_config() {
  ESP_LOGCONFIG(TAG, "  80Hz Band '%s'", this->get_name().c_str());
}

void RightEqGain80hz::control(float value) {
  this->publish_state(value);
  this->parent_->set_eq_gain(RIGHT_CHANNEL, BAND_80HZ, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphome::tas58xx
