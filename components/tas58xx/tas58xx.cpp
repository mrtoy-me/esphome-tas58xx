#include "tas58xx.h"
#include "tas58xx_eq.h"
#include "tas58xx_helpers.h"
#include "tas58xx_startup.h"

#include <algorithm>

#include "esphome/core/log.h"

#include <cmath>

namespace esphome::tas58xx {

#ifdef USE_TAS5805M_DAC
ESPHOME_LOG_TAG(TAG, "tas5805m");
#else
ESPHOME_LOG_TAG(TAG, "tas5825m");
#endif

static constexpr const char* ERROR = "Error";
static constexpr const char* MIXER_MODE = "Mixer Mode";
#ifdef USE_TAS58XX_EQ_GAINS
static constexpr const char* EQ_BAND = "EQ Band";
#endif

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
}

bool Tas58xxComponent::configure_registers_() {
  static constexpr uint8_t ESPHOME_MAXIMUM_DELAY = 5; // milliseconds

  uint16_t i = 0;
  uint16_t counter = 0;
  uint16_t number_configurations = sizeof(TAS58XX_CONFIG) / sizeof(TAS58XX_CONFIG[0]);

  while (i < number_configurations) {
    switch (TAS58XX_CONFIG[i].addr) {
      case TAS58XX_CFG_META_DELAY:
        if (TAS58XX_CONFIG[i].value > ESPHOME_MAXIMUM_DELAY) {
          ESP_LOGE(TAG, "%s startup delay %d ms is too long", ERROR, TAS58XX_CONFIG[i].value);
          return false;
        }
        delay(TAS58XX_CONFIG[i].value);
        break;
      default:
        if (!this->write_byte(TAS58XX_CONFIG[i].addr, TAS58XX_CONFIG[i].value)) {
          ESP_LOGE(TAG, "%s writing startup register 0x%02X", ERROR, TAS58XX_CONFIG[i].addr);
          return false;
        }
        counter++;
        break;
    }
    i++;
  }
  this->number_registers_configured_ = counter;

  // enable Tas58xx
  if (!this->set_modulation_scheme_(this->tas58xx_modulation_scheme_)) return false;

  if (!this->set_dac_mode_(this->tas58xx_dac_mode_)) return false;

  if (!this->set_analog_gain_(this->tas58xx_analog_gain_)) return false;

  float initial_volume = remap<float, float>(0.0f, this->tas58xx_volume_min_, this->tas58xx_volume_max_, 0.0f, 1.0f);
  if (!this->set_volume(initial_volume)) return false;

  if (!this->set_state_(CTRL_PLAY, this->is_muted_)) return false;

  // clear any faults latched while the tas58xx was powering up
  if (!this->write_byte(TAS58XX_FAULT_CLEAR, TAS58XX_ANALOG_FAULT_CLEAR)) {
    ESP_LOGW(TAG, "%s initialising faults", ERROR);
  }
  return true;
}

// DSP settings (EQ mode, input mixer, channel volumes, EQ gains or presets) can only be written
// once the tas58xx has an I2S clock and are kept when the clock stops

// speakers that support it (ESPHome 2026.10.0 or later) call this once the I2S clock is running
void Tas58xxComponent::on_audio_started() {
  if (this->is_failed() || this->dsp_ready_) return;
  ESP_LOGD(TAG, "Audio started");
  this->write_dsp_settings_();
}

// the tas58xx only reaches Play with a running I2S clock
bool Tas58xxComponent::is_dac_playing_() {
  static constexpr uint8_t POWER_STATE_MASK = 0x03;
  uint8_t power_state;
  if (!this->read_byte(TAS58XX_POWER_STATE, &power_state)) {
    ESP_LOGW(TAG, "%s reading Power State", ERROR);
    return false;
  }
  return (power_state & POWER_STATE_MASK) == CTRL_PLAY;
}

// true if DSP settings can be written now. If the tas58xx is not playing (no I2S clock)
// the value is only saved and all settings are rewritten at the next audio start or update
bool Tas58xxComponent::can_write_dsp_() {
  if (!this->dsp_ready_) return false;
  if (this->is_dac_playing_()) return true;
  this->dsp_ready_ = false;
  ESP_LOGD(TAG, "DAC not playing, settings written at next audio start");
  return false;
}

