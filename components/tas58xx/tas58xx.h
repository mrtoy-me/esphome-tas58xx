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

// one EQ biquad of a channel, set from YAML: an EQ band (with a gain number) or a fixed filter (no gain)
struct EqBiquad {
  FilterType type;
  uint16_t frequency;
  double q_factor;  // double, so a Q such as 2.144908 is the same value TI PurePath Console uses
  bool fixed;
};

// text of each FilterType, for logs
static constexpr const char *const FILTER_TYPE_TEXT[] = {"All Pass",  "Equalizer", "Peaking", "Low Shelf", "High Shelf",
                                                         "Low Pass",  "High Pass", "Notch",   "Band Pass"};

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

  // YAML volume_follows: makes `follower` copy the volume, mute and audio start of this DAC (its leader)
  void add_volume_follower(Tas58xxComponent *follower);

  uint8_t get_configured_dac_mode();

  uint8_t get_mixer_mode();
  bool set_input_mixer_mode(InputMixerMode mode);

  bool set_channel_volume(Channels channel, int8_t volume_dB);

  void select_eq_mode(uint8_t select_index);
  uint8_t get_highest_eq_select_index();

  // used by eq gain numbers, biquad_index is the biquad allocated to the band from YAML
  bool set_eq_gain(Channels channel, uint8_t biquad_index, int8_t gain);

#ifdef USE_TAS58XX_EQ_GAINS
  // the EQ bands and fixed filters of a channel, each in the biquad allocated to it from YAML
  void add_eq_biquad(Channels channel, uint8_t biquad_index, FilterType type, uint16_t frequency, double q_factor,
                     bool fixed);
  const EqBiquad &get_eq_biquad(Channels channel, uint8_t biquad_index) const {
    return this->eq_biquads_[channel][biquad_index];
  }
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
   bool write_eq_biquads_(Channels channel, uint8_t biquad_index, const BiquadCoefficients *biquads,
                          uint8_t count);
   bool write_eq_gain_(Channels channel, uint8_t biquad_index);
#ifdef USE_TAS58XX_EQ_GAINS
   BiquadCoefficients eq_biquad_coefficients_(Channels channel, uint8_t biquad_index);
   void log_eq_biquad_(Channels channel, uint8_t biquad_index);
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

   // volume leader and followers (YAML volume_follows), see the members below
   void send_volume_to_followers_();
   void send_mute_to_followers_();
   void on_leader_audio_started_();
   void sync_with_volume_leader_();

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

   int8_t tas58xx_eq_gain_[NUMBER_CHANNELS][NUMBER_EQ_BANDS]{}; // gain of each EQ band biquad, set by its number

#ifdef USE_TAS58XX_EQ_GAINS
   // only the biquads defined in YAML are written, the others stay at their power on (flat) state
   EqBiquad eq_biquads_[NUMBER_CHANNELS][NUMBER_EQ_BANDS]{};
   uint16_t eq_biquad_used_[NUMBER_CHANNELS]{0};   // one bit per biquad defined in YAML
   uint16_t eq_biquad_fixed_[NUMBER_CHANNELS]{0};  // one bit per fixed filter, kept when the channel is set flat
   bool eq_fixed_written_[NUMBER_CHANNELS]{false, false};  // fixed filters are in the DSP
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
   // last value written to DIG_VOL, so an unchanged volume is not written again; 0xFF (mute, never set by
   // set_volume) until the first write
   uint8_t tas58xx_raw_volume_{0xFF};

   // Volume leader and followers (YAML volume_follows). A speaker drives only the one audio_dac it names (the
   // leader); a follower is another DAC on the same I2S output that copies the leader:
   // - volume: passed as the 0.0 to 1.0 position, which the follower maps onto its own volume_min to volume_max
   // - mute: passed when it is set
   // - audio start: passed from on_audio_started(); the follower checks it is playing before writing its DSP
   // A leader keeps its followers as a singly linked list (first_volume_follower_, then each follower's
   // next_volume_follower_), as sensor filters do. YAML validation forbids chains, so a follower never has followers.
   Tas58xxComponent *volume_leader_{nullptr};          // set on a follower: the DAC it copies
   Tas58xxComponent *first_volume_follower_{nullptr};  // set on a leader: start of its follower list
   Tas58xxComponent *next_volume_follower_{nullptr};   // set on a follower: next follower of the same leader

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


   uint16_t number_registers_configured_{0}; // number tas58xx registers configured during 'setup'

};

}  // namespace esphome::tas58xx
