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

  if (!this->i2s_prime_open_channel_()) {
    ESP_LOGW(TAG, "I2S priming unavailable, DAC will fault on Play transition");
  } else {
    size_t bytes_written = 0;
    this->i2s_prime_write_(PRIME_BUFFER, sizeof(PRIME_BUFFER), &bytes_written);
    this->i2s_prime_close_channel_();
  }
  // enable Tas58xx
  if (!this->set_deep_sleep_off_()) return false;

  if (!this->set_modulation_scheme_(this->tas58xx_modulation_scheme_)) return false;

  if (!this->set_dac_mode_(this->tas58xx_dac_mode_)) return false;

  if (!this->set_analog_gain_(this->tas58xx_analog_gain_)) return false;

  if (!this->set_state_(CTRL_PLAY)) return false;
  if (!this->tas58xx_write_byte_(TAS58XX_FAULT_CLEAR, TAS58XX_ANALOG_FAULT_CLEAR)) return false;

  return true;
}

void Tas58xxComponent::update() {
  ESP_LOGD(TAG, "reading faults");
  if (!this->read_fault_registers_()) {
    ESP_LOGW(TAG, "%s reading faults", ERROR);
    return;
  }

  if (this->tas58xx_faults_.have_fault) {
    ESP_LOGD(TAG, "clearing faults");
    if (!this->clear_fault_registers_()) {
      ESP_LOGW(TAG, "%s clearing faults", ERROR);
    }
  }

  if ( !(this->is_new_common_fault_ || this->is_new_channel_fault_ || this->is_new_global_fault_) ) return;

#ifdef USE_TAS58XX_BINARY_SENSOR
  ESP_LOGD(TAG, "publishing faults");
  this->publish_faults_();
#endif
}

