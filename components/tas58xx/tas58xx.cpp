#include "tas58xx.h"
#include "tas58xx_minimal.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"
#include <cmath>

namespace esphome::tas58xx {

#ifdef USE_TAS5805M_DAC
static const char *const TAG               = "tas58xx";
#else
static const char *const TAG               = "tas5825m";
#endif

static const char *const ERROR             = "Error";
static const char *const MIXER_MODE        = "Mixer Mode";
static const char *const EQ_BAND           = "EQ Band";

static const uint8_t TAS58XX_MUTE_CONTROL = 0x08;  // LR Channel Mute
static const uint8_t REMOVE_CLOCK_FAULT    = 0xFB;  // used to zero clock fault bit of global_fault1 register

// maximum delay allowed in "tas58xx_minimal.h" used in configure_registers()
static const uint8_t ESPHOME_MAXIMUM_DELAY = 5;     // milliseconds

// initial delay in 'loop' before writing eq gains to ensure on boot sound has
// played and tas58xx has detected i2s clock
static const uint8_t DELAY_LOOPS           = 20;    // 20 loop iterations ~ 320ms

// initial ms delay before starting fault updates
static const uint16_t INITIAL_UPDATE_DELAY = 4000;

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

  // rescale -103db to 24db digital volume range to register digital volume range 254 to 0
  this->tas58xx_raw_volume_max_ = (uint8_t)((this->tas58xx_volume_max_ - 24) * -2);
  this->tas58xx_raw_volume_min_ = (uint8_t)((this->tas58xx_volume_min_ - 24) * -2);
}

bool Tas58xxComponent::configure_registers_() {
  uint16_t i = 0;
  uint16_t counter = 0;
  uint16_t number_configurations = sizeof(TAS58XX_REGISTERS) / sizeof(TAS58XX_REGISTERS[0]);

  while (i < number_configurations) {
    switch (TAS58XX_REGISTERS[i].offset) {
      case TAS58XX_CFG_META_DELAY:
        if (TAS58XX_REGISTERS[i].value > ESPHOME_MAXIMUM_DELAY) return false;
        delay(TAS58XX_REGISTERS[i].value);
        break;
      default:
        if (!this->tas58xx_write_byte_(TAS58XX_REGISTERS[i].offset, TAS58XX_REGISTERS[i].value)) return false;
        counter++;
        break;
    }
    i++;
  }
  this->number_registers_configured_ = counter;

  // enable Tas58xx
  if (!this->set_deep_sleep_off_()) return false;

  // only setup once here
  if (!this->set_dac_mode_(this->tas58xx_dac_mode_)) return false;

  // note: setup of mixer mode deferred to 'loop'

  if (!this->set_analog_gain_(this->tas58xx_analog_gain_)) return false;

  if (!this->set_state_(CTRL_PLAY)) return false;

  // initialise to now
  this->start_time_ = App.get_loop_component_start_time();
  return true;
}

void Tas58xxComponent::loop() {
  // 'play_file' is initiated by YAML on_boot with priority 220.0f
  // 'refresh_eq_settings' is triggered by Number 'left_eq_gain_16000hz' or right_eq_gain_16000hz or Select 'eq_mode'
  // each with setup priority AFTER_CONNECTION = 100.0f
  // when tas58xx has detected i2s clock then EQ settings can be written within 'loop'

  // initially loop_setup_stage_ = WAIT_FOR_TRIGGER

  switch (this->loop_setup_stage_) {
    case WAIT_FOR_TRIGGER:
      return;

    case RUN_DELAY_LOOP:
      // wait ensures on boot sound has played and tas58xx has detected i2s clock
      if (this->loop_counter_ < DELAY_LOOPS) {    // loop_count was initialised to 0
        this->loop_counter_++;
        return;
      }
      this->loop_setup_stage_ = INPUT_MIXER_SETUP;
      return;

    case INPUT_MIXER_SETUP:
      ESP_LOGD(TAG, "Setup Mixer Gains");
      if (!this->set_mixer_mode(this->tas58xx_mixer_mode_)) {
        // show warning but continue as if mixer mode was set ok
        ESP_LOGW(TAG, "%s setting Mixer Mode: %s", ERROR, MIXER_MODE);
      }
      this->loop_setup_stage_ = LR_VOLUME_SETUP;
      return;

    case LR_VOLUME_SETUP:
#ifdef USE_TAS58XX_CHANNEL_GAINS
      ESP_LOGD(TAG, "Setup Channel Gains");
      if (!this->set_channel_gain(LEFT_CHANNEL, this->tas58xx_channel_gain_[LEFT_CHANNEL])) {
        // show warning but continue as if left channel gain was set ok
        ESP_LOGW(TAG, "%s setting Left Channel Gain: %ddb", ERROR, this->tas58xx_channel_gain_[LEFT_CHANNEL]);
      }

      if (!this->set_channel_gain(RIGHT_CHANNEL, this->tas58xx_channel_gain_[RIGHT_CHANNEL])) {
        // show warning but continue as if right channel gain was set ok
        ESP_LOGW(TAG, "%ssetting Right Channel Gain: %ddb", ERROR, this->tas58xx_channel_gain_[RIGHT_CHANNEL]);
      }
#endif

#ifdef USE_TAS58XX_EQ_GAINS
      this->loop_setup_stage_ = EQ_BANDS_SETUP;
#endif

#ifdef USE_TAS58XX_EQ_PRESETS
      this->loop_setup_stage_ = EQ_PRESETS_SETUP;
#endif

      // if loop_setup_stage_ has not changed then no EQ to setup
      if (this->loop_setup_stage_ == LR_VOLUME_SETUP) this->loop_setup_stage_ = SETUP_COMPLETE;
      return;

    case EQ_BANDS_SETUP:
      // write gains one eq band per 'loop' so component does not take too long in 'loop'
      if (this->refresh_band_ == NUMBER_EQ_BANDS) {     // refresh_band_ was initialised to 0
        // finished writing all bands
        this->loop_setup_stage_ = SETUP_COMPLETE;
        this->refresh_band_ = 0;
        return;
      }

      // write Left gains of current band and increment to next band ready for when loop next runs
      ESP_LOGD(TAG, "Setup Left Channel EQ Band %d Gain", this->refresh_band_);
      if (!this->set_eq_gain(LEFT_CHANNEL, this->refresh_band_, this->tas58xx_eq_gain_[LEFT_CHANNEL][this->refresh_band_])) {
        // show warning but continue as if eq gain was set ok
  #ifdef USE_TAS58XX_EQ_BIAMP
        ESP_LOGW(TAG, "%s setting Left EQ Band %d Gain", ERROR, this->refresh_band_);
  #else
        ESP_LOGW(TAG, "%s setting EQ Band %d Gain", ERROR, this->refresh_band_);
  #endif
      }

  #ifdef USE_TAS58XX_EQ_BIAMP
      // write Right gains of current band and increment to next band ready for when loop next runs
      ESP_LOGD(TAG, "Set up Right Channel EQ Band %d Gain", this->refresh_band_);
      if (!this->set_eq_gain(RIGHT_CHANNEL, this->refresh_band_, this->tas58xx_eq_gain_[RIGHT_CHANNEL][this->refresh_band_])) {
        // show warning but continue as if eq gain was set ok
        ESP_LOGW(TAG, "%s setting Right EQ Band %d Gain", ERROR, this->refresh_band_);
      }
  #endif
      // progress to next band
      this->refresh_band_++;
      return;

    case EQ_PRESETS_SETUP:
      ESP_LOGD(TAG, "Setup Channel Presets");
      if (!this->set_eq_preset(LEFT_CHANNEL, this->tas58xx_channel_preset_[LEFT_CHANNEL])) {
        ESP_LOGW(TAG, "%s setting Left Channel Preset using index: %d", ERROR, this->tas58xx_channel_preset_[LEFT_CHANNEL]);
      }
      if (!this->set_eq_preset(RIGHT_CHANNEL, this->tas58xx_channel_preset_[RIGHT_CHANNEL])) {
        ESP_LOGW(TAG, "%s setting Right Channel Preset using index: %d", ERROR, this->tas58xx_channel_preset_[RIGHT_CHANNEL]);
      }
      this->loop_setup_stage_ = SETUP_COMPLETE;
      return;

    case SETUP_COMPLETE:
      this->disable_loop(); // requires Esphome 2025.7.0
      return;
  }
}


