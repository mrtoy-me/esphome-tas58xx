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
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

protected:
  ESPPreferenceObject pref_;

  //Pointers into stored_options_
  FixedVector<const char*> option_ptrs_;

  // Storage for actual string data (must persist for lifetime)
  std::string stored_options_[MAX_SELECT_OPTIONS] = {"Off", "EQ Shared", "EQ BIAMP", "EQ BIAMP Presets"};

  bool trigger_refresh_settings_{false};
  uint8_t select_options_index_{0};
  
  void control(size_t index) override;
};

}  // namespace esphome::tas58xx