void Tas58xxComponent::dump_config() {
#ifdef USE_TAS5805M_DAC
  ESP_LOGCONFIG(TAG, "Tas5805m Audio Dac:");
#else
  ESP_LOGCONFIG(TAG, "Tas5825m Audio Dac:");
#endif

  LOG_I2C_DEVICE(this);
  LOG_PIN("  Enable Pin: ", this->enable_pin_);

  switch (this->error_code_) {
    case CONFIGURATION_FAILED:
      ESP_LOGE(TAG, "  %s setup failed: %i", ERROR, this->i2c_error_);
      break;
    case NONE:
      ESP_LOGCONFIG(TAG,
              "  Registers Configured: %i\n"
              "  Analog Gain: %3.1fdB\n"
              "  Modulation: %s\n"
              "  DAC Mode: %s\n"
              "  Mixer Mode: %s\n"
              "  Volume Maximum: %idB\n"
              "  Volume Minimum: %idB\n",
              this->number_registers_configured_, this->tas58xx_analog_gain_,
              this->tas58xx_modulation_scheme_ ? "1SPW Mode" : "BD Mode",
              this->tas58xx_dac_mode_ ? "PBTL" : "BTL",
              INPUT_MIXER_MODE_TEXT[this->tas58xx_input_mixer_mode_],
              this->tas58xx_volume_max_, this->tas58xx_volume_min_
              );
      LOG_UPDATE_INTERVAL(this);
      break;
  }

#ifdef USE_TAS58XX_BINARY_SENSOR
  ESP_LOGCONFIG(TAG, "Tas58xx Binary Sensors:");
  LOG_BINARY_SENSOR("  ", "Any Faults", this->have_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Right Channel Over Current", this->right_channel_over_current_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Left Channel Over Current", this->left_channel_over_current_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Right Channel DC Fault", this->right_channel_dc_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Left Channel DC Fault", this->left_channel_dc_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "PVDD Under Voltage", this->pvdd_under_voltage_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "PVDD Over Voltage", this->pvdd_over_voltage_fault_binary_sensor_);
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

// used by select mixer mode
uint8_t Tas58xxComponent::get_configured_dac_mode() {
   return static_cast<uint8_t>(this->tas58xx_dac_mode_); // BTL = 0 , PBTL = 1
}

// used by select eq mode
uint8_t Tas58xxComponent::get_configured_eq_mode() {
  return static_cast<uint8_t>(this->configured_eq_mode_);
}

uint8_t Tas58xxComponent::get_mixer_mode() {
  return static_cast<uint8_t>(this->tas58xx_input_mixer_mode_);
}

bool Tas58xxComponent::set_input_mixer_mode(InputMixerMode mode) {

  this->tas58xx_input_mixer_mode_ = mode;

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
  return true;
}

// used by 'select eq mode' to determine initially selected EQ mode
bool Tas58xxComponent::is_eq_configured() {
  return this->eq_configured_;
}

bool Tas58xxComponent::set_channel_volume(Channels channel, int8_t volume_dB) {
#ifdef USE_TAS58XX_CHANNEL_VOLUMES
  if (volume_dB < TAS58XX_CHANNEL_VOLUME_MIN_DB || volume_dB > TAS58XX_CHANNEL_VOLUME_MAX_DB) {
    ESP_LOGE(TAG, "Invalid %s Channel Volume: %ddB", LR_CHANNEL_TEXT[channel], volume_dB);
    return false;
  }

  this->tas58xx_channel_volume_[channel] = volume_dB;

  int32_t little_endian_9_23 = tas58xx_helpers::gain_to_f9_23_(volume_dB);

  if (!this-> book_page_write_bytes_(TAS58XX_AUDIO_CTRL_BOOK, TAS58XX_CHANNEL_VOLUME_PAGE, TAS58XX_CHANNEL_VOLUME_SUBADDR[channel],
                                      reinterpret_cast<uint8_t*>(&little_endian_9_23), sizeof(little_endian_9_23))) {
    ESP_LOGW(TAG, "%s writing %s Channel Volume: %ddb", ERROR, LR_CHANNEL_TEXT[channel], volume_dB);
    return false;
  }

  ESP_LOGD(TAG, "%s Channel Volume >> %ddB", LR_CHANNEL_TEXT[channel], volume_dB);
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
    ESP_LOGE(TAG, "Invalid Band index: %d", band_index);
    return false;
  }

  const uint8_t band = band_index + 1;

  if (gain < TAS58XX_EQ_MIN_DB || gain > TAS58XX_EQ_MAX_DB) {
    ESP_LOGE(TAG, "Invalid %s Channel %s:%d Gain: %ddB", LR_CHANNEL_TEXT[channel], EQ_BAND, band, gain);
    return false;
  }

  this->tas58xx_eq_gain_[channel][band_index] = gain;

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

  if (eq_address == NULL) {
    ESP_LOGE(TAG, "NULL discovered %s Channel %s:%d Gain: %ddB", LR_CHANNEL_TEXT[channel], EQ_BAND, band, gain);
    return false;
  }

  static constexpr uint32_t EQ_SAMPLE_RATE = 96000;
  ESP_LOGD(TAG, "%s Channel %s:%dHz Gain >> %ddB", LR_CHANNEL_TEXT[channel], EQ_BAND, EQ_BAND_FREQUENCY[band_index], gain);

  tas58xx_helpers::BiquadCoefficients biquad =
      tas58xx_helpers::equalizer_qfactor_(EQ_SAMPLE_RATE, EQ_BAND_FREQUENCY[band_index], gain, EQ_BAND_QFACTOR[band_index]);

  if (!this->biquad_write_bytes_(TAS58XX_EQ_CTRL_BOOK, eq_address->page, eq_address->sub_addr,
                                  reinterpret_cast<uint8_t*>(&biquad), sizeof(biquad))) {
    ESP_LOGW(TAG, "%s writing Biquad %s Channel %s:%d Gain: %ddB", ERROR, LR_CHANNEL_TEXT[channel], EQ_BAND, band, gain);
    return false;
  }

#endif
  return true;
}

