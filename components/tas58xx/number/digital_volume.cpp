#include "digital_volume.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static constexpr const char* TAG = "tas58xx.number";

void DigitalVolume::setup() {
  float value;
  this->pref_ = this->make_entity_preference<float>();
  if (!this->pref_.load(&value)) value= 10.0;
  this->publish_state(value);
  this->parent_->set_tas58xx_volume(value / 100.0);
}

void DigitalVolume::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas58xx Volume:");
  ESP_LOGCONFIG(TAG, "  Digital Volume '%s'", this->get_name().c_str());
}

void DigitalVolume::control(float value) {
  this->publish_state(value);
  this->parent_->set_tas58xx_volume(value / 100.0);
  this->pref_.save(&value);
}

}  // namespace esphomme::tas58xx
