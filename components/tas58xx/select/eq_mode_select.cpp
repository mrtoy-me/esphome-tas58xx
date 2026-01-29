#include "eq_mode_select.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static const char *const TAG = "tas58xx.select";

static const uint8_t EQ_OFF_NUMBER_OPTIONS = 1; // only one option if EQ is off
static const uint8_t EQ_ON_NUMBER_OPTIONS  = 2; // two options there is an EQ option

void EqModeSelect::setup() {
  size_t restored_index = EqMode::EQ_OFF;

  // retrieve the select options enum (index) which was derived from YAML configuration
  // provides the index for the EQ Mode saved in stored_options_
  // Off = 0; EQ 15 Band = 1; EQ BIAMP = 2; EQ BIAMP Presets = 3; EQ BIAMP Presets = 4
  uint8_t select_options_index = this->parent_->get_eq_mode_enum();

  if (this->parent_->use_eq_switch_refresh()) {
    // if YAML configured auto_fresh: EQ_SWITCH
    // then trigger refresh_settings and start with EQ Off
    this->trigger_refresh_settings_ = true;
  }
#ifdef USE_TAS58XX_EQ
  #ifdef USE_SPEAKER
  else {
    // if EQ and Speaker component is defined then by default enable EQ
    if (restored_index == EqMode::EQ_OFF) {
      if (select_options_index > EqMode::EQ_OFF) {
        restored_index= EqMode::EQ_ON;
      }
    }
  }
  #endif
#endif

  // based on select options enum (index) which was derived from YAML configuration
  // set size of select option as either 1 = EQ Off only or 2 = EQ Off plus one of the other EQ On options
  if (select_options_index > EqMode::EQ_OFF) {
    this->option_ptrs_.init(EQ_ON_NUMBER_OPTIONS);
  } else {
    this->option_ptrs_.init(EQ_OFF_NUMBER_OPTIONS);
  }

  // build pointer array pointing into select option strings
  this->option_ptrs_.push_back(stored_options_[EqMode::EQ_OFF].c_str());  // is always EQ Off option
  if (select_options_index > EqMode::EQ_OFF) this->option_ptrs_.push_back(stored_options_[select_options_index].c_str()); // now add second option dervied from YAML config

  traits.set_options(this->option_ptrs_);

  this->publish_state(restored_index);
  this->parent_->eq_mode_select(restored_index);
}

void EqModeSelect::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas58xx Select:");
  LOG_SELECT("  ", "Eq Mode", this);
}

void EqModeSelect::control(size_t index) {
  this->publish_state(index);
  this->parent_->eq_mode_select(index);

  // normal condition
  if (!this->trigger_refresh_settings_) return;

  // when 'trigger_refresh_settings_' is set true by 'setup'
  // then effectively 'refresh_settings' triggers on first transition from Off to Eq On
  // if 'refresh_settings' has already been called somewhere else
  // it does not matter as'parent_->refresh_settings()' will only run once
  if (index > EqMode::EQ_OFF) {
    ESP_LOGD(TAG, "Triggering refresh settings");
    this->parent_->refresh_settings();
    this->trigger_refresh_settings_ = false;
  }
}

}  // namespace esphome::tas58xx
