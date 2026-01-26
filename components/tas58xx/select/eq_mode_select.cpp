#include "eq_mode_select.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static const char *const TAG = "tas58xx.select";

void EqModeSelect::setup() {
  size_t restored_index; // initial index of select: Off = 0; EQ 15 Band = 1; EQ BIAMP = 2; EQ BIAMP Presets = 4

  // retrieve the maximum select options -> derived from YAML configuration
  // Off = 1; Off + EQ 15 Band = 2; Off + EQ 15 Band + EQ BIAMP = 3; Off + EQ 15 Band + EQ BIAMP + EQ BIAMP Presets = 4
  this->select_options_index_ = (uint8_t)this->parent_->get_eq_mode_enum();

  // // check it is not beyond maximum defined in this component since number of select options is provided by parent
  // if (number_select_options > MAX_SELECT_OPTIONS) number_select_options = MAX_SELECT_OPTIONS;


  if (this->parent_->use_eq_switch_refresh()) {
    // if YAML configured auto_fresh: EQ_SWITCH
    // then trigger refresh_settings and start with EQ Off
    this->trigger_refresh_settings_ = true;
    restored_index = EqMode::EQ_OFF;
  } else {
    // try to load saved select index
    this->pref_ = global_preferences->make_preference<size_t>(this->get_preference_hash());

    // no saved select index so initially set to EQ Off
    if (!this->pref_.load(&restored_index)) {
      restored_index = EqMode::EQ_OFF;
    }

    // establish default select index
#ifdef USE_TAS58XX_EQ
    #ifdef USE_SPEAKER
    // if EQ is Off then enable EQ by default if speaker is defined
    if (restored_index == EqMode::EQ_OFF) {
      if (this->select_options_index_ > EqMode::EQ_OFF) {
        restored_index= EqMode::EQ_15BAND_ON;
      }
    }
    #else
    // EQ Off by dafult if no YAML speaker defined
    restored_index = EqMode::EQ_OFF;
    #endif
#else
    // EQ Off if no YAML EQ numbers defined
    restored_index = EqMode::EQ_OFF;
#endif
  }

  if (this->select_options_index_ > EqMode::EQ_OFF) {
    this->option_ptrs_.init(2);
  } else {
    this->option_ptrs_.init(1);
  }

  // Build pointer array pointing into stored_options_
  //this->option_ptrs_.init(this->stored_options_.size());
  this->option_ptrs_.push_back(stored_options_[0].c_str());
  if (this->select_options_index_ > EqMode::EQ_OFF) this->option_ptrs_.push_back(stored_options_[this->select_options_index_].c_str());

  // for (const auto &opt : this->stored_options_) {
  //   this->option_ptrs_.push_back(opt.c_str());
  // }

  // Set the traits (pointers remain valid because stored_options_ persists)
  this->traits.set_options(this->option_ptrs_);

  this->publish_state(restored_index);
  this->parent_->eq_mode_select(restored_index);
}

void EqModeSelect::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas58xx Select:");
  LOG_SELECT("  ", "Eq Mode", this);
  ESP_LOGI(TAG, "  Select Index: %d >> %s", this->select_options_index_, stored_options_[this->select_options_index_].c_str());
}

void EqModeSelect::control(size_t index) {
  this->publish_state(index);
  this->pref_.save(&index);
  this->parent_->eq_mode_select(index);
  ESP_LOGI(TAG, "  Select Index: %d", index);

  // normal condition
  if (!this->trigger_refresh_settings_) return;

  // when 'trigger_refresh_settings_' is set true by 'setup'
  // then effectively 'refresh_settings' triggers on first transition from Off to On
  // if 'refresh_settings' has already been called,
  // it does not matter as'parent_->refresh_settings()'  will only run once
  if (index > 0) {
    ESP_LOGD(TAG, "Triggering refresh settings");
    this->parent_->refresh_settings();
    this->trigger_refresh_settings_ = false;
  }
}

}  // namespace esphome::tas58xx