bool Tas58xxComponent::set_eq_preset(Channels channel, uint8_t select_preset) {
#ifdef USE_TAS58XX_EQ_PRESETS
  if (select_preset > EQ_PROFILE_MAXIMUM_INDEX) {
    ESP_LOGE(TAG, "Invalid %s Channel Preset index: %d", LR_CHANNEL_TEXT[channel], select_preset);
    return false;
  }

  this->tas58xx_channel_preset_[channel] = select_preset;

  // only save until ready to setup in 'loop'
  if (this->loop_setup_stage_ < EQ_PRESETS_SETUP) {
    ESP_LOGD(TAG, "Save %s Channel EQ Preset index: %d", LR_CHANNEL_TEXT[channel], select_preset);
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

  if ((biquad1_address == NULL) || (biquad2_address == NULL) || (biquad3_address == NULL)) {
    ESP_LOGE(TAG, "NULL EQ Preset Address pointer");
    return false;
  }

  static constexpr uint32_t EQ_SAMPLE_RATE = 96000;

  // calculate biquads
  if (select_preset == 0) {
    tas58xx_helpers::BiquadCoefficients biquad1 = tas58xx_helpers::all_pass_();
    tas58xx_helpers::BiquadCoefficients biquad3 = biquad1;
  } else if (select_preset <= LF_PROFILE_MAXIMUM_INDEX) {
    uint8_t profile_index = select_preset - 1;
    tas58xx_helpers::BiquadCoefficients biquad1 =
       tas58xx_helpers::low_pass_filter_(EQ_SAMPLE_RATE, PROFILES[profile_index].biquad_1_and_2_frequency, 0);
    tas58xx_helpers::BiquadCoefficients biquad3 = tas58xx_helpers::all_pass_();
  } else {
    uint8_t profile_index = select_preset - LF_PROFILE_MAXIMUM_INDEX - 1;
    tas58xx_helpers::BiquadCoefficients biquad1 =
       tas58xx_helpers::low_pass_filter_(EQ_SAMPLE_RATE, PROFILES[profile_index].biquad_1_and_2_frequency, 0);
    tas58xx_helpers::BiquadCoefficients biquad3 =
       tas58xx_helpers::high_shelf_filter_(EQ_SAMPLE_RATE, PROFILES[profile_index].biquad_3_frequency,
                                            PROFILES[profile_index].biquid_3_gain, PROFILES[profile_index].biquid_3_qfactor);
  }

  if (!this->biquad_write_bytes_(TAS58XX_EQ_CTRL_BOOK, biquad1_address->page, biquad1_address->sub_addr,
                                  reinterpret_cast<uint8_t*>(&biquad1), sizeof(biquad))) {
    ESP_LOGW(TAG, "%s writing Biquad 1 for %s Channel EQ Preset index: %d", ERROR, LR_CHANNEL_TEXT[channel], select_preset);
    return false;
  }

  if (!this->biquad_write_bytes_(TAS58XX_EQ_CTRL_BOOK, biquad2_address->page, biquad2_address->sub_addr,
                                  reinterpret_cast<uint8_t*>(&biquad1), sizeof(biquad))) {
    ESP_LOGW(TAG, "%s writing Biquad 2 for %s Channel EQ Preset index: %d", ERROR, LR_CHANNEL_TEXT[channel], select_preset);
    return false;
  }

  if (!this->biquad_write_bytes_(TAS58XX_EQ_CTRL_BOOK, biquad3_address->page, biquad3_address->sub_addr,
                                  reinterpret_cast<uint8_t*>(&biquad3), sizeof(biquad))) {
    ESP_LOGW(TAG, "%s writing Biquad 3 for %s Channel EQ Preset index: %d", ERROR, LR_CHANNEL_TEXT[channel], select_preset);
    return false;
  }

  ESP_LOGD(TAG, "%s Channel EQ Preset index >> %d", LR_CHANNEL_TEXT[channel], select_preset);
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

// override for audio_dac component volume, so mediaplayer can determine current volume of tas58xx dac
float Tas58xxComponent::volume() {
  uint8_t raw_volume;
  this->get_digital_volume_(&raw_volume);
  return remap<float, uint8_t>(raw_volume, this->tas58xx_raw_volume_min_, this->tas58xx_raw_volume_max_, 0.0f, 1.0f);
}

// override for audio_dac component set_volume, so mediaplayer can adjust volume of tas58xx dac
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

  ESP_LOGD(TAG, "Analog Gain >> %fdB", gain_db);
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

#ifdef USE_TAS5805M_DAC
  if (!this->tas58xx_write_byte_(TAS5805M_DSP_MISC, TAS5805M_CTRL_EQ[new_mode])) {
    ESP_LOGW(TAG, "%s writing Eq Mode: %s", ERROR, EQ_MODE_TEXT[new_mode]);
    return false;
  }
#else
  const EqModeCoefficients* eq_mode_coefficients = &TAS5825M_CTRL_EQ[new_mode];
  if (!this->book_page_write_bytes_(TAS58XX_AUDIO_CTRL_BOOK, TAS5825M_EQ_MODE_CTRL_PAGE, TAS5825M_GANG_EQ,
                                  reinterpret_cast<uint8_t*>(const_cast<EqModeCoefficients*>(eq_mode_coefficients)), sizeof(EqModeCoefficients))) {
    ESP_LOGW(TAG, "%s writing Eq Mode: %s", ERROR, EQ_MODE_TEXT[new_mode]);
    return false;
  }
#endif

  ESP_LOGD(TAG, "EQ Mode >> %s", EQ_MODE_TEXT[new_mode]);
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
  ESP_LOGD(TAG, "Modulation >> %s", this->tas58xx_modulation_scheme_ ? "1SPW Mode" : "BD Mode");
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
    ESP_LOGD(TAG, "publish common faults");
    if (this->have_fault_binary_sensor_ != nullptr) {
      this->have_fault_binary_sensor_->publish_state(this->tas58xx_faults_.have_fault);
    }
  }

  if (this->over_temperature_shutdown_fault_binary_sensor_ != nullptr) {
    this->over_temperature_shutdown_fault_binary_sensor_->publish_state(this->tas58xx_faults_.temperature_fault);
  }

  if (this->over_temperature_warning_binary_sensor_ != nullptr) {
    this->over_temperature_warning_binary_sensor_->publish_state(this->tas58xx_faults_.temperature_warning);
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
  ESP_LOGD(TAG, "publish channel faults");
  if (this->right_channel_over_current_fault_binary_sensor_ != nullptr) {
    ESP_LOGD(TAG, "publish channel fault0");
    this->right_channel_over_current_fault_binary_sensor_->publish_state(this->tas58xx_faults_.channel_fault & (1 << 0));
  }

  if (this->left_channel_over_current_fault_binary_sensor_ != nullptr) {
    ESP_LOGD(TAG, "publish channel fault1");
    this->left_channel_over_current_fault_binary_sensor_->publish_state(this->tas58xx_faults_.channel_fault & (1 << 1));
  }

  if (this->right_channel_dc_fault_binary_sensor_ != nullptr) {
    ESP_LOGD(TAG, "publish channel fault2");
    this->right_channel_dc_fault_binary_sensor_->publish_state(this->tas58xx_faults_.channel_fault & (1 << 2));
  }

  if (this->left_channel_dc_fault_binary_sensor_ != nullptr) {
    ESP_LOGD(TAG, "publish channel fault3");
    this->left_channel_dc_fault_binary_sensor_->publish_state(this->tas58xx_faults_.channel_fault & (1 << 3));
  }

  if (this->is_new_global_fault_) {
      this->set_timeout("", 15, [this]() { this->publish_global_faults_(); });
  }
}


void Tas58xxComponent::publish_global_faults_() {
  ESP_LOGD(TAG, "publish global faults");
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

  // separate GLOBAL_FAULT1 from clock faults since clock faults can occur often
  // check if any change in GLOBAL_FAULT1 register as it contains 4 fault conditions(binary sensors) excluding clock fault
  uint8_t current_global_fault = current_faults[1] & REMOVE_CLOCK_FAULT;
  this->is_new_global_fault_ = (current_global_fault != this->tas58xx_faults_.global_fault);
  this->tas58xx_faults_.global_fault = current_global_fault;

  // over temperature fault is only fault condition in global_fault2 register
  this->tas58xx_faults_.temperature_fault = current_faults[2];

  // over temperature warning is only fault condition in ot_warning register
  this->tas58xx_faults_.temperature_warning = current_faults[3];

#ifdef USE_TAS58XX_BINARY_SENSOR
  bool new_have_fault_state;
  new_have_fault_state =  ( this->tas58xx_faults_.channel_fault || this->tas58xx_faults_.global_fault ||
                            this->tas58xx_faults_.temperature_fault || this->tas58xx_faults_.temperature_warning );
  this->is_new_common_fault_ = (new_have_fault_state != this->tas58xx_faults_.have_fault);
  this->tas58xx_faults_.have_fault = new_have_fault_state;
#endif

  return true;
}

//// low level functions

// i2s priming functions run in setup() at HARDWARE priority
// should be before any other component's loop()/background task exists

bool Tas58xxComponent::i2s_prime_open_channel_() {
  // defensive check — not expected to actually fail
  if (!this->parent_->try_lock()) {
    return false;
  }

  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(
      static_cast<i2s_port_t>(this->parent_->get_port()), I2S_ROLE_MASTER);
  esp_err_t err = i2s_new_channel(&chan_cfg, &this->prime_tx_handle_, nullptr);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "I2S New Channel failed: %s", esp_err_to_name(err));
    this->prime_tx_handle_ = nullptr;
    this->parent_->unlock();
    return false;
  }

  i2s_std_gpio_config_t pin_cfg = this->parent_->get_pin_config();
  pin_cfg.dout = this->dout_pin_;  // use YAML configured dout

  i2s_std_clk_config_t clk_cfg = {
      .sample_rate_hz = 48000,
      .clk_src = I2S_CLK_SRC_DEFAULT,
      .mclk_multiple = I2S_MCLK_MULTIPLE_256,
  };
  i2s_std_slot_config_t slot_cfg =
      I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  i2s_std_config_t std_cfg = {.clk_cfg = clk_cfg, .slot_cfg = slot_cfg, .gpio_cfg = pin_cfg};

  err = i2s_channel_init_std_mode(this->prime_tx_handle_, &std_cfg);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "I2S Channel Init Std Mode failed: %s", esp_err_to_name(err));
    i2s_del_channel(this->prime_tx_handle_);
    this->prime_tx_handle_ = nullptr;
    this->parent_->unlock();
    return false;
  }

  err = i2s_channel_enable(this->prime_tx_handle_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "I2S Channel Enable failed: %s", esp_err_to_name(err));
    i2s_del_channel(this->prime_tx_handle_);
    this->prime_tx_handle_ = nullptr;
    this->parent_->unlock();
    return false;
  }

  return true;
}

