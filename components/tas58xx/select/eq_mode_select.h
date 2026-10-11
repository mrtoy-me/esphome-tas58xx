#pragma once

#include "../tas58xx.h"
#include "esphome/components/select/select.h"
#include "esphome/core/component.h"

namespace esphome::tas58xx {

class EqModeSelect : public select::Select, public Component, public Parented<Tas58xxComponent> {

public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_CONNECTION; }

protected:
  void control(size_t index) override;
};

}  // namespace esphome::tas58xx
