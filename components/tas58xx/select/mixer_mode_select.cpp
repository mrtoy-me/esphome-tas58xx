#include "mixer_mode_select.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static constexpr const char* TAG = "tas58xx.select";

void MixerModeSelect::setup() {
  // the offset is kept per select, so two DACs (eg one BTL, one PBTL) do not change each other's options
  if (this->parent_->get_configured_dac_mode()) {
    // PBTL
    this->min_mixer_mode_ = MONO;
    this->traits.set_options({"MONO", "RIGHT", "LEFT"});
  } else {
    // BTL
    this->min_mixer_mode_ = STEREO;
    this->traits.set_options({"STEREO", "STEREO_INVERSE", "MONO", "RIGHT", "LEFT"});
  }

  size_t restored_index;

  // load saved mixer mode index
  // mixed mode index saved is always the index of the all 5 MixerModes
  // even if the reduced number of valid MixerModes is used in this select for PBTL mode
  this->pref_ = this->make_entity_preference<size_t>();
  if (!this->pref_.load(&restored_index) || restored_index > LEFT) {
    restored_index = this->parent_->get_mixer_mode();
  }

  if (restored_index < this->min_mixer_mode_) {
    restored_index = this->min_mixer_mode_;
  }

  this->publish_state(restored_index - this->min_mixer_mode_);
  this->parent_->set_input_mixer_mode(static_cast<InputMixerMode>(restored_index));
}

void MixerModeSelect::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas58xx Select:");
  LOG_SELECT("  ", "Mixer Mode", this);
}

void MixerModeSelect::control(size_t index) {
  this->publish_state(index);
  index = index + this->min_mixer_mode_;
  this->pref_.save(&index);
  this->parent_->set_input_mixer_mode(static_cast<InputMixerMode>(index));
}

}  // namespace esphome::tas58xx
