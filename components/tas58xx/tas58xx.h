#pragma once

#include "esphome/components/audio_dac/audio_dac.h"
#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/hal.h"

#include "tas58xx_defs.h"
#include "tas58xx_helpers.h"

#ifdef USE_TAS58XX_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif

namespace esphome::tas58xx {

struct EqBand;  // tas58xx_eq.h

class Tas58xxComponent : public audio_dac::AudioDac, public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;

  void update() override;

  // called by the speaker driving this DAC once its I2S clock is running
  void on_audio_started() override;

  void dump_config() override;

  float get_setup_priority() const override { return setup_priority::IO; }

  void set_enable_pin(GPIOPin *enable) { this->enable_pin_ = enable; }

  // optional YAML config

  void config_analog_gain(float analog_gain) { this->tas58xx_analog_gain_ = analog_gain; }

  void config_dac_mode(DacMode dac_mode) {this->tas58xx_dac_mode_ = dac_mode; }

  void config_modulation_scheme(ModulationScheme modulation_scheme) {this->tas58xx_modulation_scheme_ = modulation_scheme; }

  // bit mask of the EQ modes that can be selected, derived from YAML (bit EQ_ON, bit EQ_BIAMP_ON)
  void config_eq_modes(uint8_t eq_modes) { this->eq_modes_ = eq_modes; }

  void config_ignore_fault_mode(ExcludeIgnoreMode ignore_fault_mode) {
    this->ignore_clock_faults_when_clearing_faults_ = (ignore_fault_mode == ExcludeIgnoreMode::CLOCK_FAULT);
  }

  void config_input_mixer_mode(InputMixerMode mixer_mode) {this->tas58xx_input_mixer_mode_ = mixer_mode; }

  // configured maximum and minimum with units dB
  void config_volume_max(float volume_max) { this->tas58xx_volume_max_ = static_cast<int8_t>(volume_max); }
  void config_volume_min(float volume_min) { this->tas58xx_volume_min_ = static_cast<int8_t>(volume_min); }


#ifdef USE_TAS58XX_BINARY_SENSOR
  void config_exclude_fault(ExcludeIgnoreMode exclude_fault) {
    this->exclude_clock_fault_from_have_faults_ = (exclude_fault == ExcludeIgnoreMode::CLOCK_FAULT);
  }

  SUB_BINARY_SENSOR(have_fault)
  SUB_BINARY_SENSOR(left_channel_dc_fault)
  SUB_BINARY_SENSOR(right_channel_dc_fault)
  SUB_BINARY_SENSOR(left_channel_over_current_fault)
  SUB_BINARY_SENSOR(right_channel_over_current_fault)

  SUB_BINARY_SENSOR(otp_crc_check_error)
  SUB_BINARY_SENSOR(bq_write_failed_fault)
  SUB_BINARY_SENSOR(clock_fault)
  SUB_BINARY_SENSOR(pvdd_over_voltage_fault)
  SUB_BINARY_SENSOR(pvdd_under_voltage_fault)

  SUB_BINARY_SENSOR(over_temperature_shutdown_fault)
  SUB_BINARY_SENSOR(over_temperature_warning)
#endif

  void enable_dac(bool enable);

  uint8_t get_configured_dac_mode();

  uint8_t get_mixer_mode();
  bool set_input_mixer_mode(InputMixerMode mode);

  bool set_channel_volume(Channels channel, int8_t volume_dB);

  void select_eq_mode(uint8_t select_index);
  uint8_t get_highest_eq_select_index();

  bool set_eq_gain(Channels channel, uint8_t band_index, int8_t gain);

#ifdef USE_TAS58XX_EQ_GAINS
  // the filter of each EQ gain number band, a constant table of NUMBER_EQ_BANDS bands for the channel
  // when not set, EQ_DEFAULT_BANDS is used
  void config_eq_bands(Channels channel, const EqBand *bands) { this->eq_bands_[channel] = bands; }
#endif

  bool set_eq_preset(Channels channel, uint8_t select_preset);

  bool is_muted() override { return this->is_muted_; }
  bool set_mute_off() override;
  bool set_mute_on() override;

  uint32_t times_faults_cleared();


  float volume() override { return this->tas58xx_volume_; }
  bool set_volume(float value) override;

 protected:
   GPIOPin* enable_pin_{nullptr};

   bool configure_registers_();

   bool is_dac_playing_();
   bool can_write_dsp_();
   bool write_dsp_settings_();

   // write saved settings to the DSP, only called when the I2S clock is running
   bool write_channel_volume_(Channels channel);
   bool write_eq_channel_(Channels channel, bool filters);
   bool write_eq_channels_(bool rewrite_filters);
   bool write_eq_biquads_(Channels channel, uint8_t biquad_index, const tas58xx_helpers::BiquadCoefficients *biquads,
                          uint8_t count);
   bool write_eq_gain_(Channels channel, uint8_t band_index);
#ifdef USE_TAS58XX_EQ_GAINS
   const EqBand &eq_band_(Channels channel, uint8_t band_index);
   tas58xx_helpers::BiquadCoefficients eq_gain_biquad_(Channels channel, uint8_t band_index);
   void log_eq_gain_(Channels channel, uint8_t band_index);
#endif
   bool write_eq_mode_();
   bool write_eq_preset_(Channels channel);
   bool write_input_mixer_mode_();

