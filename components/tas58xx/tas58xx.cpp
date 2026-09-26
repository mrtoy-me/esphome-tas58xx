#include "tas58xx.h"
#include "tas58xx_minimal.h"

#include "esphome/core/log.h"
#include "esphome/core/application.h"

namespace esphome::tas58xx {

#ifdef USE_TAS5805M_DAC
static constexpr const char* TAG = "tas5805m";
#else
static constexpr const char* TAG = "tas5825m";
#endif

static constexpr const char* ERROR = "Error";
static constexpr const char* MIXER_MODE = "Mixer Mode";
static constexpr const char* EQ_BAND = "EQ Band";

static constexpr uint8_t TAS58XX_MUTE_CONTROL = 0x08; // bit mask for mute control

void Tas58xxComponent::setup() {
  ESP_LOGCONFIG(TAG, "Running setup");
  if (this->enable_pin_ != nullptr) {
    this->enable_pin_->setup();
    this->enable_pin_->digital_write(false);
    delay(1);
    this->enable_pin_->digital_write(true);
    delay(5);
  }

  if (!this->configure_registers_()) {
    this->error_code_ = CONFIGURATION_FAILED;
    this->mark_failed();
  }

  #ifdef USE_TAS58XX_BINARY_SENSOR
  this->configure_active_fault_sensors_();
  #else
  ESP_LOGD(TAG, "stopping update polling");
  stop_poller();
  #endif

  // rescale -103db to 24db digital volume range to register digital volume range 254 to 0
  this->tas58xx_raw_volume_max_ = (uint8_t)((this->tas58xx_volume_max_ - 24) * -2);
  this->tas58xx_raw_volume_min_ = (uint8_t)((this->tas58xx_volume_min_ - 24) * -2);
}

bool Tas58xxComponent::configure_registers_() {
  static constexpr uint8_t ESPHOME_MAXIMUM_DELAY = 5; // milliseconds

  size_t i = 0;
  size_t counter = 0;
  size_t number_configurations = sizeof(TAS58XX_CONFIG) / sizeof(TAS58XX_CONFIG[0]);

  while (i < number_configurations) {
    switch (TAS58XX_CONFIG[i].addr) {
      case TAS58XX_CFG_META_DELAY:
        if (TAS58XX_CONFIG[i].value > ESPHOME_MAXIMUM_DELAY) return false;
        delay(TAS58XX_CONFIG[i].value);
        break;
      default:
        if (!this->tas58xx_write_byte_(TAS58XX_CONFIG[i].addr, TAS58XX_CONFIG[i].value)) return false;
        counter++;
        break;
    }
    i++;
  }
  this->number_registers_configured_ = counter;

  // intended to execute and complete before any other component's loop() exists
  // and therefore before any other component opens i2s channel
  // failure does not mark_failed as this step only should affect proper EQ operation
  this->i2s_sync_successful_ = this->i2s_sync_(&this->i2s_sync_byte_count_, &i2s_sync_attempts_);

  // enable Tas58xx
  if (!this->set_deep_sleep_off_()) return false;

  if (!this->set_dac_mode_(this->tas58xx_dac_mode_)) return false;

  if (!this->set_input_mixer_mode_(this->tas58xx_input_mixer_mode_)) return false;

  if (!this->set_analog_gain_(this->tas58xx_analog_gain_)) return false;

  if (!this->set_state_(CTRL_PLAY)) return false;
  if (!this->tas58xx_write_byte_(TAS58XX_FAULT_CLEAR, TAS58XX_ANALOG_FAULT_CLEAR)) return false;
  return true;
}

void Tas58xxComponent::update() {
#ifdef USE_TAS58XX_BINARY_SENSOR
  static constexpr size_t MAX_FAULT_REGISTERS = 4;

  uint8_t fault_registers_current_state_[MAX_FAULT_REGISTERS];
  bool trigger_clear_faults{false};

  // read all faults registers
  if (!this->tas58xx_read_bytes_(TAS58XX_START_FAULT_REGISTERS, fault_registers_current_state_, MAX_FAULT_REGISTERS)) {
    ESP_LOGW(TAG, "%s reading fault registers", ERROR);
    return;
  };

  for (size_t i = 0; i < this->active_fault_sensor_count_; i++) {

    auto &x = this->active_fault_sensors_[i];
    bool state = (fault_registers_current_state_[x.register_index] & x.bit_mask) != 0;
    trigger_clear_faults |= state;

    // dedup is implemented in binary sensor component, but log messages are not shown at debug level
    // tas58xx faults/warnings require a log message
    // so fault register state changes are tracked in this component

    // only log and publish on an actual transition
    if (state != x.last_state) {
      if (state) {
        ESP_LOGW(TAG, "%s >> ON", x.fault_sensor->get_name().c_str());
      } else {
        ESP_LOGI(TAG, "%s >> OFF", x.fault_sensor->get_name().c_str());
      }
      x.fault_sensor->publish_state(state);
      x.last_state = state;
    }
  }

  if (trigger_clear_faults) {
    ESP_LOGD(TAG, "Clearing fault registers");
    if (!this->clear_fault_registers_()) {
      ESP_LOGW(TAG, "%s clearing fault registers", ERROR);
    }
  }
#endif
}

void Tas58xxComponent::dump_config() {
#ifdef USE_TAS5805M_DAC
  ESP_LOGCONFIG(TAG, "Tas5805m Audio Dac:");
#else
  ESP_LOGCONFIG(TAG, "Tas5825m Audio Dac:");
#endif

  switch (this->error_code_) {
    case CONFIGURATION_FAILED:
      ESP_LOGE(TAG, "  Setup Failed: %zu",this->i2c_error_);
      break;
    case NONE:
      ESP_LOGCONFIG(TAG,
              "  Setup Complete:\n"
              "    I2S Sync: %s -> %zu bytes @ %zums\n"
              "    Registers Configured: %i\n"
              "    Fault Sensors Active: %i\n\n",
              this->i2s_sync_successful_ ? "Success" : "Failure",
              this->i2s_sync_byte_count_,
              this->i2s_sync_attempts_,
              this->number_registers_configured_,
              this->active_fault_sensor_count_);

      LOG_I2C_DEVICE(this);
      ESP_LOGCONFIG(TAG, "  I2S Dout Pin: GPIO%d", this->dout_pin_);
      LOG_PIN("  Enable Pin: ", this->enable_pin_);

      ESP_LOGCONFIG(TAG,
              "  Analog Gain: %3.1fdB\n"
              "  DAC Mode: %s\n"
              "  Mixer Mode: %s\n"
              "  Volume Maximum: %idB\n"
              "  Volume Minimum: %idB\n",
              this->tas58xx_analog_gain_,
              this->tas58xx_dac_mode_ ? "PBTL" : "BTL",
              INPUT_MIXER_MODE_TEXT[this->tas58xx_input_mixer_mode_],
              this->tas58xx_volume_max_, this->tas58xx_volume_min_
              );
      LOG_UPDATE_INTERVAL(this);
      break;
  }

#ifdef USE_TAS58XX_BINARY_SENSOR
  ESP_LOGCONFIG(TAG, "Tas58xx Binary Sensors:");
  LOG_BINARY_SENSOR("  ", "Left Channel DC Fault", this->left_channel_dc_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Right Channel DC Fault", this->right_channel_dc_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Left Channel Over Current Fault", this->left_channel_over_current_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Right Channel Over Current Fault", this->right_channel_over_current_fault_binary_sensor_);

  LOG_BINARY_SENSOR("  ", "OTP CRC Check Error", this->otp_crc_check_error_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "BQ Write Failed", this->bq_write_failed_binary_sensor_);
  #ifdef USE_TAS5825M_DAC
  LOG_BINARY_SENSOR("  ", "EEPROM Load Error", this->eeprom_load_error_binary_sensor_);
  #endif
  LOG_BINARY_SENSOR("  ", "PVDD Under Voltage Fault", this->pvdd_under_voltage_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "PVDD Over Voltage Fault", this->pvdd_over_voltage_fault_binary_sensor_);

  #ifdef USE_TAS5825M_DAC
  LOG_BINARY_SENSOR("  ", "Right Channel CBC Current Fault", this->right_channel_cbc_current_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Left Channel CBC Current Fault", this->left_channel_cbc_current_fault_binary_sensor_);
  #endif
  LOG_BINARY_SENSOR("  ", "Over Temperature Shutdown", this->over_temperature_shutdown_fault_binary_sensor_);

  #ifdef USE_TAS5825M_DAC
  LOG_BINARY_SENSOR("  ", "Left Channel CBC Current Warning", this->left_channel_cbc_current_warning_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Right Channel CBC Current Warning", this->right_channel_cbc_current_warning_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Over Temperature 146C Warning", this->over_temperature_146c_warning_binary_sensor_);
  #endif
  LOG_BINARY_SENSOR("  ", "Over Temperature 134C Warning", this->over_temperature_134c_warning_binary_sensor_);
  #ifdef USE_TAS5825M_DAC
  LOG_BINARY_SENSOR("  ", "Over Temperature 122C Warning", this->over_temperature_122c_warning_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Over Temperature 112C Warning", this->over_temperature_112c_warning_binary_sensor_);
  #endif
#endif

}

// public //

// used by 'enable_dac_switch'
void Tas58xxComponent::enable_dac(bool enable) {
  enable ? this->set_deep_sleep_off_() : this->set_deep_sleep_on_();
}

bool Tas58xxComponent::set_input_mixer_mode_(InputMixerMode mode) {
  // follows order of input mixer registers = Left to Left, Right to Left, Left to Right, Right to Right
  struct MixerCoefficients {
    uint32_t l_to_l;
    uint32_t r_to_l;
    uint32_t l_to_r;
    uint32_t r_to_r;
  }__attribute__((packed));

  MixerCoefficients mixer_coefficients;

  switch (mode) {
    case STEREO:
      mixer_coefficients.l_to_l = TAS58XX_MIXER_COEFF_0DB;
      mixer_coefficients.r_to_l = TAS58XX_MIXER_COEFF_MUTE;
      mixer_coefficients.l_to_r = TAS58XX_MIXER_COEFF_MUTE;
      mixer_coefficients.r_to_r = TAS58XX_MIXER_COEFF_0DB;
      break;

    case STEREO_INVERSE:
      mixer_coefficients.l_to_l = TAS58XX_MIXER_COEFF_MUTE;
      mixer_coefficients.r_to_l = TAS58XX_MIXER_COEFF_0DB;
      mixer_coefficients.l_to_r = TAS58XX_MIXER_COEFF_0DB;
      mixer_coefficients.r_to_r = TAS58XX_MIXER_COEFF_MUTE;
      break;

    case MONO:
      mixer_coefficients.l_to_l = TAS58XX_MIXER_COEFF_MINUS6DB;
      mixer_coefficients.r_to_l = TAS58XX_MIXER_COEFF_MINUS6DB;
      mixer_coefficients.l_to_r = TAS58XX_MIXER_COEFF_MINUS6DB;
      mixer_coefficients.r_to_r = TAS58XX_MIXER_COEFF_MINUS6DB;
      break;

    case LEFT:
      mixer_coefficients.l_to_l = TAS58XX_MIXER_COEFF_0DB;
      mixer_coefficients.r_to_l = TAS58XX_MIXER_COEFF_MUTE;
      mixer_coefficients.l_to_r = TAS58XX_MIXER_COEFF_0DB;
      mixer_coefficients.r_to_r = TAS58XX_MIXER_COEFF_MUTE;
      break;

    case RIGHT:
      mixer_coefficients.l_to_l = TAS58XX_MIXER_COEFF_MUTE;
      mixer_coefficients.r_to_l = TAS58XX_MIXER_COEFF_0DB;
      mixer_coefficients.l_to_r = TAS58XX_MIXER_COEFF_MUTE;
      mixer_coefficients.r_to_r = TAS58XX_MIXER_COEFF_0DB;
      break;

    default:
      ESP_LOGE(TAG, "Invalid %s", MIXER_MODE);
      return false;
  }

  if (!this->book_page_write_bytes_(TAS58XX_AUDIO_CTRL_BOOK, TAS58XX_MIXER_GAIN_PAGE, TAS58XX_MIXER_GAIN_SUBADDR,
                                  reinterpret_cast<uint8_t*>(&mixer_coefficients), sizeof(MixerCoefficients))) {
    ESP_LOGW(TAG, "%s writing Input %s: %s", ERROR, MIXER_MODE, INPUT_MIXER_MODE_TEXT[mode]);
    return false;
  }

  ESP_LOGD(TAG, "Input %s >> %s", MIXER_MODE, INPUT_MIXER_MODE_TEXT[mode]);
  this->tas58xx_input_mixer_mode_ = mode;
  return true;
}


bool Tas58xxComponent::set_mute_off() {
  if (!this->is_muted_) return true;
  if (!this->tas58xx_write_byte_(TAS58XX_DEVICE_CTRL_2, this->tas58xx_control_state_)) return false;
  this->is_muted_ = false;
  ESP_LOGV(TAG, "Mute Off");
  return true;
}

// set bit 3 MUTE in TAS58XX_DEVICE_CTRL_2 and retain current Control State
// ensures get_state = get_power_state
bool Tas58xxComponent::set_mute_on() {
  if (this->is_muted_) return true;
  if (!this->tas58xx_write_byte_(TAS58XX_DEVICE_CTRL_2, this->tas58xx_control_state_ + TAS58XX_MUTE_CONTROL)) return false;
  this->is_muted_ = true;
  ESP_LOGV(TAG, "Mute On");
  return true;
}

// override for audio_dac component volume, so mediaplayer can determine current volume of tas58xx dac
float Tas58xxComponent::volume() {
  uint8_t raw_volume = 254; // default to lowest raw volume if i2c read error
  this->tas58xx_read_bytes_(TAS58XX_DIG_VOL_CTRL, &raw_volume, 1);
  return remap<float, uint8_t>(raw_volume, this->tas58xx_raw_volume_min_, this->tas58xx_raw_volume_max_, 0.0f, 1.0f);
}

// controls both left and right channel digital volume
// digital volume is 24 dB to -103 dB in -0.5 dB step
// 00000000: +24.0 dB
// 00000001: +23.5 dB
// 00101111: +0.5 dB
// 00110000: 0.0 dB
// 00110001: -0.5 dB
// 11111110: -103 dB
// 11111111: Mute
// override for audio_dac component set_volume, so mediaplayer can adjust volume of tas58xx dac
bool Tas58xxComponent::set_volume(float volume) {
  float new_volume = clamp(volume, 0.0f, 1.0f);
  uint8_t raw_volume = remap<uint8_t, float>(new_volume, 0.0f, 1.0f, this->tas58xx_raw_volume_min_, this->tas58xx_raw_volume_max_);
  if (!this->tas58xx_write_byte_(TAS58XX_DIG_VOL_CTRL, raw_volume)) return false;
  #if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
    int8_t dB = -(raw_volume / 2) + 24;
    ESP_LOGV(TAG, "Volume >> %ddB", dB);
  #endif
  return true;
}

// protected //

// Analog Gain Control , with 0.5dB one step
// lower 5 bits controls the analog gain.
// 00000: 0 dB (29.5V peak voltage)
// 00001: -0.5db
// 11111: -15.5 dB
// set analog gain in dB
bool Tas58xxComponent::set_analog_gain_(float gain_db) {
  static constexpr uint8_t TOP_3BITS_MASK = 0xE0;

  if ((gain_db < TAS58XX_MIN_ANALOG_GAIN) || (gain_db > TAS58XX_MAX_ANALOG_GAIN)) return false;

  uint8_t new_again = static_cast<uint8_t>(-gain_db * 2.0);

  uint8_t current_again;
  if (!this->tas58xx_read_bytes_(TAS58XX_AGAIN, &current_again, 1)) return false;

  // keep top 3 reserved bits combine with bottom 5 analog gain bits
  new_again = (current_again & TOP_3BITS_MASK) | new_again;
  if (!this->tas58xx_write_byte_(TAS58XX_AGAIN, new_again)) return false;

  ESP_LOGD(TAG, "Analog Gain >> %fdB", gain_db);
  return true;
}

// only runs once from 'setup'
bool Tas58xxComponent::set_dac_mode_(DacMode mode) {
  uint8_t current_value;
  if (!this->tas58xx_read_bytes_(TAS58XX_DEVICE_CTRL_1, &current_value, 1)) return false;

  // Update bit 2 based on the mode
  if (mode == PBTL) {
      current_value |= (1 << 2);  // Set bit 2 to 1 (PBTL mode)
  } else {
      current_value &= ~(1 << 2); // Clear bit 2 to 0 (BTL mode)
  }
  if (!this->tas58xx_write_byte_(TAS58XX_DEVICE_CTRL_1, current_value)) return false;

  // save so 'set_dac_mode_' could be used more generally
  this->tas58xx_dac_mode_ = mode;
  ESP_LOGD(TAG, "DAC mode >> %s", this->tas58xx_dac_mode_ ? "PBTL" : "BTL");
  return true;
}

bool Tas58xxComponent::set_deep_sleep_off_() {
  if (this->tas58xx_control_state_ != CTRL_DEEP_SLEEP) return true; // already not in deep sleep
  // preserve mute state
  uint8_t new_value = (this->is_muted_) ? (CTRL_PLAY + TAS58XX_MUTE_CONTROL) : CTRL_PLAY;
  if (!this->tas58xx_write_byte_(TAS58XX_DEVICE_CTRL_2, new_value)) return false;

  this->tas58xx_control_state_ = CTRL_PLAY;                        // set Control State to play
  ESP_LOGV(TAG, "Deep Sleep >> Off");
  #if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
  if (this->is_muted_) ESP_LOGV(TAG, "Mute On preserved");
  #endif
  return true;
}

bool Tas58xxComponent::set_deep_sleep_on_() {
  if (this->tas58xx_control_state_ == CTRL_DEEP_SLEEP) return true; // already in deep sleep

  // preserve mute state
  uint8_t new_value = (this->is_muted_) ? (CTRL_DEEP_SLEEP + TAS58XX_MUTE_CONTROL) : CTRL_DEEP_SLEEP;
  if (!this->tas58xx_write_byte_(TAS58XX_DEVICE_CTRL_2, new_value)) return false;

  this->tas58xx_control_state_ = CTRL_DEEP_SLEEP;                   // set Control State to deep sleep
  ESP_LOGV(TAG, "Deep Sleep >> On");
  #if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
  if (this->is_muted_) ESP_LOGV(TAG, "Mute On preserved");
  #endif
  return true;
}

bool Tas58xxComponent::set_state_(ControlState state) {
  if (this->tas58xx_control_state_ == state) return true;
  if (!this->tas58xx_write_byte_(TAS58XX_DEVICE_CTRL_2, state)) return false;
  this->tas58xx_control_state_ = state;
  return true;
}

//// fault binary sensor processing

#ifdef USE_TAS58XX_BINARY_SENSOR
void Tas58xxComponent::configure_active_fault_sensors_() {
  // offset of CHAN_FAULT register from the first of the 4 fault registers
  static constexpr uint8_t CHAN_FAULT_OFFSET = 0;
  static constexpr uint8_t TAS58XX_CHAN_FAULT_OFFSET = CHAN_FAULT_OFFSET;

  // use TAS58xx datasheet CHAN_FAULT register field(bit) labelling
  // but convert the bit position to a bit mask
  // bits 7 - 4 reserved
  static constexpr uint8_t CH1_DC_1 = static_cast<uint8_t>(1u << 3);
  static constexpr uint8_t CH2_DC_1 = static_cast<uint8_t>(1u << 2);
  static constexpr uint8_t CH1_OC_I = static_cast<uint8_t>(1u << 1);
  static constexpr uint8_t CH2_OC_I = static_cast<uint8_t>(1u << 0);

  if (this->left_channel_dc_fault_binary_sensor_ != nullptr) {
    this->left_channel_dc_fault_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {left_channel_dc_fault_binary_sensor_, CHAN_FAULT_OFFSET, CH1_DC_1};
  }

  if (this->right_channel_dc_fault_binary_sensor_ != nullptr) {
    this->right_channel_dc_fault_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {this->right_channel_dc_fault_binary_sensor_, CHAN_FAULT_OFFSET, CH2_DC_1};
  }

  if (this->left_channel_over_current_fault_binary_sensor_ != nullptr) {
    this->left_channel_over_current_fault_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {this->left_channel_over_current_fault_binary_sensor_, CHAN_FAULT_OFFSET, CH1_OC_I};
  }

  if (this->right_channel_over_current_fault_binary_sensor_ != nullptr) {
    this->right_channel_over_current_fault_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {right_channel_over_current_fault_binary_sensor_, CHAN_FAULT_OFFSET, CH2_OC_I};
  }

  // offset of GLOBAL_FAULT1 register from the first of the 4 fault registers
  static constexpr uint8_t GLOBAL_FAULT1_OFFSET  = 1;
  static constexpr uint8_t TAS58XX_GLOBAL_FAULT1_OFFSET = GLOBAL_FAULT1_OFFSET;

  // use TAS58xx datasheet GLOBAL_FAULT1 register field(bit) labelling
  // but convert the bit position to a bit mask
  static constexpr uint8_t OTP_CRC_ERROR = static_cast<uint8_t>(1u << 7);
  static constexpr uint8_t BQ_WR_ERROR = static_cast<uint8_t>(1u << 6);
  #ifdef USE_TAS5825M_DAC
  static constexpr uint8_t LOAD_EEPROM_ERROR = static_cast<uint8_t>(1u << 5);
  #endif
  // bits 4 - 3 reserved
  // bit 2 CLK_FAULT_I not used as it gives false faults when i2s is manipulated by audio components
  static constexpr uint8_t PVDD_OV_I = static_cast<uint8_t>(1u << 1);
  static constexpr uint8_t PVDD_UV_I = static_cast<uint8_t>(1u << 0);

  if (this->otp_crc_check_error_binary_sensor_ != nullptr) {
    this->otp_crc_check_error_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {this->otp_crc_check_error_binary_sensor_, GLOBAL_FAULT1_OFFSET, OTP_CRC_ERROR};
  }

  if (this->bq_write_failed_binary_sensor_ != nullptr) {
    this->bq_write_failed_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {this->bq_write_failed_binary_sensor_, GLOBAL_FAULT1_OFFSET, BQ_WR_ERROR};
  }

  #ifdef USE_TAS5825M_DAC
  if (this->eeprom_load_error_binary_sensor_ != nullptr) {
    this->eeprom_load_error_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {this->eeprom_load_error_binary_sensor_, GLOBAL_FAULT1_OFFSET, LOAD_EEPROM_ERROR};
  }
  #endif

  if (this->pvdd_over_voltage_fault_binary_sensor_ != nullptr) {
    this->pvdd_over_voltage_fault_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {this->pvdd_over_voltage_fault_binary_sensor_, GLOBAL_FAULT1_OFFSET, PVDD_OV_I};
  }

  if (this->pvdd_under_voltage_fault_binary_sensor_ != nullptr) {
    this->pvdd_under_voltage_fault_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {this->pvdd_under_voltage_fault_binary_sensor_, GLOBAL_FAULT1_OFFSET, PVDD_UV_I};
  }

  // offset of GLOBAL_FAULT2 register from the first of the 4 fault registers
  static constexpr uint8_t GLOBAL_FAULT2_OFFSET  = 2;
  static constexpr uint8_t TAS58XX_GLOBAL_FAULT2_OFFSET = GLOBAL_FAULT2_OFFSET;

  // use TAS58xx datasheet GLOBAL_FAULT1 register field(bit) labelling
  // but convert the bit position to a bit mask
  // bits 7 - 3 reserved
  #ifdef USE_TAS5825M_DAC
  static constexpr uint8_t CBC_FAULT_CH2_I = static_cast<uint8_t>(1u << 2);
  static constexpr uint8_t CBC_FAULT_CH1_I = static_cast<uint8_t>(1u << 1);
  #endif
  static constexpr uint8_t OTSD_I = static_cast<uint8_t>(1u << 0);

  #ifdef USE_TAS5825M_DAC
  if (this->right_channel_cbc_current_fault_binary_sensor_ != nullptr) {
    this->right_channel_cbc_current_fault_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {this->right_channel_cbc_current_fault_binary_sensor_, GLOBAL_FAULT2_OFFSET, CBC_FAULT_CH2_I};
  }

  if (this->left_channel_cbc_current_fault_binary_sensor_ != nullptr) {
    this->left_channel_cbc_current_fault_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {this->left_channel_cbc_current_fault_binary_sensor_, GLOBAL_FAULT2_OFFSET, CBC_FAULT_CH1_I};
  }
  #endif

  if (this->over_temperature_shutdown_fault_binary_sensor_ != nullptr) {
    this->over_temperature_shutdown_fault_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {this->over_temperature_shutdown_fault_binary_sensor_, GLOBAL_FAULT2_OFFSET, OTSD_I};
  }

  // offset of WARNING register from the first of the 4 fault registers
  static constexpr uint8_t WARNING_OFFSET  = 3;
  static constexpr uint8_t TAS58XX_WARNING_OFFSET = WARNING_OFFSET;

  // use TAS5825 datasheet WARNING register field(bit) labels (except use OTW_LEVELx as label should not end in "_I")
  // but convert the bit position to a bit mask
  // bits 7 - 6 reserved
  #ifdef USE_TAS5825M_DAC
  static constexpr uint8_t CBCW_CH1_I = static_cast<uint8_t>(1u << 5);
  static constexpr uint8_t CBCW_CH2_I = static_cast<uint8_t>(1u << 4);
  static constexpr uint8_t OTW_LEVEL4 = static_cast<uint8_t>(1u << 3);
  #endif

  static constexpr uint8_t OTW_LEVEL3 = static_cast<uint8_t>(1u << 2);
  static constexpr uint8_t OTW_LEVEL3_I = OTW_LEVEL3;

  #ifdef USE_TAS5825M_DAC
  static constexpr uint8_t OTW_LEVEL2 = static_cast<uint8_t>(1u << 1);
  static constexpr uint8_t OTW_LEVEL1 = static_cast<uint8_t>(1u << 0);
  #endif

  #ifdef USE_TAS5825M_DAC
  if (this->left_channel_cbc_current_warning_binary_sensor_ != nullptr) {
    this->left_channel_cbc_current_warning_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {this->left_channel_cbc_current_warning_binary_sensor_, WARNING_OFFSET, CBCW_CH1_I};
  }
  if (this->right_channel_cbc_current_warning_binary_sensor_ != nullptr) {
    this->right_channel_cbc_current_warning_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {this->right_channel_cbc_current_warning_binary_sensor_, WARNING_OFFSET, CBCW_CH2_I};
  }
  if (this->over_temperature_146c_warning_binary_sensor_ != nullptr) {
    this->over_temperature_146c_warning_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {this->over_temperature_146c_warning_binary_sensor_, WARNING_OFFSET, OTW_LEVEL4};
  }
  #endif

  if (this->over_temperature_134c_warning_binary_sensor_ != nullptr) {
    this->over_temperature_134c_warning_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {this->over_temperature_134c_warning_binary_sensor_, WARNING_OFFSET, OTW_LEVEL3};
  }

  #ifdef USE_TAS5825M_DAC
  if (this->over_temperature_122c_warning_binary_sensor_ != nullptr) {
    this->over_temperature_122c_warning_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {this->over_temperature_122c_warning_binary_sensor_, WARNING_OFFSET, OTW_LEVEL2};
  }

  if (this->over_temperature_112c_warning_binary_sensor_ != nullptr) {
    this->over_temperature_112c_warning_binary_sensor_->publish_initial_state(false);
    this->active_fault_sensors_[this->active_fault_sensor_count_++] =
        {this->over_temperature_112c_warning_binary_sensor_, WARNING_OFFSET, OTW_LEVEL1};
  }
  #endif
}

// if no binary sensors are defined faults registers are never cleared
bool Tas58xxComponent::clear_fault_registers_() {
  if (!this->tas58xx_write_byte_(TAS58XX_FAULT_CLEAR, TAS58XX_ANALOG_FAULT_CLEAR)) return false;
  ESP_LOGD(TAG, "Fault registers cleared");
  return true;
}
#endif

//// low level functions

bool Tas58xxComponent::i2s_sync_(size_t* bytes_written, size_t* sync_attempts) {
// runs in setup() at HARDWARE priority
// should execute and complete before any other component's loop() exists
// and therefore before any other component open's i2s channel
// calls i2s_open_channel() and i2s_close_channel()

  if (!this->i2s_open_channel_()) {
    // i2s_open_channel_() has already cleaned up
    return false;
  }

  static constexpr size_t NUMBER_SYNC_BYTES = 16;

  // 4 frames of silence at 16-bit stereo = 4 * 2 channels * 2 bytes = 16 bytes
  // used for toggling BCLK/LRCLK so the DAC sees a valid clock before
  // CTRL_STATE -> Play transition
  static constexpr uint8_t I2S_SYNC_SILENCE[NUMBER_SYNC_BYTES] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

  static constexpr size_t ATTEMPT_TIMEOUT_MS = 1;
  static constexpr size_t MAX_ATTEMPTS = 10;
  // 20ms worst case -- speaker component uses 60ms but in a dedicated FreeRTOS task
  // esp32 completes in 5 attempts => 5ms

  esp_err_t err = ESP_FAIL;
  size_t attempt_counter = 1;

  for (; attempt_counter <= MAX_ATTEMPTS; attempt_counter++) {
    err = i2s_channel_write(this->sync_tx_handle_, I2S_SYNC_SILENCE, NUMBER_SYNC_BYTES,
                             bytes_written, pdMS_TO_TICKS(ATTEMPT_TIMEOUT_MS));

    if (err == ESP_ERR_TIMEOUT && *bytes_written == 0) {
      continue;  // clock still settling -- retry, not a real failure yet
    }
    break;
  }

  bool sync_successful = (err == ESP_OK);

  if (sync_successful) {
    if (*bytes_written == NUMBER_SYNC_BYTES) {
      ESP_LOGD(TAG, "I2S Sync successful: wrote %zu bytes (attempt:%zu)", *bytes_written, attempt_counter);
    } else {
      ESP_LOGW(TAG, "I2S Sync successful but incomplete: wrote %zu of %zu bytes (attempt:%zu)",
                *bytes_written, NUMBER_SYNC_BYTES, attempt_counter);
    }
  } else {
    if (attempt_counter > MAX_ATTEMPTS) {
      ESP_LOGE(TAG, "I2S Sync failed after maximum %zu attempts", MAX_ATTEMPTS);
    } else {
    ESP_LOGE(TAG, "I2S Sync failed with error:%s but wrote %zu bytes (attempt:%zu)",
              esp_err_to_name(err), *bytes_written, attempt_counter);
    }
  }

  *sync_attempts = attempt_counter;
  this->i2s_close_channel_();
  return sync_successful;
}

bool Tas58xxComponent::i2s_open_channel_() {
  // check anyway though not expected to actually fail
  if (!this->parent_->try_lock()) {
    return false;
  }

  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(
      this->parent_->get_port(), I2S_ROLE_MASTER);
  esp_err_t err = i2s_new_channel(&chan_cfg, &this->sync_tx_handle_, nullptr);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "I2S New Channel failed error: %s", esp_err_to_name(err));
    this->sync_tx_handle_ = nullptr;
    this->parent_->unlock();
    return false;
  }

  i2s_std_gpio_config_t pin_cfg = this->parent_->get_pin_config();
  pin_cfg.dout = this->dout_pin_;  // use YAML configured dout

  i2s_std_clk_config_t clk_cfg = {
      .sample_rate_hz = 96000,
      .clk_src = I2S_CLK_SRC_DEFAULT,
      .mclk_multiple = I2S_MCLK_MULTIPLE_256,
  };
  i2s_std_slot_config_t slot_cfg =
      I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  i2s_std_config_t std_cfg = {.clk_cfg = clk_cfg, .slot_cfg = slot_cfg, .gpio_cfg = pin_cfg};

  err = i2s_channel_init_std_mode(this->sync_tx_handle_, &std_cfg);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "I2S Channel Init Std Mode failed error: %s", esp_err_to_name(err));
    i2s_del_channel(this->sync_tx_handle_);
    this->sync_tx_handle_ = nullptr;
    this->parent_->unlock();
    return false;
  }

