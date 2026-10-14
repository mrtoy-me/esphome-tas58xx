#pragma once

#include "esphome/components/number/number.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "../tas58xx.h"

namespace esphome::tas58xx {

// One EQ gain number, for one band of one channel. Channel and band do not change, so are constructor parameters.
class EqGainNumber : public number::Number, public Component, public Parented<Tas58xxComponent> {
 public:
  EqGainNumber(Channels channel, uint8_t band) : channel_(channel), band_(band) {}

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_CONNECTION; }

 protected:
  void control(float value) override;

  ESPPreferenceObject pref_;
  Channels channel_;
  uint8_t band_;
};

}  // namespace esphome::tas58xx