   bool set_analog_gain_(float gain_db);

   bool set_dac_mode_(DacMode mode);

   bool set_deep_sleep_off_();
   bool set_deep_sleep_on_();

   bool set_digital_volume_(uint8_t raw_volume);

   bool set_eq_mode_(EqMode new_mode);

   bool set_modulation_scheme_(ModulationScheme modulation);

   bool set_state_(ControlState state, bool muted);

   // manage faults
   bool clear_fault_registers_();

#ifdef USE_TAS58XX_BINARY_SENSOR
   void publish_faults_();
   void publish_channel_faults_();
   void publish_global_faults_();
#endif

   bool read_fault_registers_();

   // low level functions
   bool set_book_and_page_(uint8_t book, uint8_t page);
   bool dsp_write_bytes_(uint8_t book, uint8_t page, uint8_t sub_addr, const uint8_t *data, size_t length);
   bool tas58xx_read_bytes_(uint8_t a_register, uint8_t* data, uint8_t number_bytes);
   bool tas58xx_write_byte_(uint8_t a_register, uint8_t data);
   bool tas58xx_write_bytes_(uint8_t a_register, const uint8_t *data, uint8_t number_bytes);

   //// variables
   uint8_t eq_modes_{0}; // derived from YAML: bit mask of selectable EQ modes

   enum ErrorCode {
     NONE = 0,
     CONFIGURATION_FAILED,
   } error_code_{NONE};

   // configured by YAML
#ifdef USE_TAS58XX_BINARY_SENSOR
   bool exclude_clock_fault_from_have_faults_; // YAML default = true
#endif

   bool ignore_clock_faults_when_clearing_faults_; // YAML default = true

   float tas58xx_analog_gain_; // configured in YAML

   uint8_t tas58xx_channel_preset_[NUMBER_CHANNELS]{0};
   int8_t tas58xx_channel_volume_[NUMBER_CHANNELS]{0};

   ControlState tas58xx_control_state_{CTRL_HI_Z}; // set to CTRL_PLAY in setup by configure_registers_

   DacMode tas58xx_dac_mode_; // configured in YAML

   int8_t tas58xx_eq_gain_[NUMBER_CHANNELS][NUMBER_EQ_BANDS]{}; // used if eq gain numbers are defined in YAML

#ifdef USE_TAS58XX_EQ_GAINS
   // only the biquads defined in YAML are written, the others stay at their power on (flat) state
   uint16_t eq_gain_defined_[NUMBER_CHANNELS]{0}; // one bit per band, set by an EQ gain number
   const EqBand *eq_bands_[NUMBER_CHANNELS]{nullptr, nullptr};
#endif
#ifdef USE_TAS58XX_EQ_PRESETS
   bool eq_preset_defined_[NUMBER_CHANNELS]{false}; // set by an EQ preset select
#endif
#if defined(USE_TAS58XX_EQ_GAINS) || defined(USE_TAS58XX_EQ_PRESETS)
   // true when the DSP holds the gains or preset of the channel, false when flat (the reset state)
   bool eq_filters_written_[NUMBER_CHANNELS]{false, false};
#endif

   int8_t eq_ganged_{-1}; // ganging written to the DSP, -1 until written
   EqMode tas58xx_eq_mode_{EQ_OFF}; // current selected eq mode

   InputMixerMode tas58xx_input_mixer_mode_; // YAML default = STEREO

   ModulationScheme tas58xx_modulation_scheme_; // YAML default = BD Mode

   float tas58xx_volume_{0}; // last volume written, range 0.0 to 1.0

   int8_t tas58xx_volume_max_;  // YAML configured maximum volume dB
   int8_t tas58xx_volume_min_;  // YAML configured minimum volume dB

   //// fault processing variables
   bool is_fault_to_clear_{false}; // false so clear fault registers is skipped on first update

   bool is_new_channel_fault_{true};  // conditionally publish binary sensors - initially true so published on first update
   bool is_new_common_fault_{true};
   bool is_new_global_fault_{true};
   bool is_new_over_temperature_issue_{true};

   Tas58xxFault tas58xx_faults_;  // current state of faults

   uint32_t times_faults_cleared_{0}; // counts number of times the faults register is cleared (used for publishing to sensor)

   //// utility variables used by update and dump_config
   bool dsp_ready_{false}; // DSP settings are written only once the I2S clock has been seen

   bool faults_published_{false}; // all binary sensors are published on first update

   uint8_t i2c_error_{0}; // last i2c error

   uint16_t number_registers_configured_{0}; // number tas58xx registers configured during 'setup'

};

}  // namespace esphome::tas58xx
