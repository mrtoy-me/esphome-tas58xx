#include "right_eq_gain_16000hz.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static constexpr const char* TAG = "tas58xx.number";

void RightEqGain16000hz::setup() {
  float value;
  this->pref_ = this->make_entity_preference<float>();
  if (!this->pref_.load(&value)) value= 0.0;
  this->publish_state(value);
  this->parent_->set_eq_gain(RIGHT_CHANNEL, BAND_16000HZ, static_cast<int>(value));

#ifdef USE_TAS58XX_EQ_GAINS
  #ifdef USE_TAS58XX_EQ_BIAMP
  // if YAML configured auto_fresh: EQ_GAIN which is default then trigger refresh_settings
  if(this->parent_->using_auto_eq_refresh()) {
    this->parent_->refresh_eq_settings();
    ESP_LOGD(TAG, "Right EQ Gain Number Setup triggered EQ settings refresh");
  }
  #endif
#endif
}

void RightEqGain16000hz::dump_config() {
  ESP_LOGCONFIG(TAG, "  16000Hz Band '%s'", this->get_name().c_str());
}

void RightEqGain16000hz::control(float value) {
  this->publish_state(value);
  this->parent_->set_eq_gain(RIGHT_CHANNEL, BAND_16000HZ, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphome::tas58xx
