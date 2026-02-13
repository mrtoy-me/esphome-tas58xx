#include "mixer_mode_select.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static const char *const TAG = "tas58xx.select";

#ifdef USE_DAC_MODE_PBTL
static const uint8_t MIXER_MODE_MAX_INDEX = 2;
#else
static const uint8_t MIXER_MODE_MAX_INDEX = 4;
#endif

void MixerModeSelect::setup() {
  #ifdef USE_DAC_MODE_PBTL
  this->traits.set_options({"MONO", "RIGHT", "LEFT"});
  #else
  this->traits.set_options({"STEREO", "STEREO_INVERSE", "MONO", "RIGHT", "LEFT"});
  #endif

  size_t restored_index;

  // load saved mixer mode index
  this->pref_ = this->make_entity_preference<size_t>();
  if (!this->pref_.load(&restored_index)) {
    restored_index = this->parent_->get_mixer_mode_();
  } else {
    if (restored_index > MIXER_MODE_MAX_INDEX) {
      this->parent_->get_mixer_mode_();
    }
  }

  this->publish_state(restored_index);
  this->parent_->set_mixer_mode_(static_cast<MixerMode>(restored_index));
}

void MixerModeSelect::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas58xx Select:");
  LOG_SELECT("  ", "Mixer Mode", this);
}

void MixerModeSelect::control(size_t index) {
  this->publish_state(index);
  this->pref_.save(&index);
  this->parent_->set_mixer_mode_(static_cast<MixerMode>(index));
}

}  // namespace esphome::tas58xx
