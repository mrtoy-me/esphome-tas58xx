#pragma once

#include "../tas58xx.h"
#include "esphome/components/select/select.h"
#include "esphome/core/preferences.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"  // For FixedVector

namespace esphome::tas58xx {

static const uint8_t MAX_SELECT_OPTIONS = 4; // in stored_options_

class EqModeSelect : public select::Select, public Component, public Parented<Tas58xxComponent> {

public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

protected:
  // Storage for actual string data (must persist for lifetime)
  std::vector<std::string> stored_options_ = {"Off", "EQ 15 Band", "EQ BIAMP", "EQ BIAMP Presets"};

  //Pointers into stored_options_
  FixedVector<const char*> option_ptrs_;

  ESPPreferenceObject pref_;

  void control(size_t index) override;
};

}  // namespace esphome::tas58xx
