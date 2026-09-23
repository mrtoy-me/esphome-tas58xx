#include "eq_band_gain.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static constexpr const char* TAG = "tas58xx.number";

void EqBandGain::setup() {
  float value;
  this->pref_ = this->make_entity_preference<float>();
  if (!this->pref_.load(&value)) value= 0.0;
  this->publish_state(value);
  this->parent_->set_eq_gain(this-channel_, this->band_, static_cast<int>(value));
}

void EqBandGain::dump_config() {
  #ifdef USE_TAS58XX_EQ_BIAMP
  ESP_LOGCONFIG(TAG, "TAS58xx Gain Number: %s EQ Band %d -> '%s'", LR_CHANNEL_TEXT[this->channel_], this->band_ + 1, this->get_name().c_str());
  #else
  ESP_LOGCONFIG(TAG, "TAS58xx Gain Number: EQ Band %d -> '%s'", this->band_ + 1, this->get_name().c_str());
  #endif
}

void EqBandGain::control(float value) {
  this->publish_state(value);
  this->parent_->set_eq_gain(this-channel_, this->band_, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphomme::tas58xx
