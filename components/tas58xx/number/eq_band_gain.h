#pragma once

#include "esphome/components/number/number.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "../tas58xx.h"

namespace esphome::tas58xx {

class EqBandGain : public number::Number, public Component, public Parented<Tas58xxComponent> {
 public:
  void set_channel(Channels channel) { this->channel_ = channel; }
  void set_band(uint8_t band) { this->band_ = band; }
  // void set_filter_type(EqBandFilterType type) { this->filter_type_ = type; }
  // void set_frequency(uint16_t freq) { this->frequency_ = freq; }
  // void set_q_factor(float q) { this->q_factor_ = q; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_CONNECTION; }


 protected:
  uint8_t band_;
  Channels channel_;

  void control(float value) override;

  ESPPreferenceObject pref_;
};

}  // namespace esphome::tas58xx