  err = i2s_channel_enable(this->sync_tx_handle_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "I2S Channel Enable failed error: %s", esp_err_to_name(err));
    i2s_del_channel(this->sync_tx_handle_);
    this->sync_tx_handle_ = nullptr;
    this->parent_->unlock();
    return false;
  }

  return true;
}

void Tas58xxComponent::i2s_close_channel_() {
  if (this->sync_tx_handle_ != nullptr) {
    i2s_channel_disable(this->sync_tx_handle_);
    i2s_del_channel(this->sync_tx_handle_);
    this->sync_tx_handle_ = nullptr;

    // detach dout from the GPIO matrix and drive it low
    // since i2s_del_channel() does not undo esp_rom_gpio_connect_out_signal()
    // without the following calls the i2s channel's last output state keeps driving the pin
    gpio_reset_pin(this->dout_pin_);
    gpio_set_direction(this->dout_pin_, GPIO_MODE_OUTPUT);
    gpio_set_level(this->dout_pin_, 0);
  }
  this->parent_->unlock();  // unconditional — always attempts release, matches built-in speaker component
}

// use only when writing bytes to contiguous addresses
bool Tas58xxComponent:: book_page_write_bytes_(uint8_t book, uint8_t page, uint8_t sub_addr, uint8_t* data, uint8_t number_bytes) {
  if (!this->set_book_and_page_(book, page)) return false;
  if (!this->tas58xx_write_bytes_(sub_addr, data, number_bytes)) return false;

  // reset book and page to zero
  return this->set_book_and_page_(TAS58XX_BOOK_ZERO, TAS58XX_PAGE_ZERO);
}

