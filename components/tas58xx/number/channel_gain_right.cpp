#include "channel_gain_right.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static const char *const TAG = "tas58xx.number";

void ChannelGainRight::setup() {
  float value;
  this->pref_ = this->make_entity_preference<float>();
  if (!this->pref_.load(&value)) value= 0.0;
  this->publish_state(value);
  this->parent_->set_channel_gain(RIGHT_CHANNEL, static_cast<int>(value));
}

void ChannelGainRight::dump_config() {
  ESP_LOGCONFIG(TAG, "  Right Channel '%s'", this->get_name().c_str());
}

void ChannelGainRight::control(float value) {
  this->publish_state(value);
  this->parent_->set_channel_gain(RIGHT_CHANNEL, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphomme::tas58xx