void Tas58xxComponent::update() {
  // initial delay before proceeding with updates
  if (!this->update_delay_finished_) {
    const uint32_t current_time = App.get_loop_component_start_time();
    this->update_delay_finished_ = ((current_time - this->start_time_) > INITIAL_UPDATE_DELAY);

    if (!this->update_delay_finished_) return;

    // finished delay so clear faults
    if (!this->tas58xx_write_byte_(TAS58XX_FAULT_CLEAR, TAS58XX_ANALOG_FAULT_CLEAR)) {
      ESP_LOGW(TAG, "%s initialising faults", ERROR);
    }

    // publish all binary sensors as false on first update
#ifdef USE_TAS58XX_BINARY_SENSOR
    this->publish_faults_();
#endif

    // read and process faults from next update
    return;
  }

  // if there was a fault last update then clear any faults
  if (this->is_fault_to_clear_) {
    if (!this->clear_fault_registers_()) {
      ESP_LOGW(TAG, "%s clearing faults", ERROR);
    }
  }

  if (!this->read_fault_registers_()) {
    ESP_LOGW(TAG, "%s reading faults", ERROR);
    return;
  }

  // is there a fault that should be cleared next update
  this->is_fault_to_clear_ =
     ( this->tas58xx_faults_.is_fault_except_clock_fault || (this->tas58xx_faults_.clock_fault && (!this->ignore_clock_faults_when_clearing_faults_)) );


  // if no change in faults bypass publishing
  if ( !(this->is_new_common_fault_ || this->is_new_over_temperature_issue_ || this->is_new_channel_fault_ || this->is_new_global_fault_) ) return;

#ifdef USE_TAS58XX_BINARY_SENSOR
  this->publish_faults_();
#endif
}

#ifdef USE_TAS58XX_BINARY_SENSOR
void Tas58xxComponent::publish_faults_() {
  if (this->is_new_common_fault_) {
    if (this->have_fault_binary_sensor_ != nullptr) {
      this->have_fault_binary_sensor_->publish_state(this->tas58xx_faults_.have_fault);
    }

    if (this->clock_fault_binary_sensor_ != nullptr) {
      this->clock_fault_binary_sensor_->publish_state(this->tas58xx_faults_.clock_fault);
    }
  }

  if (this->is_new_over_temperature_issue_) {
    if (this->over_temperature_shutdown_fault_binary_sensor_ != nullptr) {
      this->over_temperature_shutdown_fault_binary_sensor_->publish_state(this->tas58xx_faults_.temperature_fault);
    }

    if (this->over_temperature_warning_binary_sensor_ != nullptr) {
      this->over_temperature_warning_binary_sensor_->publish_state(this->tas58xx_faults_.temperature_warning);
    }
  }

  // publish channel and global faults in separate loop iterations to spread component time when publishing binary sensors
  if (this->is_new_channel_fault_) {
    this->set_timeout("", 15, [this]() { this->publish_channel_faults_(); });
  }
  else {
    if (this->is_new_global_fault_) {
      this->set_timeout("", 15, [this]() { this->publish_global_faults_(); });
    }
  }
}

void Tas58xxComponent::publish_channel_faults_() {
  if (this->right_channel_over_current_fault_binary_sensor_ != nullptr) {
    this->right_channel_over_current_fault_binary_sensor_->publish_state(this->tas58xx_faults_.channel_fault & (1 << 0));
  }

  if (this->left_channel_over_current_fault_binary_sensor_ != nullptr) {
    this->left_channel_over_current_fault_binary_sensor_->publish_state(this->tas58xx_faults_.channel_fault & (1 << 1));
  }

  if (this->right_channel_dc_fault_binary_sensor_ != nullptr) {
    this->right_channel_dc_fault_binary_sensor_->publish_state(this->tas58xx_faults_.channel_fault & (1 << 2));
  }

  if (this->left_channel_dc_fault_binary_sensor_ != nullptr) {
    this->left_channel_dc_fault_binary_sensor_->publish_state(this->tas58xx_faults_.channel_fault & (1 << 3));
  }

  if (this->is_new_global_fault_) {
      this->set_timeout("", 15, [this]() { this->publish_global_faults_(); });
  }
}


