#include "left_eq_gain_16000hz.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static constexpr const char* TAG = "tas58xx.number";

void LeftEqGain16000hz::setup() {
  float value;
  this->pref_ = this->make_entity_preference<float>();
  if (!this->pref_.load(&value)) value= 0.0;
  this->publish_state(value);
  this->parent_->set_eq_gain(LEFT_CHANNEL, BAND_16000HZ, static_cast<int>(value));
}

void LeftEqGain16000hz::dump_config() {
  ESP_LOGCONFIG(TAG, "  16000Hz Band '%s'", this->get_name().c_str());
}

void LeftEqGain16000hz::control(float value) {
  this->publish_state(value);
  this->parent_->set_eq_gain(LEFT_CHANNEL, BAND_16000HZ, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphome::tas58xx
