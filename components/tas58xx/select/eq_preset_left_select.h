#pragma once

#include "../tas58xx.h"
#include "esphome/components/select/select.h"
#include "esphome/core/preferences.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"  // For FixedVector

namespace esphome::tas58xx {

class EqPresetLeftSelect : public select::Select, public Component, public Parented<Tas58xxComponent> {

public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_CONNECTION; }

protected:
  ESPPreferenceObject pref_;

  void control(size_t index) override;
};

}  // namespace esphome::tas58xx