void Tas58xxComponent::publish_global_faults_() {
  if (this->pvdd_under_voltage_fault_binary_sensor_ != nullptr) {
    this->pvdd_under_voltage_fault_binary_sensor_->publish_state(this->tas58xx_faults_.global_fault & (1 << 0));
  }

  if (this->pvdd_over_voltage_fault_binary_sensor_ != nullptr) {
    this->pvdd_over_voltage_fault_binary_sensor_->publish_state(this->tas58xx_faults_.global_fault & (1 << 1));
  }

  if (this->bq_write_failed_fault_binary_sensor_ != nullptr) {
    this->bq_write_failed_fault_binary_sensor_->publish_state(this->tas58xx_faults_.global_fault & (1 << 6));
  }

  if (this->otp_crc_check_error_binary_sensor_ != nullptr) {
    this->otp_crc_check_error_binary_sensor_->publish_state(this->tas58xx_faults_.global_fault & (1 << 7));
  }
}
#endif

void Tas58xxComponent::dump_config() {
#ifdef USE_TAS5805M_DAC
  ESP_LOGCONFIG(TAG, "Tas58xx Audio Dac:");
#else
  ESP_LOGCONFIG(TAG, "Tas5825m Audio Dac:");
#endif

  switch (this->error_code_) {
    case CONFIGURATION_FAILED:
      ESP_LOGE(TAG, "  %s setup failed: %i", ERROR, this->i2c_error_);
      break;
    case NONE:
      LOG_I2C_DEVICE(this);
      LOG_PIN("  Enable Pin: ", this->enable_pin_);
      ESP_LOGCONFIG(TAG,
              "  Registers Configured: %i\n"
              "  Analog Gain: %3.1fdB\n"
              "  DAC Mode: %s\n"
              "  Mixer Mode: %s\n"
              "  Volume Maximum: %idB\n"
              "  Volume Minimum: %idB\n"
              "  Ignore Fault: %s\n"
              "  Refresh EQ: %s\n",
              this->number_registers_configured_, this->tas58xx_analog_gain_,
              this->tas58xx_dac_mode_ ? "PBTL" : "BTL",
              MIXER_MODE_TEXT[this->tas58xx_mixer_mode_],
              this->tas58xx_volume_max_, this->tas58xx_volume_min_,
              this->ignore_clock_faults_when_clearing_faults_ ? "CLOCK FAULTS" : "NONE",
              this->eq_refresh_ ? "MANUAL" : "AUTO"
              );
      LOG_UPDATE_INTERVAL(this);
      break;
  }

#ifdef USE_TAS58XX_BINARY_SENSOR
  ESP_LOGCONFIG(TAG, "Tas58xx Binary Sensors:");
  LOG_BINARY_SENSOR("  ", "Any Faults", this->have_fault_binary_sensor_);
  ESP_LOGCONFIG(TAG, "    Exclude: %s", this->exclude_clock_fault_from_have_faults_ ? "CLOCK FAULTS" : "NONE");

  LOG_BINARY_SENSOR("  ", "Right Channel Over Current", this->right_channel_over_current_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Left Channel Over Current", this->left_channel_over_current_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Right Channel DC Fault", this->right_channel_dc_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Left Channel DC Fault", this->left_channel_dc_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "PVDD Under Voltage", this->pvdd_under_voltage_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "PVDD Over Voltage", this->pvdd_over_voltage_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Clock Fault", this->clock_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "BQ Write Failed", this->bq_write_failed_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "OTP CRC Check Error", this->otp_crc_check_error_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Over Temperature Shutdown", this->over_temperature_shutdown_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Over Temperature Warning", this->over_temperature_warning_binary_sensor_);
#endif
}


// public

// used by 'enable_dac_switch'
void Tas58xxComponent::enable_dac(bool enable) {
  enable ? this->set_deep_sleep_off_() : this->set_deep_sleep_on_();
}

// used by select eq mode
uint8_t Tas58xxComponent::get_configured_eq_mode() {
  return static_cast<uint8_t>(this->configured_eq_mode_);
}

// used by select eq mode
void Tas58xxComponent::select_eq_mode(uint8_t select_index) {
  if ( select_index == static_cast<uint8_t>(EqMode::EQ_OFF) ) {
    this->set_eq_mode_(EqMode::EQ_OFF);
  } else {
    this->set_eq_mode_(this->configured_eq_mode_);
  }
}

