#include "eq_mode_select.h"
#include "esphome/core/log.h"

namespace esphome::tas58xx {

ESPHOME_LOG_TAG(TAG, "tas58xx.select");

void EqModeSelect::setup() {
  // options are set by codegen: "Off" plus the EQ mode derived from YAML configuration, if any
  // start with the EQ mode selected when EQ gains or EQ presets are configured
  size_t initial_select_index = this->parent_->is_eq_configured() ? EqMode::EQ_ON : EqMode::EQ_OFF;

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
}

}  // namespace esphome::tas58xx
