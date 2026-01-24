#pragma once

#include "../tas58xx.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome::tas58xx {

class EnableEqSwitch : public switch_::Switch, public Component, public Parented<Tas58xxComponent> {
public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_CONNECTION; }

protected:
  void write_state(bool state) override;

  bool trigger_refresh_settings_{false};
};

}  // namespace esphome::tas58xx
