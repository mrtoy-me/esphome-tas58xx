#include "mixer_mode_select.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static constexpr const char* TAG = "tas58xx.select";

#ifdef USE_DAC_MODE_PBTL
// only last three MixerModes are valid for Select
static constexpr uint8_t MAX_SELECT_INDEX = 2;
static constexpr uint8_t MIN_MIXER_MODE = 2; // Mono
#else
// all five possible MixerModes are valid for Select
static constexpr uint8_t MAX_SELECT_INDEX = 4;
static constexpr uint8_t MIN_MIXER_MODE = 0; // Stereo
#endif

void MixerModeSelect::setup() {
  #ifdef USE_DAC_MODE_PBTL
  this->traits.set_options({"MONO", "RIGHT", "LEFT"});
  #else
  this->traits.set_options({"STEREO", "STEREO_INVERSE", "MONO", "RIGHT", "LEFT"});
  #endif

  size_t restored_index;

  // load saved mixer mode index
  // mixed mode index saved is always the index of the all 5 MixerModes
  // even if the reduced number of valid MixerModes is used in this select for PBTL mode
  this->pref_ = this->make_entity_preference<size_t>();
  if (!this->pref_.load(&restored_index)) {
    restored_index = this->parent_->get_mixer_mode();
  }

  if (restored_index < MIN_MIXER_MODE) {
    restored_index = MIN_MIXER_MODE;
  }

  this->publish_state(restored_index -  MIN_MIXER_MODE);
  this->parent_->set_mixer_mode(static_cast<MixerMode>(restored_index));
}

void MixerModeSelect::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas58xx Select:");
  LOG_SELECT("  ", "Mixer Mode", this);
}

void MixerModeSelect::control(size_t index) {
  this->publish_state(index);
  index = index + MIN_MIXER_MODE;
  this->pref_.save(&index);
  this->parent_->set_mixer_mode(static_cast<MixerMode>(index));
}

}  // namespace esphome::tas58xx