bool Tas58xxComponent::write_dsp_settings_() {
  // callers have checked the I2S clock is running, write all saved settings
  bool ok = true;

  // setup Eq Mode first
  if (!this->write_eq_mode_()) {
    ESP_LOGW(TAG, "%s setting EQ Mode: %s", ERROR, EQ_MODE_TEXT[this->tas58xx_eq_mode_]);
    ok = false;
  }

  if (!this->write_input_mixer_mode_()) {
    ESP_LOGW(TAG, "%s setting %s: %s", ERROR, MIXER_MODE, INPUT_MIXER_MODE_TEXT[this->tas58xx_input_mixer_mode_]);
    ok = false;
  }

#ifdef USE_TAS58XX_CHANNEL_VOLUMES
  if (!this->write_channel_volume_(LEFT_CHANNEL)) {
    ESP_LOGW(TAG, "%s setting Left Channel Gain: %ddb", ERROR, this->tas58xx_channel_volume_[LEFT_CHANNEL]);
    ok = false;
  }

  if (!this->write_channel_volume_(RIGHT_CHANNEL)) {
    ESP_LOGW(TAG, "%s setting Right Channel Gain: %ddb", ERROR, this->tas58xx_channel_volume_[RIGHT_CHANNEL]);
    ok = false;
  }
#endif

#if defined(USE_TAS58XX_EQ_GAINS) || defined(USE_TAS58XX_EQ_PRESETS)
  if (!this->write_eq_channels_(true)) ok = false;
#endif

  if (!ok) {
    // retried at next audio start or update
    this->dsp_ready_ = false;
    return false;
  }
  // from now on setters write changes straight away while the tas58xx is playing
  this->dsp_ready_ = true;
  ESP_LOGD(TAG, "DSP settings written");
  return true;
}

