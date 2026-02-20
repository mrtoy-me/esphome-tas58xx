#pragma once

#include "../tas58xx.h"
#include "esphome/components/select/select.h"
#include "esphome/core/preferences.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"  // For FixedVector

namespace esphome::tas58xx {

static constexpr uint8_t MAX_SELECT_OPTIONS = 4; // in stored_options_

class EqModeSelect : public select::Select, public Component, public Parented<Tas58xxComponent> {

public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_CONNECTION; }

protected:
  bool trigger_refresh_settings_{false};

  ESPPreferenceObject pref_;

  //Pointers into stored_options_
  FixedVector<const char*> option_ptrs_;

  // Storage for actual string data (must persist for lifetime from developer blog 7th Nov 25 "Select Entity Class: Index-Based Operations and Flash Storage" @bdraco)
  // https://developers.esphome.io/blog/2025/11/07/select-entity-class-index-based-operations-and-flash-storage/
  std::string stored_options_[MAX_SELECT_OPTIONS] = {"Off", "EQ 15 Band", "EQ BIAMP 15 Band", "EQ Presets"};

  void control(size_t index) override;
};

}  // namespace esphome::tas58xx