bool Tas58xxComponent::i2s_prime_write_(const uint8_t *data, size_t len, size_t *bytes_written) {
  if (this->prime_tx_handle_ == nullptr) return false;

  static constexpr int ATTEMPT_TIMEOUT_MS = 2;
  static constexpr int MAX_ATTEMPTS = 10;
  // 20ms worst case - speaker component uses 60ms but in dedicated FreeRTOS task
  // esp32 completes in 2 attempts => 4ms

  for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
    esp_err_t err = i2s_channel_write(this->prime_tx_handle_, data, len, bytes_written,
                                       pdMS_TO_TICKS(ATTEMPT_TIMEOUT_MS));
    if (err == ESP_OK && *bytes_written == len) {
      ESP_LOGD(TAG, "I2S Prime Write completed for %u bytes (attempt %d)",
                (unsigned) *bytes_written, attempt);
      return true;
    }
    if (err == ESP_ERR_TIMEOUT && *bytes_written == 0) {
      continue;  // clock still settling — retry, not a real failure yet
    }
    ESP_LOGW(TAG, "I2S Prime Write incomplete: %u of %u bytes (err=%d, attempt %d)",
              (unsigned) *bytes_written, (unsigned) len, (int) err, attempt);
    return false;
  }

  ESP_LOGE(TAG, "I2S Prime Write failed to succeed after %d attempts", MAX_ATTEMPTS);
  return false;
}

