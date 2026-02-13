#include "channel_gain_left.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static const char *const TAG = "tas58xx.number";

void ChannelGainLeft::setup() {
  float value;
  this->pref_ = this->make_entity_preference<float>();
  if (!this->pref_.load(&value)) value= 0.0;
  this->publish_state(value);
  this->parent_->set_channel_gain(LEFT_CHANNEL, static_cast<int>(value));
}

void ChannelGainLeft::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas58xx Channel Gain Numbers:");
  ESP_LOGCONFIG(TAG, "  Left Channel '%s'", this->get_name().c_str());
}

void ChannelGainLeft::control(float value) {
  this->publish_state(value);
  this->parent_->set_channel_gain(LEFT_CHANNEL, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphomme::tas58xx
