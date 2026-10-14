#include "eq_gain_number.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

ESPHOME_LOG_TAG(TAG, "tas58xx.number");

// 31.5Hz is stored as 32 in EQ_BAND_FREQUENCY, so the band names used in YAML are kept here for the log
static constexpr const char *EQ_BAND_NAME[NUMBER_EQ_BANDS] = {"20",   "31.5", "50",   "80",   "125",
                                                              "200",  "315",  "500",  "800",  "1250",
                                                              "2000", "3150", "5000", "8000", "16000"};

void EqGainNumber::setup() {
  float value;
  this->pref_ = this->make_entity_preference<float>();
  if (!this->pref_.load(&value))
    value = 0.0;  // no saved gain so set to 0dB
  this->publish_state(value);
  this->parent_->set_eq_gain(this->channel_, this->band_, static_cast<int>(value));
}

void EqGainNumber::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas58xx %s EQ Gain Number: %sHz Band '%s'", LR_CHANNEL_TEXT[this->channel_],
                EQ_BAND_NAME[this->band_], this->get_name().c_str());
}

void EqGainNumber::control(float value) {
  this->publish_state(value);
  this->parent_->set_eq_gain(this->channel_, this->band_, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphome::tas58xx