void Tas58xxComponent::i2s_prime_close_channel_() {
  if (this->prime_tx_handle_ != nullptr) {
    i2s_channel_disable(this->prime_tx_handle_);
    i2s_del_channel(this->prime_tx_handle_);
    this->prime_tx_handle_ = nullptr;

    // detach dout from the GPIO matrix and drive it low — i2s_del_channel() does not undo esp_rom_gpio_connect_out_signal()
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

// write up to 20 bytes (BIQUAD_SIZE) to a book and page starting at subaddress
// limited to writing across one page boundary as is required for tas5805m while tas5825m has biquads aligned to page boundaries
bool Tas58xxComponent::biquad_write_bytes_(uint8_t book, uint8_t page, uint8_t sub_addr, uint8_t* biquad, uint8_t number_bytes) {
  // Biquad addressing constants
  static constexpr uint8_t PAGE_SIZE = 0x80;           		// 0x7F + 1 = 0x80
  static constexpr uint8_t MINIMUM_PAGE_SUBADDR = 0x08;   // start subaddr for pages = 0x08

  // check for usage error on number bytes to write
  if (number_bytes != BIQUAD_SIZE) {
    ESP_LOGE(TAG, "Incorrect biquad size");
    return false;
  }

  uint8_t bytes_in_block1{BIQUAD_SIZE};
  uint8_t bytes_in_block2{0};

  if ((sub_addr + BIQUAD_SIZE) > PAGE_SIZE) {
    bytes_in_block1 = PAGE_SIZE - sub_addr;
    bytes_in_block2 = BIQUAD_SIZE - bytes_in_block1;
  }

  if (!this->set_book_and_page_(book, page)) return false;
  if (!this->tas58xx_write_bytes_(sub_addr, biquad, bytes_in_block1)) return false;

  if (bytes_in_block2 != 0) {
    uint8_t next_page = page + 1;

    //ESP_LOGD(TAG, "Writing new page:0x%02X", next_page);

    // book already set so just change to next page
    if (!this->tas58xx_write_byte_(TAS58XX_PAGE_SET, next_page)) {
      ESP_LOGW(TAG, "%s setting next page", ERROR);
      return false;
    }

    if (!this->tas58xx_write_bytes_(MINIMUM_PAGE_SUBADDR, biquad + bytes_in_block1, bytes_in_block2)) return false;
  }

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
    ESP_LOGE(TAG, "%s code:%d writing address:0x%02X bytes:%d ", ERROR, error_code, a_register, number_bytes);
    this->i2c_error_ = (uint8_t)error_code;
    return false;
  }
  return true;
}

}  // namespace esphome::tas58xx
