#include "eq_gain_number.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

ESPHOME_LOG_TAG(TAG, "tas58xx.number");

void EqGainNumber::setup() {
  float value;
  this->pref_ = this->make_entity_preference<float>();
  if (!this->pref_.load(&value))
    value = 0.0;  // no saved gain so set to 0dB
  this->publish_state(value);
  this->parent_->set_eq_gain(this->channel_, this->biquad_index_, static_cast<int>(value));
}

// every band setting is logged, including those defaulted in YAML
void EqGainNumber::dump_config() {
  const EqBiquad &biquad = this->parent_->get_eq_biquad(this->channel_, this->biquad_index_);
  if (biquad.type == FILTER_TYPE_LOW_PASS || biquad.type == FILTER_TYPE_HIGH_PASS ||
      biquad.type == FILTER_TYPE_LOW_PASS_CHEBYSHEV || biquad.type == FILTER_TYPE_HIGH_PASS_CHEBYSHEV) {
    ESP_LOGCONFIG(TAG, "Tas58xx %s EQ Band (biquad %d) '%s': %s %dHz Q fixed", LR_CHANNEL_TEXT[this->channel_],
                  this->biquad_index_ + 1, this->get_name().c_str(), FILTER_TYPE_TEXT[biquad.type],
                  biquad.frequency);
  } else {
    ESP_LOGCONFIG(TAG, "Tas58xx %s EQ Band (biquad %d) '%s': %s %dHz %s %.3f", LR_CHANNEL_TEXT[this->channel_],
                  this->biquad_index_ + 1, this->get_name().c_str(), FILTER_TYPE_TEXT[biquad.type],
                  biquad.frequency, biquad.q_factor);
  }
}

void EqGainNumber::control(float value) {
  this->publish_state(value);
  this->parent_->set_eq_gain(this->channel_, this->biquad_index_, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphome::tas58xx