void Tas58xxComponent::update() {
  // fallback for audio sources that do not call 'on_audio_started' (eg snapclient)
  // retries a failed write while the tas58xx is playing
  if (!this->dsp_ready_ && !this->is_failed() && this->is_dac_playing_()) {
    this->write_dsp_settings_();
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


  // publish all binary sensors on first update, then only when faults change
  if (!this->faults_published_) {
    this->is_new_channel_fault_ = true;
    this->is_new_common_fault_ = true;
    this->is_new_global_fault_ = true;
    this->is_new_over_temperature_issue_ = true;
    this->faults_published_ = true;
  }

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

  LOG_I2C_DEVICE(this);
  LOG_PIN("  Enable Pin: ", this->enable_pin_);

  switch (this->error_code_) {
    case CONFIGURATION_FAILED:
      ESP_LOGE(TAG, "  %s setup failed", ERROR);
      break;
    case NONE:
      ESP_LOGCONFIG(TAG,
              "  Registers Configured: %i\n"
              "  Analog Gain: %3.1fdB\n"
              "  Modulation: %s\n"
              "  DAC Mode: %s\n"
              "  Mixer Mode: %s\n"
              "  Volume Maximum: %idB\n"
              "  Volume Minimum: %idB\n"
              "  Ignore Fault: %s\n",
              this->number_registers_configured_, this->tas58xx_analog_gain_,
              this->tas58xx_modulation_scheme_ ? LOG_STR_LITERAL("1SPW Mode") : LOG_STR_LITERAL("BD Mode"),
              this->tas58xx_dac_mode_ ? LOG_STR_LITERAL("PBTL") : LOG_STR_LITERAL("BTL"),
              INPUT_MIXER_MODE_TEXT[this->tas58xx_input_mixer_mode_],
              this->tas58xx_volume_max_, this->tas58xx_volume_min_,
              this->ignore_clock_faults_when_clearing_faults_ ? LOG_STR_LITERAL("CLOCK FAULTS") : LOG_STR_LITERAL("NONE")
              );
      LOG_UPDATE_INTERVAL(this);
      break;
  }

#ifdef USE_TAS58XX_EQ_GAINS
  // fixed filters from YAML; EQ bands are listed by their numbers, biquads not listed keep their power on (flat) state
  for (uint8_t channel = LEFT_CHANNEL; channel < NUMBER_CHANNELS; channel++) {
    for (uint8_t index = 0; index < NUMBER_EQ_BANDS; index++) {
      if (!(this->eq_biquad_fixed_[channel] & (1u << index))) continue;
      const EqBiquad &biquad = this->eq_biquads_[channel][index];
      ESP_LOGCONFIG(TAG, "  %s Fixed Filter (biquad %d): %s %dHz Q %.3f", LR_CHANNEL_TEXT[channel], index + 1,
                    FILTER_TYPE_TEXT[biquad.type], biquad.frequency, biquad.q_factor);
    }
  }
#endif

#ifdef USE_TAS58XX_BINARY_SENSOR
  ESP_LOGCONFIG(TAG, "Tas58xx Binary Sensors:");
  LOG_BINARY_SENSOR("  ", "Any Faults", this->have_fault_binary_sensor_);
  ESP_LOGCONFIG(TAG, "    Exclude: %s", this->exclude_clock_fault_from_have_faults_ ? LOG_STR_LITERAL("CLOCK FAULTS") : LOG_STR_LITERAL("NONE"));

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

// used by select mixer mode
uint8_t Tas58xxComponent::get_configured_dac_mode() {
   return static_cast<uint8_t>(this->tas58xx_dac_mode_); // BTL = 0 , PBTL = 1
}

uint8_t Tas58xxComponent::get_mixer_mode() {
  return static_cast<uint8_t>(this->tas58xx_input_mixer_mode_);
}

bool Tas58xxComponent::set_input_mixer_mode(InputMixerMode mode) {

  this->tas58xx_input_mixer_mode_ = mode;

  // only save until the I2S clock has been seen
  if (!this->can_write_dsp_()) {
     ESP_LOGD(TAG, "Save %s: %s", MIXER_MODE, INPUT_MIXER_MODE_TEXT[mode]);
     return true;
  }
  return this->write_input_mixer_mode_();
}

bool Tas58xxComponent::write_input_mixer_mode_() {
  const InputMixerMode mode = this->tas58xx_input_mixer_mode_;

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

  if (!this->dsp_write_bytes_(TAS58XX_AUDIO_CTRL_BOOK, TAS58XX_MIXER_GAIN_PAGE, TAS58XX_MIXER_GAIN_SUBADDR,
                              reinterpret_cast<const uint8_t *>(&mixer_coefficients), sizeof(MixerCoefficients))) {
    ESP_LOGW(TAG, "%s writing Input %s: %s", ERROR, MIXER_MODE, INPUT_MIXER_MODE_TEXT[mode]);
    return false;
  }

  ESP_LOGD(TAG, "Input %s >> %s", MIXER_MODE, INPUT_MIXER_MODE_TEXT[mode]);
  return true;
}

bool Tas58xxComponent::set_channel_volume(Channels channel, int8_t volume_dB) {
#ifdef USE_TAS58XX_CHANNEL_VOLUMES
  if (volume_dB < TAS58XX_CHANNEL_VOLUME_MIN_DB || volume_dB > TAS58XX_CHANNEL_VOLUME_MAX_DB) {
    ESP_LOGE(TAG, "Invalid %s Channel Volume: %ddB", LR_CHANNEL_TEXT[channel], volume_dB);
    return false;
  }

  this->tas58xx_channel_volume_[channel] = volume_dB;

  // only save until the I2S clock has been seen
  if (!this->can_write_dsp_()) {
    ESP_LOGD(TAG, "Save %s Channel Volume: %ddB", LR_CHANNEL_TEXT[channel], volume_dB);
    return true;
  }
  return this->write_channel_volume_(channel);
#else
  return true;
#endif
}

bool Tas58xxComponent::write_channel_volume_(Channels channel) {
#ifdef USE_TAS58XX_CHANNEL_VOLUMES
  const int8_t volume_dB = this->tas58xx_channel_volume_[channel];
  int32_t little_endian_9_23 = gain_to_fixed_9_23(volume_dB);

  if (!this->dsp_write_bytes_(TAS58XX_AUDIO_CTRL_BOOK, TAS58XX_CHANNEL_VOLUME_PAGE, TAS58XX_CHANNEL_VOLUME_SUBADDR[channel],
                              reinterpret_cast<const uint8_t *>(&little_endian_9_23), sizeof(little_endian_9_23))) {
    ESP_LOGW(TAG, "%s writing %s Channel Volume: %ddb", ERROR, LR_CHANNEL_TEXT[channel], volume_dB);
    return false;
  }

  ESP_LOGD(TAG, "%s Channel Volume >> %ddB", LR_CHANNEL_TEXT[channel], volume_dB);
#endif
  return true;
}

// used by select eq mode, the options are EQ Off followed by each EQ mode in eq_modes_
void Tas58xxComponent::select_eq_mode(uint8_t select_index) {
  uint8_t option = 0;
  for (uint8_t mode = EQ_ON; mode < NUMBER_EQ_MODES; mode++) {
    if ((this->eq_modes_ & (1u << mode)) && ++option == select_index) {
      this->set_eq_mode_(static_cast<EqMode>(mode));
      return;
    }
  }
  this->set_eq_mode_(EqMode::EQ_OFF);
}

// used by select eq mode to start at the highest EQ mode configured, which is its last option
uint8_t Tas58xxComponent::get_highest_eq_select_index() {
  uint8_t option = 0;
  for (uint8_t mode = EQ_ON; mode < NUMBER_EQ_MODES; mode++) {
    if (this->eq_modes_ & (1u << mode)) option++;
  }
  return option;
}

// used by eq gain numbers
bool Tas58xxComponent::set_eq_gain(Channels channel, uint8_t biquad_index, int8_t gain) {
#ifdef USE_TAS58XX_EQ_GAINS

  if (biquad_index >= NUMBER_EQ_BANDS || !(this->eq_biquad_used_[channel] & (1u << biquad_index))) {
    ESP_LOGE(TAG, "Invalid %s biquad: %d", EQ_BAND, biquad_index + 1);
    return false;
  }

  if (gain < TAS58XX_EQ_MIN_DB || gain > TAS58XX_EQ_MAX_DB) {
    ESP_LOGE(TAG, "Invalid %s Channel %s biquad %d Gain: %ddB", LR_CHANNEL_TEXT[channel], EQ_BAND, biquad_index + 1,
             gain);
    return false;
  }

  this->tas58xx_eq_gain_[channel][biquad_index] = gain;

  // only save until the I2S clock has been seen
  if (!this->can_write_dsp_()) {
    ESP_LOGD(TAG, "Save %s Channel %s biquad %d Gain: %ddB", LR_CHANNEL_TEXT[channel], EQ_BAND, biquad_index + 1,
             gain);
    return true;
  }
  // a channel that is flat in the DSP gets its gains when an EQ mode needs them
  if (!this->eq_filters_written_[channel]) return true;
  return this->write_eq_gain_(channel, biquad_index);
#else
  return true;
#endif
}

#ifdef USE_TAS58XX_EQ_GAINS
void Tas58xxComponent::add_eq_biquad(Channels channel, uint8_t biquad_index, FilterType type, uint16_t frequency,
                                     double q_factor, bool fixed) {
  if (channel >= NUMBER_CHANNELS || biquad_index >= NUMBER_EQ_BANDS) return;
  this->eq_biquads_[channel][biquad_index] = {type, frequency, q_factor, fixed};
  this->eq_biquad_used_[channel] |= static_cast<uint16_t>(1u << biquad_index);
  if (fixed) this->eq_biquad_fixed_[channel] |= static_cast<uint16_t>(1u << biquad_index);
}

// fixed filters have no gain
BiquadCoefficients Tas58xxComponent::eq_biquad_coefficients_(Channels channel, uint8_t biquad_index) {
  const EqBiquad &biquad = this->eq_biquads_[channel][biquad_index];
  const int8_t gain = biquad.fixed ? 0 : this->tas58xx_eq_gain_[channel][biquad_index];
  return make_biquad(biquad.type, EQ_SAMPLE_RATE, biquad.frequency, gain, biquad.q_factor);
}

void Tas58xxComponent::log_eq_biquad_(Channels channel, uint8_t biquad_index) {
  const EqBiquad &biquad = this->eq_biquads_[channel][biquad_index];
  if (biquad.fixed) {
    ESP_LOGD(TAG, "%s Channel Fixed Filter biquad %d: %s %dHz Q %.3f", LR_CHANNEL_TEXT[channel], biquad_index + 1,
             FILTER_TYPE_TEXT[biquad.type], biquad.frequency, biquad.q_factor);
  } else {
    ESP_LOGD(TAG, "%s Channel %s biquad %d: %s %dHz Gain >> %ddB", LR_CHANNEL_TEXT[channel], EQ_BAND, biquad_index + 1,
             FILTER_TYPE_TEXT[biquad.type], biquad.frequency, this->tas58xx_eq_gain_[channel][biquad_index]);
  }
}
#endif

bool Tas58xxComponent::write_eq_gain_(Channels channel, uint8_t biquad_index) {
#ifdef USE_TAS58XX_EQ_GAINS
  const BiquadCoefficients biquad = this->eq_biquad_coefficients_(channel, biquad_index);
  if (!this->write_eq_biquads_(channel, biquad_index, &biquad, 1)) {
    ESP_LOGW(TAG, "%s writing %s Channel %s biquad %d Gain", ERROR, LR_CHANNEL_TEXT[channel], EQ_BAND,
             biquad_index + 1);
    return false;
  }
  this->log_eq_biquad_(channel, biquad_index);
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
  this->eq_preset_defined_[channel] = true;

  // only save until the I2S clock has been seen
  if (!this->can_write_dsp_()) {
    ESP_LOGD(TAG, "Save %s Channel EQ Preset index: %d", LR_CHANNEL_TEXT[channel], select_preset);
    return true;
  }
  if (!this->eq_filters_written_[channel]) return true;
  return this->write_eq_preset_(channel);
#else
  return true;
#endif
}

bool Tas58xxComponent::write_eq_preset_(Channels channel) {
#ifdef USE_TAS58XX_EQ_PRESETS
  const uint8_t select_preset = this->tas58xx_channel_preset_[channel];
  const EqPreset &preset = EQ_PRESETS[select_preset];

  // BQ1 and BQ2 are the same filter, BQ3 is a high shelf or all pass
  const BiquadCoefficients filter =
      make_biquad(preset.filter, EQ_SAMPLE_RATE, preset.cutoff_frequency, 0, 0.0);
  const BiquadCoefficients shelf =
      (preset.shelf_frequency == 0)
          ? ALL_PASS
          : make_biquad(FILTER_TYPE_HIGH_SHELF, EQ_SAMPLE_RATE, preset.shelf_frequency,
                                         EQ_PRESET_SHELF_GAIN, EQ_PRESET_SHELF_QFACTOR);
  const BiquadCoefficients biquads[EQ_PROFILE_BIQUADS] = {filter, filter, shelf};

  if (!this->write_eq_biquads_(channel, 0, biquads, EQ_PROFILE_BIQUADS)) {
    ESP_LOGW(TAG, "%s writing %s Channel EQ Preset index: %d", ERROR, LR_CHANNEL_TEXT[channel], select_preset);
    return false;
  }

  ESP_LOGD(TAG, "%s Channel EQ Preset index >> %d", LR_CHANNEL_TEXT[channel], select_preset);
#endif
  return true;
}

// write consecutive biquads of a channel, starting at biquad_index
bool Tas58xxComponent::write_eq_biquads_(Channels channel, uint8_t biquad_index,
                                         const BiquadCoefficients *biquads, uint8_t count) {
#if defined(USE_TAS58XX_EQ_GAINS) || defined(USE_TAS58XX_EQ_PRESETS)
  const AddressSequence address = eq_biquad_address(channel, biquad_index);
  return this->dsp_write_bytes_(TAS58XX_EQ_CTRL_BOOK, address.page, address.sub_addr,
                                reinterpret_cast<const uint8_t *>(biquads), count * BIQUAD_SIZE);
#else
  return true;
#endif
}

// write the biquads defined in YAML of a channel: its bands, fixed filters or preset (filters = true), or flat
// flat sets the bands and preset to all pass and keeps the fixed filters
// consecutive biquads are written together
bool Tas58xxComponent::write_eq_channel_(Channels channel, bool filters) {
  bool ok = true;
#if defined(USE_TAS58XX_EQ_GAINS) || defined(USE_TAS58XX_EQ_PRESETS)
#ifdef USE_TAS58XX_EQ_GAINS
  // only biquads in YAML are written, the other biquads keep their power on (flat) state
  const uint16_t used = this->eq_biquad_used_[channel];
  const uint16_t fixed = this->eq_biquad_fixed_[channel];
  BiquadCoefficients biquads[NUMBER_EQ_BANDS];
  uint8_t index = 0;
  while (index < NUMBER_EQ_BANDS) {
    if (!(used & (1u << index))) {
      index++;
      continue;
    }
    const uint8_t first = index;
    for (; index < NUMBER_EQ_BANDS && (used & (1u << index)); index++) {
      biquads[index] = (filters || (fixed & (1u << index))) ? this->eq_biquad_coefficients_(channel, index) : ALL_PASS;
    }
    if (!this->write_eq_biquads_(channel, first, &biquads[first], index - first)) {
      ESP_LOGW(TAG, "%s writing %s Channel biquads %d to %d", ERROR, LR_CHANNEL_TEXT[channel], first + 1, index);
      ok = false;
    } else {
      for (uint8_t written = first; written < index; written++) {
        if (filters || (fixed & (1u << written))) this->log_eq_biquad_(channel, written);
      }
    }
  }
  if (ok && fixed != 0) this->eq_fixed_written_[channel] = true;
#endif
#ifdef USE_TAS58XX_EQ_PRESETS
  if (this->eq_preset_defined_[channel]) {
    if (filters) {
      if (!this->write_eq_preset_(channel)) ok = false;
    } else {
      static constexpr BiquadCoefficients FLAT_PRESET[EQ_PROFILE_BIQUADS] = {
          ALL_PASS, ALL_PASS, ALL_PASS};
      if (!this->write_eq_biquads_(channel, 0, FLAT_PRESET, EQ_PROFILE_BIQUADS)) ok = false;
    }
  }
#endif
  if (ok) {
    this->eq_filters_written_[channel] = filters;
    if (!filters) ESP_LOGD(TAG, "%s Channel EQ >> flat", LR_CHANNEL_TEXT[channel]);  // one entry for each channel set flat
  }
#endif
  return ok;
}

// write only the channels whose biquads must change for the current EQ mode
// rewrite_filters: also write the filters already in the DSP, as values may have been saved while not playing
bool Tas58xxComponent::write_eq_channels_(bool rewrite_filters) {
  bool ok = true;
#if defined(USE_TAS58XX_EQ_GAINS) || defined(USE_TAS58XX_EQ_PRESETS)
  for (uint8_t channel = LEFT_CHANNEL; channel < NUMBER_CHANNELS; channel++) {
    bool defined = false;
    bool fixed_to_write = false;  // fixed filters are written when a channel is first set flat too
#ifdef USE_TAS58XX_EQ_GAINS
    defined |= this->eq_biquad_used_[channel] != 0;
    fixed_to_write = this->eq_biquad_fixed_[channel] != 0 && !this->eq_fixed_written_[channel];
#endif
#ifdef USE_TAS58XX_EQ_PRESETS
    defined |= this->eq_preset_defined_[channel];
#endif
    if (!defined) continue;  // nothing in YAML, the channel stays flat

    const Channels ch = static_cast<Channels>(channel);
    const bool written = this->eq_filters_written_[channel];
    switch (eq_channel_need(this->tas58xx_eq_mode_, ch)) {
      case EQ_CHANNEL_FILTERS:
        if ((!written || rewrite_filters) && !this->write_eq_channel_(ch, true)) ok = false;
        break;
      case EQ_CHANNEL_FLAT:
        if ((written || fixed_to_write) && !this->write_eq_channel_(ch, false)) ok = false;
        break;
      default:
        if (written && rewrite_filters && !this->write_eq_channel_(ch, true)) ok = false;
        break;
    }
  }
#endif
  return ok;
}

bool Tas58xxComponent::set_mute_off() {
  if (!this->is_muted_) return true;
  if (!this->set_state_(this->tas58xx_control_state_, false)) return false;
  this->is_muted_ = false;
  ESP_LOGV(TAG, "Mute Off");
  return true;
}

// set bit 3 MUTE in TAS58XX_DEVICE_CTRL_2 and retain current Control State
bool Tas58xxComponent::set_mute_on() {
  if (this->is_muted_) return true;
  if (!this->set_state_(this->tas58xx_control_state_, true)) return false;
  this->is_muted_ = true;
  ESP_LOGV(TAG, "Mute On");
  return true;
}

// used by fault sensor
uint32_t Tas58xxComponent::times_faults_cleared() {
  return this->times_faults_cleared_;
}

bool Tas58xxComponent::set_volume(float volume) {
  float new_volume = clamp(volume, 0.0f, 1.0f);
  float volume_db = remap<float, float>(new_volume, 0.0f, 1.0f, this->tas58xx_volume_min_, this->tas58xx_volume_max_);
  // round to nearest 0.5dB step of digital volume register
  uint8_t raw_volume = static_cast<uint8_t>(
      lroundf(clamp(TAS58XX_DIG_VOL_0DB - volume_db * 2.0f, 0.0f, float{TAS58XX_DIG_VOL_MINUS_103DB})));
  if (!this->set_digital_volume_(raw_volume)) return false;
  this->tas58xx_volume_ = new_volume;
  ESP_LOGV(TAG, "Volume >> %.1fdB", (TAS58XX_DIG_VOL_0DB - raw_volume) / 2.0f);
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
  if (!this->read_byte(TAS58XX_AGAIN, &current_again)) {
    ESP_LOGE(TAG, "%s reading Analog Gain", ERROR);
    return false;
  }

  // keep top 3 reserved bits combine with bottom 5 analog gain bits
  new_again = (current_again & TOP_3BITS_MASK) | new_again;
  if (!this->write_byte(TAS58XX_AGAIN, new_again)) {
    ESP_LOGE(TAG, "%s writing Analog Gain", ERROR);
    return false;
  }

  ESP_LOGD(TAG, "Analog Gain >> %fdB", gain_db);
  return true;
}

// only runs once from 'setup'
bool Tas58xxComponent::set_dac_mode_(DacMode mode) {
  uint8_t current_value;
  if (!this->read_byte(TAS58XX_DEVICE_CTRL_1, &current_value)) {
    ESP_LOGE(TAG, "%s reading DAC mode", ERROR);
    return false;
  }

  // Update bit 2 based on the mode
  if (mode == PBTL) {
      current_value |= (1 << 2);  // Set bit 2 to 1 (PBTL mode)
  } else {
      current_value &= ~(1 << 2); // Clear bit 2 to 0 (BTL mode)
  }
  if (!this->write_byte(TAS58XX_DEVICE_CTRL_1, current_value)) {
    ESP_LOGE(TAG, "%s writing DAC mode", ERROR);
    return false;
  }

  // save so 'set_dac_mode_' could be used more generally
  this->tas58xx_dac_mode_ = mode;
  ESP_LOGD(TAG, "DAC mode >> %s", this->tas58xx_dac_mode_ ? LOG_STR_LITERAL("PBTL") : LOG_STR_LITERAL("BTL"));
  return true;
}

bool Tas58xxComponent::set_deep_sleep_off_() {
  if (this->is_failed()) return false;
  if (this->tas58xx_control_state_ == CTRL_PLAY) return true; // already out of deep sleep
  if (!this->set_book_and_page_(TAS58XX_BOOK_ZERO, TAS58XX_PAGE_ZERO)) {
    ESP_LOGE(TAG, "Select Book-Page failed");
    return false;
  }

  // deep sleep off needs this sequence - tas5805 datasheet 7.4.5
  // Hi-Z, Deep Sleep, Hi-Z then Play, preserving mute state
  if (!this->set_state_(CTRL_HI_Z, this->is_muted_) ||
      !this->set_state_(CTRL_DEEP_SLEEP, this->is_muted_) ||
      !this->set_state_(CTRL_HI_Z, this->is_muted_) ||
      !this->set_state_(CTRL_PLAY, this->is_muted_)) return false;

  ESP_LOGV(TAG, "Deep Sleep >> Off");
  #if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
  if (this->is_muted_) {
    ESP_LOGV(TAG, "Mute On preserved");
  }
  #endif
  return true;
}

bool Tas58xxComponent::set_deep_sleep_on_() {
  if (this->is_failed()) return false;
  if (this->tas58xx_control_state_ == CTRL_DEEP_SLEEP) return true; // already in deep sleep
  if (!this->set_book_and_page_(TAS58XX_BOOK_ZERO, TAS58XX_PAGE_ZERO)) {
    ESP_LOGE(TAG, "Select Book-Page failed");
    return false;
  }
  if (!this->set_state_(CTRL_DEEP_SLEEP, this->is_muted_)) return false;

  ESP_LOGV(TAG, "Deep Sleep >> On");
  #if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
  if (this->is_muted_) {
    ESP_LOGV(TAG, "Mute On preserved");
  }
  #endif
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
  if (!this->write_byte(TAS58XX_DIG_VOL_CTRL, raw_volume)) {
    ESP_LOGW(TAG, "%s writing Digital Volume", ERROR);
    return false;
  }
  return true;
}

bool Tas58xxComponent::set_eq_mode_(EqMode new_mode) {
#if defined(USE_TAS58XX_EQ_GAINS) || defined(USE_TAS58XX_EQ_PRESETS)
  this->tas58xx_eq_mode_ = new_mode;

  // only save until the I2S clock has been seen
  if (!this->can_write_dsp_()) {
    ESP_LOGD(TAG, "Save EQ Mode: %s", EQ_MODE_TEXT[new_mode]);
    return true;
  }
  return this->write_eq_mode_() && this->write_eq_channels_(false);
#else
  return true;
#endif
}

bool Tas58xxComponent::write_eq_mode_() {
#if defined(USE_TAS58XX_EQ_GAINS) || defined(USE_TAS58XX_EQ_PRESETS)
  const EqMode new_mode = this->tas58xx_eq_mode_;

  const bool ganged = eq_mode_is_ganged(new_mode);
  const bool bypass = new_mode == EQ_OFF;

#ifdef USE_TAS5805M_DAC
  const uint8_t dsp_misc = TAS5805M_DSP_MISC_BYPASS_DRC_FIR | (bypass ? TAS5805M_DSP_MISC_BYPASS_EQ : 0) |
                           (ganged ? 0 : TAS5805M_DSP_MISC_SEPARATE_COEFFICIENTS);
  if (!this->write_byte(TAS5805M_DSP_MISC, dsp_misc)) {
    ESP_LOGW(TAG, "%s writing Eq Mode: %s", ERROR, EQ_MODE_TEXT[new_mode]);
    return false;
  }
#else
  // the Gang EQ and Bypass EQ flags are consecutive 32 bit values
  const uint32_t flags[2] = {ganged ? TAS5825M_EQ_FLAG_TRUE : 0, bypass ? TAS5825M_EQ_FLAG_TRUE : 0};
  if (!this->dsp_write_bytes_(TAS58XX_AUDIO_CTRL_BOOK, TAS5825M_EQ_MODE_CTRL_PAGE, TAS5825M_GANG_EQ,
                              reinterpret_cast<const uint8_t *>(flags), sizeof(flags))) {
    ESP_LOGW(TAG, "%s writing Eq Mode: %s", ERROR, EQ_MODE_TEXT[new_mode]);
    return false;
  }
#endif

  ESP_LOGD(TAG, "EQ Mode >> %s", EQ_MODE_TEXT[new_mode]);

  // log when the ganging of the left and right channel filters changes
  if (static_cast<int8_t>(ganged) != this->eq_ganged_) {
    this->eq_ganged_ = static_cast<int8_t>(ganged);
    ESP_LOGD(TAG, "EQ Ganged >> %s", ganged ? LOG_STR_LITERAL("On") : LOG_STR_LITERAL("Off"));
  }
#endif
  return true;
}

// only runs once from 'setup'
bool Tas58xxComponent::set_modulation_scheme_(ModulationScheme modulation) {
  static constexpr uint8_t MODULATION_MASK = 0b11111100; // bits 0 and 1 are modulation

  uint8_t value;
  if (!this->read_byte(TAS58XX_DEVICE_CTRL_1, &value)) {
    ESP_LOGE(TAG, "%s reading Modulation", ERROR);
    return false;
  }

  value = (value & MODULATION_MASK) | modulation;
  if (!this->write_byte(TAS58XX_DEVICE_CTRL_1, value)) {
    ESP_LOGE(TAG, "%s writing Modulation", ERROR);
    return false;
  }

  // save, so 'set_modulation_scheme_' could be used more generally
  this->tas58xx_modulation_scheme_ = modulation;
  ESP_LOGD(TAG, "Modulation >> %s", this->tas58xx_modulation_scheme_ ? LOG_STR_LITERAL("1SPW Mode") : LOG_STR_LITERAL("BD Mode"));
  return true;
}

// write Control State to DEVICE_CTRL_2 with bit 3 MUTE
bool Tas58xxComponent::set_state_(ControlState state, bool muted) {
  uint8_t ctrl_value = muted ? (state + TAS58XX_MUTE_CONTROL) : state;
  if (!this->write_byte(TAS58XX_DEVICE_CTRL_2, ctrl_value)) {
    ESP_LOGW(TAG, "%s writing Control State", ERROR);
    return false;
  }
  this->tas58xx_control_state_ = state;
  return true;
}

//// fault processing functions

bool Tas58xxComponent::clear_fault_registers_() {
  if (!this->write_byte(TAS58XX_FAULT_CLEAR, TAS58XX_ANALOG_FAULT_CLEAR)) {
    ESP_LOGW(TAG, "%s clearing faults", ERROR);
    return false;
  }
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
  if (!this->read_bytes(TAS58XX_CHAN_FAULT, current_faults, sizeof(current_faults))) {
    ESP_LOGW(TAG, "%s reading faults", ERROR);
    return false;
  }

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

// Write bytes to consecutive DSP addresses from book, page and sub_addr. When a page ends the write continues
// on the next page at subaddress 0x08. Each page is written in one I2C write of the subaddress and the data.
bool Tas58xxComponent::dsp_write_bytes_(uint8_t book, uint8_t page, uint8_t sub_addr, const uint8_t *data,
                                        size_t length) {
  static constexpr uint8_t PAGE_END = 0x80;
  static constexpr uint8_t PAGE_FIRST_SUBADDR = 0x08;
  static constexpr size_t PAGE_BYTES = PAGE_END - PAGE_FIRST_SUBADDR;

  if (!this->set_book_and_page_(book, page)) return false;

  uint8_t buffer[1 + PAGE_BYTES];
  bool ok = true;
  while (length > 0) {
    if (sub_addr >= PAGE_END) {
      // the book is already set, only change to the next page
      page++;
      sub_addr = PAGE_FIRST_SUBADDR;
      if (!this->write_byte(TAS58XX_PAGE_SET, page)) {
        ESP_LOGE(TAG, "%s setting page:0x%02X", ERROR, page);
        ok = false;
        break;
      }
    }
    const size_t bytes = std::min(length, static_cast<size_t>(PAGE_END - sub_addr));
    buffer[0] = sub_addr;
    std::copy(data, data + bytes, buffer + 1);
    const i2c::ErrorCode error_code = this->write(buffer, bytes + 1);
    if (error_code != i2c::ERROR_OK) {
      ESP_LOGE(TAG, "%s code:%d writing %d bytes to book:0x%02X page:0x%02X address:0x%02X", ERROR, error_code,
               static_cast<int>(bytes), book, page, sub_addr);
      ok = false;
      break;
    }
    data += bytes;
    sub_addr += bytes;
    length -= bytes;
  }

  // book and page are reset to zero, also after an error
  return this->set_book_and_page_(TAS58XX_BOOK_ZERO, TAS58XX_PAGE_ZERO) && ok;
}

bool Tas58xxComponent::set_book_and_page_(uint8_t book, uint8_t page) {
  if (this->write_byte(TAS58XX_PAGE_SET, TAS58XX_PAGE_ZERO) && this->write_byte(TAS58XX_BOOK_SET, book) &&
      this->write_byte(TAS58XX_PAGE_SET, page)) {
    return true;
  }
  ESP_LOGE(TAG, "%s setting book:0x%02X page:0x%02X", ERROR, book, page);
  return false;
}

}  // namespace esphome::tas58xx