bool Tas58xxComponent::set_eq_preset(Channels channel, uint8_t select_preset) {
#ifdef USE_TAS58XX_EQ_PRESETS
  if (select_preset > EQ_PROFILE_MAXIMUM_INDEX) {
    ESP_LOGE(TAG, "Invalid %s Channel Preset index: %d", EQ_CHANNEL_TEXT[channel], select_preset);
    return false;
  }

  if (this->loop_setup_stage_ < EQ_PRESETS_SETUP) {
    ESP_LOGD(TAG, "Save %s Channel EQ Preset index >> %d", EQ_CHANNEL_TEXT[channel], select_preset);
    this->tas58xx_channel_preset_[channel] = select_preset;
    return true;
  }

  ESP_LOGD(TAG, "Set %s Channel EQ Preset index >> %d", EQ_CHANNEL_TEXT[channel], select_preset);


#ifdef USE_TAS5805M_DAC
  const AddressSequence* biquad1_address = (channel == LEFT_CHANNEL) ? &TAS5805M_LEFT_EQ_ADDRESS[0] : &TAS5805M_RIGHT_EQ_ADDRESS[0];
  const AddressSequence* biquad2_address = (channel == LEFT_CHANNEL) ? &TAS5805M_LEFT_EQ_ADDRESS[1] : &TAS5805M_RIGHT_EQ_ADDRESS[1];
  const AddressSequence* biquad3_address = (channel == LEFT_CHANNEL) ? &TAS5805M_LEFT_EQ_ADDRESS[2] : &TAS5805M_RIGHT_EQ_ADDRESS[2];
#else
  const AddressSequence* biquad1_address = (channel == LEFT_CHANNEL) ? &TAS5825M_LEFT_EQ_ADDRESS[0] : &TAS5825M_RIGHT_EQ_ADDRESS[0];
  const AddressSequence* biquad2_address = (channel == LEFT_CHANNEL) ? &TAS5825M_LEFT_EQ_ADDRESS[1] : &TAS5825M_RIGHT_EQ_ADDRESS[1];
  const AddressSequence* biquad3_address = (channel == LEFT_CHANNEL) ? &TAS5825M_LEFT_EQ_ADDRESS[2] : &TAS5825M_RIGHT_EQ_ADDRESS[2];
#endif

  const BiquadSequence* biquad1 = (channel == LEFT_CHANNEL) ? &EQ_PROFILE_LEFT_COEFFICIENTS[select_preset][0] : &EQ_PROFILE_RIGHT_COEFFICIENTS[select_preset][0];
  const BiquadSequence* biquad2 = (channel == LEFT_CHANNEL) ? &EQ_PROFILE_LEFT_COEFFICIENTS[select_preset][1] : &EQ_PROFILE_RIGHT_COEFFICIENTS[select_preset][1];
  const BiquadSequence* biquad3 = (channel == LEFT_CHANNEL) ? &EQ_PROFILE_LEFT_COEFFICIENTS[select_preset][2] : &EQ_PROFILE_RIGHT_COEFFICIENTS[select_preset][2];

  if ((biquad1_address == NULL) || (biquad2_address == NULL) || (biquad3_address == NULL)) {
    ESP_LOGE(TAG, "NULL EQ Preset Address pointer");
    return false;
  }

  if ((biquad1 == NULL) || (biquad2 == NULL) || (biquad3 == NULL)) {
    ESP_LOGE(TAG, "NULL EQ Preset Coefficent pointer");
    return false;
  }

  if (!this->book_and_page_write_(TAS58XX_EQ_BOOK, biquad1_address->page, biquad1_address->sub_addr,
                                  reinterpret_cast<uint8_t *>(const_cast<uint8_t *>(biquad1->coefficients)), BIQUAD_SIZE)) {
  // if (!this->write_biquad_coefficients_(biquad1_address->page, biquad1_address->sub_addr, reinterpret_cast<uint8_t *>(const_cast<uint8_t *>(biquad1->coefficients)))) {
    ESP_LOGE(TAG, "%s writing Biquad 1 for %s Channel EQ Preset index: %d", ERROR, EQ_CHANNEL_TEXT[channel], select_preset);
  }
  if (!this->book_and_page_write_(TAS58XX_EQ_BOOK, biquad2_address->page, biquad2_address->sub_addr,
                                  reinterpret_cast<uint8_t *>(const_cast<uint8_t *>(biquad2->coefficients)), BIQUAD_SIZE)) {
  // if (!this->write_biquad_coefficients_(biquad2_address->page, biquad2_address->sub_addr, reinterpret_cast<uint8_t *>(const_cast<uint8_t *>(biquad2->coefficients)))) {
    ESP_LOGE(TAG, "%s writing Biquad 2 for %s Channel EQ Preset index: %d", ERROR, EQ_CHANNEL_TEXT[channel], select_preset);
  }
  if (!this->book_and_page_write_(TAS58XX_EQ_BOOK, biquad3_address->page, biquad3_address->sub_addr,
                                  reinterpret_cast<uint8_t *>(const_cast<uint8_t *>(biquad3->coefficients)), BIQUAD_SIZE)) {
  // if (!this->write_biquad_coefficients_(biquad3_address->page, biquad3_address->sub_addr, reinterpret_cast<uint8_t *>(const_cast<uint8_t *>(biquad3->coefficients)))) {
    ESP_LOGE(TAG, "%s writing Biquad 3 for %s Channel EQ Preset index: %d", ERROR, EQ_CHANNEL_TEXT[channel], select_preset);
  }

  this->tas58xx_channel_preset_[channel] = select_preset;
#endif
  return true;
}

// used by eq gain numbers

bool Tas58xxComponent::set_eq_gain(Channels channel, uint8_t band, int8_t gain) {
#ifdef USE_TAS58XX_EQ_GAINS
  if (band < 0 || band >= NUMBER_EQ_BANDS) {
    ESP_LOGE(TAG, "Invalid %s%d", EQ_BAND, band);
    return false;
  }
  if (gain < TAS58XX_EQ_MIN_DB || gain > TAS58XX_EQ_MAX_DB) {
    ESP_LOGE(TAG, "Invalid %s%d Gain: %ddB", EQ_BAND, band, gain);
    return false;
  }

  if (this->loop_setup_stage_ < EQ_BANDS_SETUP) {
    ESP_LOGD(TAG, "Save %s Channel %s:%d Gain >> %ddB", EQ_CHANNEL_TEXT[channel], EQ_BAND, band, gain);
    this->tas58xx_eq_gain_[channel][band] = gain;
    return true;
  }

  ESP_LOGD(TAG, "Set %s Channel %s:%d Gain >> %ddB", EQ_CHANNEL_TEXT[channel], EQ_BAND, band, gain);

  uint8_t x = (gain + TAS58XX_EQ_MAX_DB);

#ifdef USE_TAS5805M_DAC
  #ifdef USE_TAS58XX_EQ_BIAMP
  const AddressSequence* eq_address = (channel == LEFT_CHANNEL) ? &TAS5805M_LEFT_EQ_ADDRESS[band] : &TAS5805M_RIGHT_EQ_ADDRESS[band];
  #else
  const AddressSequence* eq_address = &TAS5805M_LEFT_EQ_ADDRESS[band];
  #endif
#else
  #ifdef USE_TAS58XX_EQ_BIAMP
  const AddressSequence* eq_address = (channel == LEFT_CHANNEL) ? &TAS5825M_LEFT_EQ_ADDRESS[band] : &TAS5825M_RIGHT_EQ_ADDRESS[band];
  #else
  const AddressSequence* eq_address = &TAS5825M_LEFT_EQ_ADDRESS[band];
  #endif
#endif

  const BiquadSequence* biquad = &EQ_BAND_COEFFICIENTS[x][band];

  if ((eq_address == NULL) || (biquad == NULL)) {
    ESP_LOGE(TAG, "%s NULL discovered: Band: %d Gain: %d",ERROR, band, gain);
    return false;
  }

  if (!this->book_and_page_write_(TAS58XX_EQ_BOOK, eq_address->page, eq_address->sub_addr,
                                  reinterpret_cast<uint8_t *>(const_cast<uint8_t *>(biquad->coefficients)), BIQUAD_SIZE)) {
    ESP_LOGE(TAG, "%s writing Biquad for %s Channel %s:%d Gain:%ddB", ERROR, EQ_CHANNEL_TEXT[channel], EQ_BAND, band, gain);
  }

  // if (!this->write_biquad_coefficients_(eq_address->page, eq_address->sub_addr, reinterpret_cast<uint8_t *>(const_cast<uint8_t *>(biquad->coefficients)))) {
  //   ESP_LOGE(TAG, "%s writing Biquad for %s Channel %s: %d Gain: %ddB", ERROR, EQ_CHANNEL_TEXT[channel], EQ_BAND, band, gain);
  // }
#endif
  return true;
}

