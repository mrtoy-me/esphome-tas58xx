#include "right_eq_gain_16000hz.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static const char *const TAG = "tas58xx.number";

void RightEqGain16000hz::setup() {
  float value;
  this->pref_ = global_preferences->make_preference<float>(this->get_preference_hash());
  if (!this->pref_.load(&value)) value= 0.0;
  this->publish_state(value);
  this->parent_->set_eq_gain(EQ_CHANNEL_RIGHT, BAND_16000HZ, static_cast<int>(value));

  // if YAML configured auto_fresh: EQ_GAIN which is default then trigger refresh_settings
  if(this->parent_->use_eq_gain_refresh()) {
    this->parent_->refresh_settings();
  }
}

void RightEqGain16000hz::dump_config() {
  ESP_LOGCONFIG(TAG, "  16000Hz Band '%s'", this->get_name().c_str());
}

void RightEqGain16000hz::control(float value) {
  this->publish_state(value);
  this->parent_->set_eq_gain(EQ_CHANNEL_RIGHT, BAND_16000HZ, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphome::tas58xx
