#include "eq_mode_select.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

static const char *const TAG = "tas58xx.select";

void EqModeSelect::setup() {
  uint8_t number_select_options;
  size_t restored_index;

  number_select_options = this->parent_->get_select_options_count();

  // check since number of select options is provided by parent
  if (number_select_options > MAX_SELECT_OPTIONS) number_select_options = MAX_SELECT_OPTIONS;

  this->pref_ = global_preferences->make_preference<size_t>(this->get_preference_hash());
  if (!this->pref_.load(&restored_index)) {
    // no select index stored
    restored_index = 0;
  } else {
    // make sure restored index is not beyond the number of select options
    if ((restored_index  + 1) > number_select_options) restored_index = 0;
  }

  this->stored_options_.resize(number_select_options);

  // Build pointer array pointing into stored_options_
  this->option_ptrs_.init(this->stored_options_.size());
  for (const auto &opt : this->stored_options_) {
    this->option_ptrs_.push_back(opt.c_str());
  }

  // Set the traits (pointers remain valid because stored_options_ persists)
  this->traits.set_options(this->option_ptrs_);

  this->publish_state(restored_index);
}

void EqModeSelect::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas58xx Select:");
  LOG_SELECT("  ", "Eq Mode", this);
}

void EqModeSelect::control(size_t index) {
  this->publish_state(index);
  this->pref_.save(&index);
  this->parent_->eq_mode_select(index);

}

}  // namespace esphome::tas58xx