int32_t Tas58xxComponent::gain_to_q9_23_(int8_t gain) {
  float linear = powf(10.0f, ((float)gain) / 20.0f);
  if (linear > TAS58XX_LINEAR_GAIN_MAX) linear = TAS58XX_LINEAR_GAIN_MAX;
  if (linear < TAS58XX_LINEAR_GAIN_MIN) linear = TAS58XX_LINEAR_GAIN_MIN;

  int32_t fixed_q9_23 = static_cast<int32_t>(linear * (1 << 23));
  int32_t little_endian = byteswap(fixed_q9_23);

  ESP_LOGV(TAG, "Gain:%ddb = Fixed 9.23 >> 0x%08X : Convert Endian >> 0x%08X", gain, fixed_q9_23, little_endian);
  return little_endian;
}

bool Tas58xxComponent::set_channel_gain(Channels channel, int8_t gain) {
#ifdef USE_TAS58XX_CHANNEL_GAINS
  if (gain < TAS58XX_CHANNEL_GAIN_MIN_DB || gain > TAS58XX_CHANNEL_GAIN_MAX_DB) {
    ESP_LOGE(TAG, "Invalid Gain:%ddB for %s Channel",  gain, EQ_CHANNEL_TEXT[channel]);
    return false;
  }

  // Channel Gains initially set by tas58xx number component setups
  if (this->loop_setup_stage_ < LR_VOLUME_SETUP) {
    ESP_LOGD(TAG, "Save %s Channel Gain >> %ddB", EQ_CHANNEL_TEXT[channel], gain);
    this->tas58xx_channel_gain_[channel] = gain;
    return true;
  }

  ESP_LOGD(TAG, "Set %s Channel Gain >> %ddB", EQ_CHANNEL_TEXT[channel], gain);

  // if (!this->set_book_and_page_(TAS58XX_MIXER_CHANNEL_GAINS_BOOK, TAS58XX_CHANNEL_GAIN_PAGE)) {
  //   ESP_LOGE(TAG, "%s Channel Gain: setting book and page", ERROR);
  //   return false;
  // }

  int32_t little_endian_9_23 = gain_to_q9_23_(gain);

  if (!this->book_and_page_write_(TAS58XX_MIXER_CHANNEL_GAINS_BOOK, TAS58XX_CHANNEL_GAIN_PAGE, TAS58XX_CHANNEL_GAIN_OFFSET[channel],
                                  reinterpret_cast<uint8_t *>(&little_endian_9_23), sizeof(little_endian_9_23))) {
  // if (!this->tas58xx_write_bytes_(TAS58XX_CHANNEL_GAIN_OFFSET[channel], reinterpret_cast<uint8_t *>(&little_endian_9_23), 4)) {
    ESP_LOGE(TAG, "%s writing %s Channel Gain:%ddb", ERROR, EQ_CHANNEL_TEXT[channel], gain);
    return false;
  }

  // always change back to book zero and page zero
//   return this->set_book_and_page_(TAS58XX_BOOK_ZERO, TAS58XX_PAGE_ZERO);
// #else
  return true;
#endif
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

// used by 'left_gain_band16000hz' or 'right_gain_band16000hz' or 'select eq_mode'
// to trigger loop setup
void Tas58xxComponent::refresh_eq_settings() {
  if (this->loop_setup_stage_ == WAIT_FOR_TRIGGER) {
    this->loop_setup_stage_ = RUN_DELAY_LOOP;
  }
  return;
}

// used by fault sensor
uint32_t Tas58xxComponent::times_faults_cleared() {
  return this->times_faults_cleared_;
}

// used by 'select eq mode' to determine initially selected EQ mode
bool Tas58xxComponent::is_eq_configured() {
  return this->eq_configured_;
}

// used by 'left_gain_band16000hz' or 'right_gain_band16000hz' or 'select eq_mode'
bool Tas58xxComponent::using_auto_eq_refresh() {
  return (this->eq_refresh_ == EqRefreshMode::AUTO);
}

// used by 'enable_eq_switch' or 'select eq_mode'
bool Tas58xxComponent::using_manual_eq_refresh() {
  return (this->eq_refresh_ == EqRefreshMode::MANUAL);
}

float Tas58xxComponent::volume() {
  uint8_t raw_volume;
  this->get_digital_volume_(&raw_volume);
  return remap<float, uint8_t>(raw_volume, this->tas58xx_raw_volume_min_, this->tas58xx_raw_volume_max_, 0.0f, 1.0f);
}

bool Tas58xxComponent::set_volume(float volume) {
  float new_volume = clamp(volume, 0.0f, 1.0f);
  uint8_t raw_volume = remap<uint8_t, float>(new_volume, 0.0f, 1.0f, this->tas58xx_raw_volume_min_, this->tas58xx_raw_volume_max_);
  if (!this->set_digital_volume_(raw_volume)) return false;
  #if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
    int8_t dB = -(raw_volume / 2) + 24;
    ESP_LOGV(TAG, "Volume: >> %idB", dB);
  #endif
  return true;
}

// protected

bool Tas58xxComponent::get_analog_gain_(uint8_t* raw_gain) {
  uint8_t current;
  if (!this->tas58xx_read_byte_(TAS58XX_AGAIN, &current)) return false;
  // remove top 3 reserved bits
  *raw_gain = current & 0x1F;
  return true;
}

// Analog Gain Control , with 0.5dB one step
// lower 5 bits controls the analog gain.
// 00000: 0 dB (29.5V peak voltage)
// 00001: -0.5db
// 11111: -15.5 dB
// set analog gain in dB
bool Tas58xxComponent::set_analog_gain_(float gain_db) {
  if ((gain_db < TAS58XX_MIN_ANALOG_GAIN) || (gain_db > TAS58XX_MAX_ANALOG_GAIN)) return false;

  uint8_t new_again = static_cast<uint8_t>(-gain_db * 2.0);

  uint8_t current_again;
  if (!this->tas58xx_read_byte_(TAS58XX_AGAIN, &current_again)) return false;

  // keep top 3 reserved bits combine with bottom 5 analog gain bits
  new_again = (current_again & 0xE0) | new_again;
  if (!this->tas58xx_write_byte_(TAS58XX_AGAIN, new_again)) return false;

  ESP_LOGD(TAG, "Analog Gain >> %fdB", gain_db);
  return true;
}

bool Tas58xxComponent::get_dac_mode_(DacMode* mode) {
    uint8_t current_value;
    if (!this->tas58xx_read_byte_(TAS58XX_DEVICE_CTRL_1, &current_value)) return false;
    if (current_value & (1 << 2)) {
        *mode = PBTL;
    } else {
        *mode = BTL;
    }
    this->tas58xx_dac_mode_ = *mode;
    return true;
}

// only runs once from 'setup'
bool Tas58xxComponent::set_dac_mode_(DacMode mode) {
  uint8_t current_value;
  if (!this->tas58xx_read_byte_(TAS58XX_DEVICE_CTRL_1, &current_value)) return false;

  // Update bit 2 based on the mode
  if (mode == PBTL) {
      current_value |= (1 << 2);  // Set bit 2 to 1 (PBTL mode)
  } else {
      current_value &= ~(1 << 2); // Clear bit 2 to 0 (BTL mode)
  }
  if (!this->tas58xx_write_byte_(TAS58XX_DEVICE_CTRL_1, current_value)) return false;

  // 'tas58xx_state_' global already has dac mode from YAML config
  // save anyway so 'set_dac_mode' could be used more generally
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

bool Tas58xxComponent::get_digital_volume_(uint8_t* raw_volume) {
  uint8_t current = 254; // lowest raw volume
  if (!this->tas58xx_read_byte_(TAS58XX_DIG_VOL_CTRL, &current)) return false;
  *raw_volume = current;
  return true;
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
bool Tas58xxComponent::set_digital_volume_(uint8_t raw_volume) {
  if (!this->tas58xx_write_byte_(TAS58XX_DIG_VOL_CTRL, raw_volume)) return false;
  return true;
}


bool Tas58xxComponent::get_eq_mode_(EqMode* current_mode) {
  *current_mode = this->tas58xx_eq_mode_;
  return true;
}

bool Tas58xxComponent::set_eq_mode_(EqMode new_mode) {
#if defined(USE_TAS58XX_EQ_GAINS) || defined(USE_TAS58XX_EQ_PRESETS)
  if (this->tas58xx_eq_mode_ == new_mode) return true;

#ifdef USE_TAS5805M_DAC
  if (!this->tas58xx_write_byte_(TAS5805M_DSP_MISC, TAS5805M_CTRL_EQ[new_mode])) return false;
#else
  if (!this->set_book_and_page_(TAS5825M_EQ_CTRL_BOOK, TAS5825M_EQ_CTRL_PAGE)) {
    ESP_LOGE(TAG, "%s on book and page set for EQ control", ERROR);
    return false;
  }

  if (!this->tas58xx_write_bytes_(TAS5825M_GANG_EQ, reinterpret_cast<uint8_t *>(const_cast<uint32_t*>(&TAS5825M_CTRL_GANGED_EQ[new_mode])), COEFFICIENT_SIZE)) {
    ESP_LOGE(TAG, "%s writing EQ Ganged", ERROR);
    return false;
  }

  if (!this->tas58xx_write_bytes_(TAS5825M_BYPASS_EQ, reinterpret_cast<uint8_t *>(const_cast<uint32_t*>(&TAS5825M_CTRL_BYPASS_EQ[new_mode])), COEFFICIENT_SIZE)) {
    ESP_LOGE(TAG, "%s writing Bypass EQ", ERROR);
    return false;
  }
  if (!this->set_book_and_page_(TAS58XX_BOOK_ZERO, TAS58XX_PAGE_ZERO)) {
    ESP_LOGE(TAG, "%s on book and page reset", ERROR);
    return false;
  }
#endif

  this->tas58xx_eq_mode_ = new_mode;
  ESP_LOGD(TAG, "Set EQ mode >> %s", EQ_MODE_TEXT[new_mode]);
#endif
  return true;
}

uint8_t Tas58xxComponent::get_mixer_mode() {
  return static_cast<uint8_t>(this->tas58xx_mixer_mode_);
}

bool Tas58xxComponent::set_mixer_mode(MixerMode mode) {
  // save until eq refresh is triggered
  if (this->loop_setup_stage_ < INPUT_MIXER_SETUP) {
     ESP_LOGD(TAG, "Save %s >> %s", MIXER_MODE, MIXER_MODE_TEXT[mode]);
     this->tas58xx_mixer_mode_ = mode;
     return true;
  }

  // uint32_t mixer_l_to_l, mixer_r_to_r, mixer_l_to_r, mixer_r_to_l;

  // order of input mixer registers = Left to Left, Right to Left, Left to Right, Right to Right
  struct InputMixerCoefficients {
    uint32_t l_to_l;
    uint32_t r_to_l;
    uint32_t l_to_r;
    uint32_t r_to_r;
  };

  InputMixerCoefficients mixer_coefficients;

  switch (mode) {
    case STEREO:
      mixer_coefficients.l_to_l = TAS58XX_MIXER_VALUE_0DB;
      mixer_coefficients.r_to_l = TAS58XX_MIXER_VALUE_MUTE;
      mixer_coefficients.l_to_r = TAS58XX_MIXER_VALUE_MUTE;
      mixer_coefficients.r_to_r = TAS58XX_MIXER_VALUE_0DB;
      break;

    case STEREO_INVERSE:
      mixer_coefficients.l_to_l = TAS58XX_MIXER_VALUE_MUTE;
      mixer_coefficients.r_to_l = TAS58XX_MIXER_VALUE_0DB;
      mixer_coefficients.l_to_r = TAS58XX_MIXER_VALUE_0DB;
      mixer_coefficients.r_to_r = TAS58XX_MIXER_VALUE_MUTE;
      break;

    case MONO:
      mixer_coefficients.l_to_l = TAS58XX_MIXER_VALUE_MINUS6DB;
      mixer_coefficients.r_to_l = TAS58XX_MIXER_VALUE_MINUS6DB;
      mixer_coefficients.l_to_r = TAS58XX_MIXER_VALUE_MINUS6DB;
      mixer_coefficients.r_to_r = TAS58XX_MIXER_VALUE_MINUS6DB;
      break;

    case LEFT:
      mixer_coefficients.l_to_l = TAS58XX_MIXER_VALUE_0DB;
      mixer_coefficients.r_to_l = TAS58XX_MIXER_VALUE_MUTE;
      mixer_coefficients.l_to_r = TAS58XX_MIXER_VALUE_0DB;
      mixer_coefficients.r_to_r = TAS58XX_MIXER_VALUE_MUTE;
      break;

    case RIGHT:
      mixer_coefficients.l_to_l = TAS58XX_MIXER_VALUE_MUTE;
      mixer_coefficients.r_to_l = TAS58XX_MIXER_VALUE_0DB;
      mixer_coefficients.l_to_r = TAS58XX_MIXER_VALUE_MUTE;
      mixer_coefficients.r_to_r = TAS58XX_MIXER_VALUE_0DB;
      break;

    default:
      ESP_LOGW(TAG, "Invalid %s", MIXER_MODE);
      return false;
  }

  // if (!this->set_book_and_page_(TAS58XX_MIXER_CHANNEL_GAINS_BOOK, TAS58XX_MIXER_GAIN_PAGE)) {
  //   ESP_LOGE(TAG, "%s begin Set %s", ERROR, MIXER_MODE);
  //   return false;
  // }

  if (!this->book_and_page_write_(TAS58XX_MIXER_CHANNEL_GAINS_BOOK, TAS58XX_MIXER_GAIN_PAGE, TAS58XX_MIXER_GAIN_OFFSET[LEFT_2_LEFT_GAIN],
                                  reinterpret_cast<uint8_t *>(&mixer_coefficients), sizeof(mixer_coefficients))) {
    ESP_LOGE(TAG, "%s writing Input Mixer gains", ERROR);
    return false;
  }


  // if (!this->tas58xx_write_bytes_(TAS58XX_MIXER_GAIN_OFFSET[LEFT_2_LEFT_GAIN], reinterpret_cast<uint8_t *>(&mixer_coefficients), sizeof(mixer_coefficients))) {
  //   ESP_LOGE(TAG, "%s Mixer L-L Gain", ERROR);
  //   return false;
  // }

  // if (!this->tas58xx_write_bytes_(TAS58XX_MIXER_GAIN_OFFSET[RIGHT_2_LEFT_GAIN], reinterpret_cast<uint8_t *>(&mixer_r_to_l), 4)) {
  //   ESP_LOGE(TAG, "%s Mixer R-L Gain", ERROR);
  //   return false;
  // }

  // if (!this->tas58xx_write_bytes_(TAS58XX_MIXER_GAIN_OFFSET[LEFT_2_RIGHT_GAIN], reinterpret_cast<uint8_t *>(&mixer_l_to_r), 4)) {
  //   ESP_LOGE(TAG, "%s Mixer L-R Gain", ERROR);
  //   return false;
  // }

  // if (!this->tas58xx_write_bytes_(TAS58XX_MIXER_GAIN_OFFSET[RIGHT_2_RIGHT_GAIN], reinterpret_cast<uint8_t *>(&mixer_r_to_r), 4)) {
  //   ESP_LOGE(TAG, "%s Mixer R-R Gain", ERROR);
  //   return false;
  // }

  // if (!this->set_book_and_page_(TAS58XX_BOOK_ZERO, TAS58XX_PAGE_ZERO)) {
  //   ESP_LOGE(TAG, "%s end Set %s", ERROR, MIXER_MODE);
  //   return false;
  // }

  // 'tas58xx_state_' global already has mixer mode from YAML config
  // save anyway so 'set_mixer_mode' could be used more generally in future
  this->tas58xx_mixer_mode_ = mode;
  ESP_LOGD(TAG, "Set %s >> %s", MIXER_MODE, MIXER_MODE_TEXT[this->tas58xx_mixer_mode_]);
  return true;
}

bool Tas58xxComponent::get_state_(ControlState* state) {
  *state = this->tas58xx_control_state_;
  return true;
}

bool Tas58xxComponent::set_state_(ControlState state) {
  if (this->tas58xx_control_state_ == state) return true;
  if (!this->tas58xx_write_byte_(TAS58XX_DEVICE_CTRL_2, state)) return false;
  this->tas58xx_control_state_ = state;
  return true;
}

bool Tas58xxComponent::clear_fault_registers_() {
  if (!this->tas58xx_write_byte_(TAS58XX_FAULT_CLEAR, TAS58XX_ANALOG_FAULT_CLEAR)) return false;
  this->times_faults_cleared_++;
  ESP_LOGD(TAG, "Faults cleared");
  return true;
}

bool Tas58xxComponent::read_fault_registers_() {
  uint8_t current_faults[4];

  // read all faults registers
  if (!this->tas58xx_read_bytes_(TAS58XX_CHAN_FAULT, current_faults, 4)) return false;

  // note: new state is saved regardless as it is not worth conditionally saving state based on whether state has changed

  // check if any change CHAN_FAULT register as it contains 4 fault conditions(binary sensors)
  this->is_new_channel_fault_ = (current_faults[0] != this->tas58xx_faults_.channel_fault);
  this->tas58xx_faults_.channel_fault = current_faults[0];

  // separate clock fault from GLOBAL_FAULT1 register since clock faults can occur often
  // check if any change in GLOBAL_FAULT1 register as it contains 4 fault conditions(binary sensors) excluding clock fault
  uint8_t current_global_fault = current_faults[1] & REMOVE_CLOCK_FAULT;
  this->is_new_global_fault_ = (current_global_fault != this->tas58xx_faults_.global_fault);
  this->tas58xx_faults_.global_fault = current_global_fault;

  // over temperature fault is only fault condition in global_fault2 register
  this->is_new_over_temperature_issue_ = (current_faults[2] != this->tas58xx_faults_.temperature_fault);
  this->tas58xx_faults_.temperature_fault = current_faults[2];

  // over temperature warning is only fault condition in ot_warning register
  this->is_new_over_temperature_issue_ = (this->is_new_over_temperature_issue_ || (current_faults[3] != this->tas58xx_faults_.temperature_warning));
  this->tas58xx_faults_.temperature_warning = current_faults[3];

  bool new_fault_state; // reuse for temporary storage of new fault state

  // process clock_fault binary sensor
  new_fault_state = (current_faults[1] & (1 << 2));
  this->is_new_common_fault_ = (new_fault_state != this->tas58xx_faults_.clock_fault);
  this->tas58xx_faults_.clock_fault = new_fault_state;

  this->tas58xx_faults_.is_fault_except_clock_fault =
    ( this->tas58xx_faults_.channel_fault || this->tas58xx_faults_.global_fault ||
      this->tas58xx_faults_.temperature_fault || this->tas58xx_faults_.temperature_warning );

  #ifdef USE_TAS58XX_BINARY_SENSOR
  // process have_fault binary sensor
  new_fault_state = (this->tas58xx_faults_.is_fault_except_clock_fault || (this->tas58xx_faults_.clock_fault && (!this->exclude_clock_fault_from_have_faults_)));
  this->is_new_common_fault_ = this->is_new_common_fault_ || (new_fault_state != this->tas58xx_faults_.have_fault);
  this->tas58xx_faults_.have_fault = new_fault_state;
  #endif

  return true;
}


// low level functions

bool Tas58xxComponent::set_book_and_page_(uint8_t book, uint8_t page) {
  if (this->tas58xx_write_byte_(TAS58XX_PAGE_SET, TAS58XX_PAGE_ZERO)) {
    if (this->tas58xx_write_byte_(TAS58XX_BOOK_SET, book)) {
      if (this->tas58xx_write_byte_(TAS58XX_PAGE_SET, page)) return true;
    }
  }
  ESP_LOGE(TAG, "%s setting book and page", ERROR);
  return false;
}

bool Tas58xxComponent::book_and_page_write_(uint8_t book, uint8_t page, uint8_t sub_addr, uint8_t* data, uint8_t number_bytes) {
  // write up to 20 bytes (BIQUAD_SIZE) to a book and page starting at subaddress
  // limited to writing across one page boundary as is required for tas5805m while tas5825m has biquads aligned to page boundaries
  // shorter consecutive writes required by tas5805m and tas5825m do not extend over page boundaries

  if (number_bytes == 0 || number_bytes > BIQUAD_SIZE) {
    ESP_LOGE(TAG, "Incorrect length for book and page write");
    return false;
  }

  uint8_t bytes_in_block1{number_bytes};
  uint8_t bytes_in_block2{0};

  if ((sub_addr + number_bytes) > PAGE_SIZE) {
    bytes_in_block1 = PAGE_SIZE - sub_addr;
    bytes_in_block2 = number_bytes - bytes_in_block1;
  }

  if (!this->set_book_and_page_(book, page)) return false;

  ESP_LOGD(TAG, "Writing book:0x%02X page:0x%02X subaddress:0x%02X bytes:%d", book, page, sub_addr, bytes_in_block1);
  if (!this->tas58xx_write_bytes_(sub_addr, data, bytes_in_block1)) return false;

  if (bytes_in_block2 != 0) {
    uint8_t next_page = page + 1;

    // book already set so just change to next page
    if (!this->tas58xx_write_byte_(TAS58XX_PAGE_SET, next_page)) return false;

    ESP_LOGD(TAG, "Writing book:0x%02X page:0x%02X subaddress:0x%02X bytes:%d", book, next_page, MINIMUM_PAGE_SUBADDR, bytes_in_block2);
    if (!this->tas58xx_write_bytes_(MINIMUM_PAGE_SUBADDR, data + bytes_in_block1, bytes_in_block2)) return false;
  }

  return this->set_book_and_page_(TAS58XX_BOOK_ZERO, TAS58XX_PAGE_ZERO);
}


bool Tas58xxComponent::write_biquad_coefficients_(uint8_t page, uint8_t sub_addr, uint8_t* data) {
  // write a set of 20 biquad coefficents starting at page and subaddress
  // if (sizeof(*data) <> BIQUAD_SIZE) {
  //   ESP_LOGE(TAG, "Incorrect size of Biquad Coefficients");
  // }

  uint8_t bytes_in_block1{BIQUAD_SIZE};
  uint8_t bytes_in_block2{0};

  // write biquad page
  if (!this->set_book_and_page_(TAS58XX_EQ_BOOK, page)) {
    ESP_LOGE(TAG, "%s setting EQ Book @ Page:0x%02X", ERROR, page);
    return false;
  }
  ESP_LOGD(TAG, "Writing Biquad @ subaddress:0x%02X for %d bytes", sub_addr, bytes_in_block1);
  if ((sub_addr + BIQUAD_SIZE) > PAGE_SIZE) {
    bytes_in_block1 = PAGE_SIZE - sub_addr;
    bytes_in_block2 = BIQUAD_SIZE - bytes_in_block1;
  }

  if (!this->tas58xx_write_bytes_(sub_addr, data, bytes_in_block1)) {
    ESP_LOGE(TAG, "%s writing Biquad @ subaddress :0x%02X bytes: %d", ERROR, sub_addr, bytes_in_block1);
  }

  if (bytes_in_block2 != 0) {
    // if (!this->set_book_and_page_(TAS58XX_EQ_BOOK, page + 1)) {
    //   ESP_LOGE(TAG, "%s setting eq book @ page: 0x%02X", ERROR, page + 1);
    //   return false;
    // }
    // book already set so just change to next page
    if (!this->tas58xx_write_byte_(TAS58XX_PAGE_SET, page + 1)) {
      ESP_LOGE(TAG, "%s page 0x%02X", ERROR, page + 1);
      return false;
    }
    ESP_LOGD(TAG, "Writing Biquad @ subaddress: 0x%02X for %d bytes", ERROR, MINIMUM_PAGE_SUBADDR, bytes_in_block2);
    if (!this->tas58xx_write_bytes_(MINIMUM_PAGE_SUBADDR, data + bytes_in_block1, bytes_in_block2)) {
      ESP_LOGE(TAG, "%s writing Biquad @ subaddress: 0x%02X for %d bytes", ERROR, MINIMUM_PAGE_SUBADDR, bytes_in_block2);
      return false;
    }
  }

  return this->set_book_and_page_(TAS58XX_BOOK_ZERO, TAS58XX_PAGE_ZERO);
}

bool Tas58xxComponent::tas58xx_read_byte_(uint8_t a_register, uint8_t* data) {
  return this->tas58xx_read_bytes_(a_register, data, 1);
}

bool Tas58xxComponent::tas58xx_read_bytes_(uint8_t a_register, uint8_t* data, uint8_t number_bytes) {
  i2c::ErrorCode error_code;
  error_code = this->write(&a_register, 1);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "%s code: %d >> writing address: 0x%02X to start read", ERROR, error_code, a_register);
    this->i2c_error_ = (uint8_t)error_code;
    return false;
  }
  error_code = this->read_register(a_register, data, number_bytes);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "%s code: %d >> reading from address: 0x%02X for %d bytes", ERROR, error_code, a_register, number_bytes);
    this->i2c_error_ = (uint8_t)error_code;
    return false;
  }
  return true;
}

bool Tas58xxComponent::tas58xx_write_byte_(uint8_t a_register, uint8_t data) {
  return this->tas58xx_write_bytes_(a_register, &data, 1);
}

bool Tas58xxComponent::tas58xx_write_bytes_(uint8_t a_register, uint8_t* data, uint8_t number_bytes) {
  i2c::ErrorCode error_code = this->write_register(a_register, data, number_bytes);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "%s code: %d >> writing from address: 0x%02X for %d bytes", ERROR, error_code, a_register, number_bytes);
    this->i2c_error_ = (uint8_t)error_code;
    return false;
  }
  return true;
}

}  // namespace esphome::tas58xx