bool Tas58xxComponent::set_book_and_page_(uint8_t book, uint8_t page) {
  if (this->tas58xx_write_byte_(TAS58XX_PAGE_SET, TAS58XX_PAGE_ZERO)) {
    if (this->tas58xx_write_byte_(TAS58XX_BOOK_SET, book)) {
      if (this->tas58xx_write_byte_(TAS58XX_PAGE_SET, page)) {
        return true;
      }
    }
  }
  ESP_LOGD(TAG, "%s setting book:0x%02X page:0x%02X", ERROR, book, page);
  return false;
}

bool Tas58xxComponent::tas58xx_read_bytes_(uint8_t a_register, uint8_t* data, uint8_t number_bytes) {
  i2c::ErrorCode error_code;
  error_code = this->write(&a_register, 1);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "%s code:%d writing address:0x%02X to start read", ERROR, error_code, a_register);
    this->i2c_error_ = static_cast<int>(error_code);
    return false;
  }
  error_code = this->read_register(a_register, data, number_bytes);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "%s code:%d reading %d bytes from address:0x%02X", ERROR, error_code, number_bytes, a_register);
    this->i2c_error_ = static_cast<int>(error_code);
    return false;
  }
  return true;
}

bool Tas58xxComponent::tas58xx_write_byte_(uint8_t a_register, uint8_t data) {
  i2c::ErrorCode error_code = this->write_register(a_register, &data, 1);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "%s code:%d writing to address:0x%02X", ERROR, error_code, a_register);
    this->i2c_error_ = static_cast<int>(error_code);
    return false;
  }
  return true;
}

bool Tas58xxComponent::tas58xx_write_bytes_(uint8_t a_register, uint8_t* data, uint8_t number_bytes) {
  i2c::ErrorCode error_code = this->write_register(a_register, data, number_bytes);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "%s code:%d writing address:0x%02X bytes:%d", ERROR, error_code, a_register, number_bytes);
    this->i2c_error_ = static_cast<int>(error_code);
    return false;
  }
  return true;
}

}  // namespace esphome::tas58xx
