#include "tas58xx.h"
#include "tas58xx_minimal.h"
#include "tas58xx_helpers.h"

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

static constexpr uint8_t DELAY_LOOPS = 40;  // 40 loop iterations ~ 300ms initial delay in 'loop' before writing eq settings

static constexpr uint16_t INITIAL_UPDATE_DELAY = 4000;  // initial ms delay before starting fault updates

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
  static constexpr uint8_t ESPHOME_MAXIMUM_DELAY = 5; // milliseconds

  uint16_t i = 0;
  uint16_t counter = 0;
  uint16_t number_configurations = sizeof(TAS58XX_CONFIG) / sizeof(TAS58XX_CONFIG[0]);

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

  // enable Tas58xx
  if (!this->set_deep_sleep_off_()) return false;

  if (!this->set_modulation_scheme_(this->tas58xx_modulation_scheme_)) return false;

  if (!this->set_dac_mode_(this->tas58xx_dac_mode_)) return false;

  if (!this->set_analog_gain_(this->tas58xx_analog_gain_)) return false;

  if (!this->set_state_(CTRL_PLAY)) return false;

  this->start_time_ = App.get_loop_component_start_time();
  return true;
}

void Tas58xxComponent::loop() {
  // 'play_file' is initiated by YAML on_boot with priority 220.0f
  // 'refresh_eq_settings' is triggered by Number 'left_eq_gain_16000hz' or 'right_eq_gain_16000hz' or Select 'eq_mode'
  // each with setup priority AFTER_CONNECTION = 100.0f
  // delay refreshing EQ settings until refresh is triggered so tas58xx has detected i2s clock through sound being played

  // loop_setup_stage_ is initially WAIT_FOR_TRIGGER

  switch (this->loop_setup_stage_) {
    case WAIT_FOR_TRIGGER:
      return;

    case RUN_DELAY_LOOP:
      if (this->loop_counter_ < DELAY_LOOPS) {    // loop_count was initialised to 0
        this->loop_counter_++;
        return;
      }
      this->loop_setup_stage_ = INPUT_MIXER_SETUP;
      return;

    case INPUT_MIXER_SETUP:
      // setup Eq Mode first
      if (!this->set_eq_mode_(this->tas58xx_eq_mode_)) {
         ESP_LOGW(TAG, "%s setting EQ Mode: %s", ERROR, EQ_MODE_TEXT[this->tas58xx_eq_mode_]);
      }

      if (!this->set_mixer_mode(this->tas58xx_mixer_mode_)) {
        ESP_LOGW(TAG, "%s setting %s:%s", ERROR, MIXER_MODE, MIXER_MODE_TEXT[this->tas58xx_mixer_mode_]);
      }
      this->loop_setup_stage_ = LR_VOLUME_SETUP;
      return;

    case LR_VOLUME_SETUP:
#ifdef USE_TAS58XX_CHANNEL_VOLUMES
      if (!this->set_channel_volume(LEFT_CHANNEL, this->tas58xx_channel_volume_[LEFT_CHANNEL])) {
        ESP_LOGW(TAG, "%s setting Left Channel Gain:%ddb", ERROR, this->tas58xx_channel_volume_[LEFT_CHANNEL]);
      }

      if (!this->set_channel_volume(RIGHT_CHANNEL, this->tas58xx_channel_volume_[RIGHT_CHANNEL])) {
        ESP_LOGW(TAG, "%ssetting Right Channel Gain:%ddb", ERROR, this->tas58xx_channel_volume_[RIGHT_CHANNEL]);
      }
#endif

#ifdef USE_TAS58XX_EQ_GAINS
      this->loop_setup_stage_ = EQ_BANDS_SETUP;
#endif

#ifdef USE_TAS58XX_EQ_PRESETS
      this->loop_setup_stage_ = EQ_PRESETS_SETUP;
#endif

      // if loop_setup_stage_ has not changed then no EQ configured
      if (this->loop_setup_stage_ == LR_VOLUME_SETUP) this->loop_setup_stage_ = SETUP_COMPLETE;
      return;

    case EQ_BANDS_SETUP:
#ifdef USE_TAS58XX_EQ_GAINS
      if (this->refresh_band_ == NUMBER_EQ_BANDS) {     // refresh_band_ was initialised to 0
        // finished writing all bands
        this->loop_setup_stage_ = SETUP_COMPLETE;
        this->refresh_band_ = 0;
        return;
      }

      if (!this->set_eq_gain(LEFT_CHANNEL, this->refresh_band_, this->tas58xx_eq_gain_[LEFT_CHANNEL][this->refresh_band_])) {
#ifdef USE_TAS58XX_EQ_BIAMP
        ESP_LOGW(TAG, "%s setting Gain Left %s:%d", ERROR, EQ_BAND, this->refresh_band_ + 1);
#else
        ESP_LOGW(TAG, "%s setting Gain %s:%d", ERROR, EQ_BAND, this->refresh_band_ + 1);
#endif
      }

#ifdef USE_TAS58XX_EQ_BIAMP
      if (!this->set_eq_gain(RIGHT_CHANNEL, this->refresh_band_, this->tas58xx_eq_gain_[RIGHT_CHANNEL][this->refresh_band_])) {
        ESP_LOGW(TAG, "%s setting Gain Right %s:%d", ERROR, EQ_BAND, this->refresh_band_ + 1);
      }
#endif

      this->refresh_band_++;
#endif // USE_TAS58XX_EQ_GAINS
      return;

    case EQ_PRESETS_SETUP:
#ifdef USE_TAS58XX_EQ_PRESETS
      if (!this->set_eq_preset(LEFT_CHANNEL, this->tas58xx_channel_preset_[LEFT_CHANNEL])) {
        ESP_LOGW(TAG, "%s setting Left Channel Preset index:%d", ERROR, this->tas58xx_channel_preset_[LEFT_CHANNEL]);
      }
      if (!this->set_eq_preset(RIGHT_CHANNEL, this->tas58xx_channel_preset_[RIGHT_CHANNEL])) {
        ESP_LOGW(TAG, "%s setting Right Channel Preset index:%d", ERROR, this->tas58xx_channel_preset_[RIGHT_CHANNEL]);
      }
      this->loop_setup_stage_ = SETUP_COMPLETE;
#endif
      return;

    case SETUP_COMPLETE:
      this->disable_loop(); // requires Esphome 2025.7.0 or greater
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

  // after delay updates starts here

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

void Tas58xxComponent::dump_config() {
#ifdef USE_TAS5805M_DAC
  ESP_LOGCONFIG(TAG, "Tas5805m Audio Dac:");
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
              "  Modulation: %s\n"
              "  DAC Mode: %s\n"
              "  Mixer Mode: %s\n"
              "  Volume Maximum: %idB\n"
              "  Volume Minimum: %idB\n"
              "  Ignore Fault: %s\n"
              "  Refresh EQ: %s\n",
              this->number_registers_configured_, this->tas58xx_analog_gain_,
              this->tas58xx_modulation_scheme_ ? "1SPW Mode" : "BD Mode",
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

// public //

// used by 'enable_dac_switch'
void Tas58xxComponent::enable_dac(bool enable) {
  enable ? this->set_deep_sleep_off_() : this->set_deep_sleep_on_();
}

// used by select eq mode
uint8_t Tas58xxComponent::get_configured_eq_mode() {
  return static_cast<uint8_t>(this->configured_eq_mode_);
}

uint8_t Tas58xxComponent::get_mixer_mode() {
  return static_cast<uint8_t>(this->tas58xx_mixer_mode_);
}

bool Tas58xxComponent::set_mixer_mode(MixerMode mode) {

  this->tas58xx_mixer_mode_ = mode;

  // only save until ready to setup in 'loop'
  if (this->loop_setup_stage_ < INPUT_MIXER_SETUP) {
     ESP_LOGV(TAG, "Save %s: %s", MIXER_MODE, MIXER_MODE_TEXT[mode]);
     return true;
  }

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
      mixer_coefficients.l_to_l =TAS58XX_MIXER_COEFF_0DB;
      mixer_coefficients.r_to_l =TAS58XX_MIXER_COEFF_MUTE;
      mixer_coefficients.l_to_r =TAS58XX_MIXER_COEFF_MUTE;
      mixer_coefficients.r_to_r =TAS58XX_MIXER_COEFF_0DB;
      break;

    case STEREO_INVERSE:
      mixer_coefficients.l_to_l =TAS58XX_MIXER_COEFF_MUTE;
      mixer_coefficients.r_to_l =TAS58XX_MIXER_COEFF_0DB;
      mixer_coefficients.l_to_r =TAS58XX_MIXER_COEFF_0DB;
      mixer_coefficients.r_to_r =TAS58XX_MIXER_COEFF_MUTE;
      break;

    case MONO:
      mixer_coefficients.l_to_l =TAS58XX_MIXER_COEFF_MINUS6DB;
      mixer_coefficients.r_to_l =TAS58XX_MIXER_COEFF_MINUS6DB;
      mixer_coefficients.l_to_r =TAS58XX_MIXER_COEFF_MINUS6DB;
      mixer_coefficients.r_to_r =TAS58XX_MIXER_COEFF_MINUS6DB;
      break;

    case LEFT:
      mixer_coefficients.l_to_l =TAS58XX_MIXER_COEFF_0DB;
      mixer_coefficients.r_to_l =TAS58XX_MIXER_COEFF_MUTE;
      mixer_coefficients.l_to_r =TAS58XX_MIXER_COEFF_0DB;
      mixer_coefficients.r_to_r =TAS58XX_MIXER_COEFF_MUTE;
      break;

    case RIGHT:
      mixer_coefficients.l_to_l =TAS58XX_MIXER_COEFF_MUTE;
      mixer_coefficients.r_to_l =TAS58XX_MIXER_COEFF_0DB;
      mixer_coefficients.l_to_r =TAS58XX_MIXER_COEFF_MUTE;
      mixer_coefficients.r_to_r =TAS58XX_MIXER_COEFF_0DB;
      break;

    default:
      ESP_LOGW(TAG, "Invalid %s", MIXER_MODE);
      return false;
  }

  if (!this->book_and_page_write_(TAS58XX_AUDIO_CTRL_BOOK, TAS58XX_MIXER_GAIN_PAGE, TAS58XX_MIXER_GAIN_SUBADDR,
                                  reinterpret_cast<uint8_t*>(&mixer_coefficients), sizeof(MixerCoefficients))) {
    ESP_LOGE(TAG, "%s writing %s: %s gains", MIXER_MODE, MIXER_MODE_TEXT[mode]);
    return false;
  }

  ESP_LOGD(TAG, "Set %s: %s", MIXER_MODE, MIXER_MODE_TEXT[mode]);
  return true;
}

// used by 'select eq mode' to determine initially selected EQ mode
bool Tas58xxComponent::is_eq_configured() {
  return this->eq_configured_;
}

// used by 'left_gain_band16000hz' or 'right_gain_band16000hz' or 'select eq_mode'
// to trigger loop setup
void Tas58xxComponent::refresh_eq_settings() {
  if (this->loop_setup_stage_ == WAIT_FOR_TRIGGER) {
    this->loop_setup_stage_ = RUN_DELAY_LOOP;
  }
  return;
}

bool Tas58xxComponent::set_channel_volume(Channels channel, int8_t volume_dB) {
#ifdef USE_TAS58XX_CHANNEL_VOLUMES
  if (volume_dB < TAS58XX_CHANNEL_VOLUME_MIN_DB || volume_dB > TAS58XX_CHANNEL_VOLUME_MAX_DB) {
    ESP_LOGE(TAG, "Invalid %s Channel Volume:%ddB", LR_CHANNEL_TEXT[channel], volume_dB);
    return false;
  }

  this->tas58xx_channel_volume_[channel] = volume_dB;

  // only save until ready to setup in 'loop'
  if (this->loop_setup_stage_ < LR_VOLUME_SETUP) {
    ESP_LOGV(TAG, "Save %s Channel Volume:%ddB", LR_CHANNEL_TEXT[channel], volume_dB);
    return true;
  }

  int32_t little_endian_9_23 = tas58xx_helpers::gain_to_f9_23_(volume_dB);

  if (!this->book_and_page_write_(TAS58XX_AUDIO_CTRL_BOOK, TAS58XX_CHANNEL_VOLUME_PAGE, TAS58XX_CHANNEL_VOLUME_SUBADDR[channel],
                                  reinterpret_cast<uint8_t*>(&little_endian_9_23), sizeof(little_endian_9_23))) {
    ESP_LOGE(TAG, "%s writing %s Channel Volume:%ddb", ERROR, LR_CHANNEL_TEXT[channel], volume_dB);
    return false;
  }

  ESP_LOGD(TAG, "Set %s Channel Volume:%ddB", LR_CHANNEL_TEXT[channel], volume_dB);
#endif
  return true;
}

// used by select eq mode
void Tas58xxComponent::select_eq_mode(uint8_t select_index) {
  if ( select_index == static_cast<uint8_t>(EqMode::EQ_OFF) ) {
    this->set_eq_mode_(EqMode::EQ_OFF);
  } else {
    this->set_eq_mode_(this->configured_eq_mode_);
  }
}

// used by eq gain numbers
bool Tas58xxComponent::set_eq_gain(Channels channel, uint8_t band_index, int8_t gain) {
#ifdef USE_TAS58XX_EQ_GAINS

  if (band_index >= NUMBER_EQ_BANDS) {
    ESP_LOGE(TAG, "Invalid Band index:%d", band_index);
    return false;
  }

  const uint8_t band = band_index + 1;

  if (gain < TAS58XX_EQ_MIN_DB || gain > TAS58XX_EQ_MAX_DB) {
    ESP_LOGE(TAG, "Invalid %s Channel %s:%d Gain:%ddB", LR_CHANNEL_TEXT[channel], EQ_BAND, band, gain);
    return false;
  }

  this->tas58xx_eq_gain_[channel][band_index] = gain;

  // only save until ready to setup in 'loop'
  if (this->loop_setup_stage_ < EQ_BANDS_SETUP) {
    ESP_LOGV(TAG, "Save %s Channel %s:%d Gain:%ddB", LR_CHANNEL_TEXT[channel], EQ_BAND, band, gain);
    return true;
  }

  uint8_t x = (gain + TAS58XX_EQ_MAX_DB);

#ifdef USE_TAS5805M_DAC
  #ifdef USE_TAS58XX_EQ_BIAMP
  const AddressSequence* eq_address = (channel == LEFT_CHANNEL) ? &TAS5805M_LEFT_EQ_ADDRESS[band_index] : &TAS5805M_RIGHT_EQ_ADDRESS[band_index];
  #else
  const AddressSequence* eq_address = &TAS5805M_LEFT_EQ_ADDRESS[band_index];
  #endif
#else
  #ifdef USE_TAS58XX_EQ_BIAMP
  const AddressSequence* eq_address = (channel == LEFT_CHANNEL) ? &TAS5825M_LEFT_EQ_ADDRESS[band_index] : &TAS5825M_RIGHT_EQ_ADDRESS[band_index];
  #else
  const AddressSequence* eq_address = &TAS5825M_LEFT_EQ_ADDRESS[band_index];
  #endif
#endif

  const BiquadSequence* biquad = &EQ_BAND_COEFFICIENTS[x][band_index];

  if ((eq_address == NULL) || (biquad == NULL)) {
    ESP_LOGE(TAG, "%s NULL discovered %s Channel %s:%d Gain:%ddB", LR_CHANNEL_TEXT[channel], EQ_BAND, band, gain);
    return false;
  }

  if (!this->book_and_page_write_(TAS58XX_EQ_CTRL_BOOK, eq_address->page, eq_address->sub_addr,
                                  reinterpret_cast<uint8_t*>(const_cast<uint8_t*>(biquad->coefficients)), BIQUAD_SIZE)) {
    ESP_LOGE(TAG, "%s writing Biquad %s Channel %s:%d Gain:%ddB", ERROR, LR_CHANNEL_TEXT[channel], EQ_BAND, band, gain);
    return false;
  }

  ESP_LOGD(TAG, "Set %s Channel %s:%d Gain:%ddB", LR_CHANNEL_TEXT[channel], EQ_BAND, band, gain);
#endif
  return true;
}

bool Tas58xxComponent::set_eq_preset(Channels channel, uint8_t select_preset) {
#ifdef USE_TAS58XX_EQ_PRESETS
  if (select_preset > EQ_PROFILE_MAXIMUM_INDEX) {
    ESP_LOGE(TAG, "Invalid %s Channel Preset index:%d", LR_CHANNEL_TEXT[channel], select_preset);
    return false;
  }

  this->tas58xx_channel_preset_[channel] = select_preset;

  // only save until ready to setup in 'loop'
  if (this->loop_setup_stage_ < EQ_PRESETS_SETUP) {
    ESP_LOGV(TAG, "Save %s Channel EQ Preset index:%d", LR_CHANNEL_TEXT[channel], select_preset);
    return true;
  }

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

  if (!this->book_and_page_write_(TAS58XX_EQ_CTRL_BOOK, biquad1_address->page, biquad1_address->sub_addr,
                                  reinterpret_cast<uint8_t*>(const_cast<uint8_t*>(biquad1->coefficients)), BIQUAD_SIZE)) {
    ESP_LOGE(TAG, "%s writing Biquad 1 for %s Channel EQ Preset index:%d", ERROR, LR_CHANNEL_TEXT[channel], select_preset);
    return false;
  }
  if (!this->book_and_page_write_(TAS58XX_EQ_CTRL_BOOK, biquad2_address->page, biquad2_address->sub_addr,
                                  reinterpret_cast<uint8_t*>(const_cast<uint8_t*>(biquad2->coefficients)), BIQUAD_SIZE)) {
    ESP_LOGE(TAG, "%s writing Biquad 2 for %s Channel EQ Preset index:%d", ERROR, LR_CHANNEL_TEXT[channel], select_preset);
    return false;
  }
  if (!this->book_and_page_write_(TAS58XX_EQ_CTRL_BOOK, biquad3_address->page, biquad3_address->sub_addr,
                                  reinterpret_cast<uint8_t*>(const_cast<uint8_t*>(biquad3->coefficients)), BIQUAD_SIZE)) {
    ESP_LOGE(TAG, "%s writing Biquad 3 for %s Channel EQ Preset index:%d", ERROR, LR_CHANNEL_TEXT[channel], select_preset);
    return false;
  }

  ESP_LOGD(TAG, "Set %s Channel EQ Preset index >> %d", LR_CHANNEL_TEXT[channel], select_preset);
#endif
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

// used by fault sensor
uint32_t Tas58xxComponent::times_faults_cleared() {
  return this->times_faults_cleared_;
}

// used by 'left_gain_band16000hz' or 'right_gain_band16000hz' or 'select eq_mode'
bool Tas58xxComponent::using_auto_eq_refresh() {
  return (this->eq_refresh_ == EqRefreshMode::AUTO);
}

// used by 'select eq_mode'
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
    ESP_LOGV(TAG, "Volume >> %ddB", dB);
  #endif
  return true;
}

// protected //

bool Tas58xxComponent::get_analog_gain_(uint8_t* raw_gain) {
  uint8_t current;
  if (!this->tas58xx_read_bytes_(TAS58XX_AGAIN, &current, 1)) return false;
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
  static constexpr uint8_t TOP_3BITS_MASK = 0xE0;

  if ((gain_db < TAS58XX_MIN_ANALOG_GAIN) || (gain_db > TAS58XX_MAX_ANALOG_GAIN)) return false;

  uint8_t new_again = static_cast<uint8_t>(-gain_db * 2.0);

  uint8_t current_again;
  if (!this->tas58xx_read_bytes_(TAS58XX_AGAIN, &current_again, 1)) return false;

  // keep top 3 reserved bits combine with bottom 5 analog gain bits
  new_again = (current_again & TOP_3BITS_MASK) | new_again;
  if (!this->tas58xx_write_byte_(TAS58XX_AGAIN, new_again)) return false;

  ESP_LOGV(TAG, "Analog Gain >> %fdB", gain_db);
  return true;
}

bool Tas58xxComponent::get_dac_mode_(DacMode* mode) {
    uint8_t current_value;
    if (!this->tas58xx_read_bytes_(TAS58XX_DEVICE_CTRL_1, &current_value, 1)) return false;
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
  ESP_LOGV(TAG, "DAC mode >> %s", this->tas58xx_dac_mode_ ? "PBTL" : "BTL");
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
  if (!this->tas58xx_read_bytes_(TAS58XX_DIG_VOL_CTRL, &current, 1)) return false;
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
  this->tas58xx_eq_mode_ = new_mode;

  // only save until ready to setup in 'loop'
  if (this->loop_setup_stage_ < INPUT_MIXER_SETUP) {
    ESP_LOGV(TAG, "Save EQ Mode: %s", EQ_MODE_TEXT[new_mode]);
    return true;
  }

#ifdef USE_TAS5805M_DAC
  if (!this->tas58xx_write_byte_(TAS5805M_DSP_MISC, TAS5805M_CTRL_EQ[new_mode])) {
    ESP_LOGE(TAG, "%s writing Eq Mode: %s", ERROR, EQ_MODE_TEXT[new_mode]);
    return false;
  }
#else
  const EqModeCoefficients* eq_mode_coefficients = &TAS5825M_CTRL_EQ[new_mode];
  if (!this->book_and_page_write_(TAS58XX_AUDIO_CTRL_BOOK, TAS5825M_EQ_MODE_CTRL_PAGE, TAS5825M_GANG_EQ,
                                  reinterpret_cast<uint8_t*>(const_cast<EqModeCoefficients*>(eq_mode_coefficients)), sizeof(EqModeCoefficients))) {
    ESP_LOGE(TAG, "%s writing Eq Mode: %s", ERROR, EQ_MODE_TEXT[new_mode]);
    return false;
  }
#endif

  ESP_LOGD(TAG, "Set EQ Mode: %s", EQ_MODE_TEXT[new_mode]);
#endif
  return true;
}

// only runs once from 'setup'
bool Tas58xxComponent::set_modulation_scheme_(ModulationScheme modulation) {
  static constexpr uint8_t MODULATION_MASK = 0b11111100; // bits 0 and 1 are modulation

  uint8_t value;
  if (!this->tas58xx_read_bytes_(TAS58XX_DEVICE_CTRL_1, &value, 1)) return false;

  value = value & (MODULATION_MASK + static_cast<uint8_t>(modulation));

  if (!this->tas58xx_write_byte_(TAS58XX_DEVICE_CTRL_1, value)) return false;

  // save so 'set_modulation_scheme_' could be used more generally
  this->tas58xx_modulation_scheme_ = modulation;
  ESP_LOGV(TAG, "Modulation >> %s", this->tas58xx_modulation_scheme_ ? "1SPW Mode" : "BD Mode");
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

//// fault processing functions

bool Tas58xxComponent::clear_fault_registers_() {
  if (!this->tas58xx_write_byte_(TAS58XX_FAULT_CLEAR, TAS58XX_ANALOG_FAULT_CLEAR)) return false;
  this->times_faults_cleared_++;
  ESP_LOGD(TAG, "Faults cleared");
  return true;
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

bool Tas58xxComponent::read_fault_registers_() {
  static constexpr uint8_t REMOVE_CLOCK_FAULT = 0xFB;  // clock fault bit of global_fault1 register

  uint8_t current_faults[4];

  // read all faults registers
  if (!this->tas58xx_read_bytes_(TAS58XX_CHAN_FAULT, current_faults, sizeof(current_faults))) return false;

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

//// low level functions

bool Tas58xxComponent::book_and_page_write_(uint8_t book, uint8_t page, uint8_t sub_addr, uint8_t* data, uint8_t number_bytes) {
  // write up to 20 bytes (BIQUAD_SIZE) to a book and page starting at subaddress
  // limited to writing across one page boundary as is required for tas5805m while tas5825m has biquads aligned to page boundaries
  // shorter consecutive writes required by tas5805m and tas5825m do not extend over page boundaries

  // Biquad addressing constants
  static constexpr uint8_t PAGE_SIZE = 0x80;           		// 0x7F + 1 = 0x80
  static constexpr uint8_t MINIMUM_PAGE_SUBADDR = 0x08;   // start subaddr for pages = 0x08

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

  // set book and page
  bool ok = this->tas58xx_write_byte_(TAS58XX_PAGE_SET, TAS58XX_PAGE_ZERO);
  if (ok) ok = this->tas58xx_write_byte_(TAS58XX_BOOK_SET, book);
  if (ok) ok = this->tas58xx_write_byte_(TAS58XX_PAGE_SET, page);
  if (!ok) {
    ESP_LOGE(TAG, "%s setting book:0x%02X page:0x%02X", ERROR, book, page);
    return false;
  }

  // do block write to book and page sub-address
  ESP_LOGV(TAG, "Writing book:0x%02X page:0x%02X subaddress:0x%02X bytes:%d", book, page, sub_addr, bytes_in_block1);
  if (!this->tas58xx_write_bytes_(sub_addr, data, bytes_in_block1)) return false;

  if (bytes_in_block2 != 0) {
    uint8_t next_page = page + 1;

    // book already set so just change to next page
    if (!this->tas58xx_write_byte_(TAS58XX_PAGE_SET, next_page)) {
      ESP_LOGE(TAG, "%s setting next page", ERROR);
      return false;
    }
    ESP_LOGV(TAG, "Writing book:0x%02X page:0x%02X subaddress:0x%02X bytes:%d", book, next_page, MINIMUM_PAGE_SUBADDR, bytes_in_block2);
    if (!this->tas58xx_write_bytes_(MINIMUM_PAGE_SUBADDR, data + bytes_in_block1, bytes_in_block2)) return false;
  }

  // reset book and page to zero
  ok = this->tas58xx_write_byte_(TAS58XX_PAGE_SET, TAS58XX_PAGE_ZERO);
  if (ok) ok = this->tas58xx_write_byte_(TAS58XX_BOOK_SET, TAS58XX_BOOK_ZERO);
  if (ok) ok = this->tas58xx_write_byte_(TAS58XX_PAGE_SET, TAS58XX_PAGE_ZERO);
  if (!ok) {
    ESP_LOGE(TAG, "%s re-setting book and page", ERROR);
    return false;
  }
  return true;
}

bool Tas58xxComponent::tas58xx_read_bytes_(uint8_t a_register, uint8_t* data, uint8_t number_bytes) {
  i2c::ErrorCode error_code;
  error_code = this->write(&a_register, 1);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "%s code:%d writing address:0x%02X to start read", ERROR, error_code, a_register);
    this->i2c_error_ = (uint8_t)error_code;
    return false;
  }
  error_code = this->read_register(a_register, data, number_bytes);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "%s code:%d reading %d bytes from address:0x%02X", ERROR, error_code, number_bytes, a_register);
    this->i2c_error_ = (uint8_t)error_code;
    return false;
  }
  return true;
}

bool Tas58xxComponent::tas58xx_write_byte_(uint8_t a_register, uint8_t data) {
   i2c::ErrorCode error_code = this->write_register(a_register, &data, 1);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "%s code:%d writing to address:0x%02X", ERROR, error_code, a_register);
    this->i2c_error_ = (uint8_t)error_code;
    return false;
  }
  return true;
}

bool Tas58xxComponent::tas58xx_write_bytes_(uint8_t a_register, uint8_t* data, uint8_t number_bytes) {
  i2c::ErrorCode error_code = this->write_register(a_register, data, number_bytes);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "%s code:%d writing %d bytes to address:0x%02X", ERROR, error_code, number_bytes, a_register);
    this->i2c_error_ = (uint8_t)error_code;
    return false;
  }
  return true;
}

}  // namespace esphome::tas58xx
