#include "eq_mode_select.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static constexpr const char* TAG = "tas58xx.select";

static constexpr uint8_t EQ_OFF_NUMBER_OPTIONS = 1; // only one option if EQ is off
static constexpr uint8_t EQ_ON_NUMBER_OPTIONS  = 2; // two options there is an EQ option

void EqModeSelect::setup() {
  // retrieve the select options index which was derived from YAML configuration
  // provides the index for the EQ Mode to be used in stored_options_
  // Off = 0; EQ 15 Band = 1; EQ BIAMP = 2; EQ Presets = 3
  uint8_t select_options_index = this->parent_->get_configured_eq_mode();

  size_t initial_select_index = EqMode::EQ_OFF;

#ifdef USE_TAS58XX_EQ_PRESETS
  if (this->parent_->using_auto_eq_refresh()) {
    this->parent_->refresh_eq_settings();
    ESP_LOGD(TAG, "EQ Mode Select Setup triggered EQ settings refresh");
  }
#endif

  if (this->parent_->is_eq_configured()) {
    initial_select_index = EqMode::EQ_ON;
  }

  // if manual eq refresh - start with Select EQ OFF and set trigger for transition from Off to Eq Mode
  if(this->parent_->using_manual_eq_refresh()) {
    initial_select_index = EqMode::EQ_OFF;
    this->trigger_refresh_settings_ = true;
  }

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

  this->publish_state(initial_select_index);
  this->parent_->select_eq_mode(initial_select_index);
}

void EqModeSelect::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas58xx Select:");
  LOG_SELECT("  ", "Eq Mode", this);
}

void EqModeSelect::control(size_t index) {
  this->publish_state(index);
  this->parent_->select_eq_mode(index);

  // normal condition
  if (!this->trigger_refresh_settings_) return;

  // when 'trigger_refresh_settings_' is set true by 'setup' manual trigger refresh is active
  // then effectively 'refresh_settings' triggers on first transition from Off to one of the Eq Modes
  // if 'refresh_settings' has somehow been already been called somewhere else
  // it does not matter as'parent_->refresh_settings()' will only run once
  if (index > EqMode::EQ_OFF) {
    ESP_LOGD(TAG, "EQ Mode Select manually triggered EQ settings refresh");
    this->parent_->refresh_eq_settings();
    this->trigger_refresh_settings_ = false;
  }
}

}  // namespace esphome::tas58xx
