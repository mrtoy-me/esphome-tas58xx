#pragma once

#include "esphome/components/audio_dac/audio_dac.h"
#include "esphome/components/i2s_audio/i2s_audio.h"
#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/hal.h"
#include <driver/i2s_std.h>
#include <driver/gpio.h>

#ifdef USE_TAS58XX_BINARY_SENSOR
#include <array>
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif

#include "tas58xx_defs.h"
#include "tas58xx_eq_common.h"
#include "tas58xx_eq_profiles.h"



namespace esphome::tas58xx {

class Tas58xxComponent final : public audio_dac::AudioDac, public PollingComponent, public i2c::I2CDevice, public i2s_audio::I2SAudioOut {
 public:
  void setup() override;

  void update() override;

  void dump_config() override;

  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void set_dout_pin(int pin) { this->dout_pin_ = static_cast<gpio_num_t>(pin); }

  void set_enable_pin(GPIOPin *enable) { this->enable_pin_ = enable; }

  // optional YAML config

  void config_analog_gain(float analog_gain) { this->tas58xx_analog_gain_ = analog_gain; }

  void config_dac_mode(DacMode dac_mode) {this->tas58xx_dac_mode_ = dac_mode; }

  void config_modulation_scheme(ModulationScheme modulation_scheme) {this->tas58xx_modulation_scheme_ = modulation_scheme; }

  void config_eq_mode(uint8_t configured_eq_mode) { this->configured_eq_mode_ = static_cast<EqMode>(configured_eq_mode); }

  void config_input_mixer_mode(InputMixerMode mixer_mode) {this->tas58xx_input_mixer_mode_ = mixer_mode; }

  // configured maximum and minimum with units dB
  void config_volume_max(float volume_max) { this->tas58xx_volume_max_ = static_cast<int8_t>(volume_max); }
  void config_volume_min(float volume_min) { this->tas58xx_volume_min_ = static_cast<int8_t>(volume_min); }

#ifdef USE_TAS58XX_BINARY_SENSOR
  // CHAN_FAULT register
  SUB_BINARY_SENSOR(left_channel_dc_fault)
  SUB_BINARY_SENSOR(right_channel_dc_fault)
  SUB_BINARY_SENSOR(left_channel_over_current_fault)
  SUB_BINARY_SENSOR(right_channel_over_current_fault)

  // GLOBAL_FAULT1 register
  SUB_BINARY_SENSOR(otp_crc_check_error)
  SUB_BINARY_SENSOR(bq_write_failed)
  #ifdef USE_TAS5825M_DAC
  SUB_BINARY_SENSOR(eeprom_load_error)
  #endif
  SUB_BINARY_SENSOR(pvdd_over_voltage_fault)
  SUB_BINARY_SENSOR(pvdd_under_voltage_fault)

  // GLOBAL_FAULT1 register
  #ifdef USE_TAS5825M_DAC
  SUB_BINARY_SENSOR(right_channel_cbc_current_fault)
  SUB_BINARY_SENSOR(left_channel_cbc_current_fault)
  #endif
  SUB_BINARY_SENSOR(over_temperature_shutdown_fault)

  // WARNING register
  #ifdef USE_TAS5825M_DAC
  SUB_BINARY_SENSOR(left_channel_cbc_current_warning)
  SUB_BINARY_SENSOR(right_channel_cbc_current_warning)
  SUB_BINARY_SENSOR(over_temperature_146c_warning)
  #endif

  SUB_BINARY_SENSOR(over_temperature_134c_warning)

  #ifdef USE_TAS5825M_DAC
  SUB_BINARY_SENSOR(over_temperature_122c_warning)
  SUB_BINARY_SENSOR(over_temperature_112c_warning)
  #endif
#endif

  gpio_num_t dout_pin_;

  i2s_chan_handle_t prime_tx_handle_{}; // channel open if NOT null

  uint32_t times_faults_cleared();

  void enable_dac(bool enable);

  bool is_eq_configured();

  uint8_t get_configured_dac_mode();

  uint8_t get_configured_eq_mode();

  uint8_t get_mixer_mode();

  void select_eq_mode(uint8_t select_index);

  bool set_channel_volume(Channels channel, int8_t volume_dB);

  bool set_eq_gain(Channels channel, uint8_t band_index, int8_t gain);

  bool set_eq_preset(Channels channel, uint8_t select_preset);

  bool set_input_mixer_mode(InputMixerMode mode);

  bool is_muted() override { return this->is_muted_; }
  bool set_mute_off() override;
  bool set_mute_on() override;

  float volume() override;
  bool set_volume(float value) override;

 protected:
   GPIOPin* enable_pin_{nullptr};

   void configure_active_fault_sensors_();
   bool configure_registers_();

   bool get_analog_gain_(uint8_t* raw_gain);
   bool set_analog_gain_(float gain_db);

   bool get_dac_mode_(DacMode* mode);
   bool set_dac_mode_(DacMode mode);

   bool set_deep_sleep_off_();
   bool set_deep_sleep_on_();

   bool get_digital_volume_(uint8_t* raw_volume);
   bool set_digital_volume_(uint8_t new_volume);

   bool get_eq_mode_(EqMode* current_mode);
   bool set_eq_mode_(EqMode new_mode);

   bool set_modulation_scheme_(ModulationScheme modulation);

   bool get_state_(ControlState* state);
   bool set_state_(ControlState state);

   // manage faults
   bool clear_fault_registers_();

   // low level functions
   size_t i2s_prime_();
   bool i2s_open_channel_();
   void i2s_close_channel_();

   bool set_book_and_page_(uint8_t book, uint8_t page);
   bool book_page_write_bytes_(uint8_t book, uint8_t page, uint8_t sub_addr, uint8_t* data, uint8_t number_bytes);
   bool biquad_write_bytes_(uint8_t book, uint8_t page, uint8_t sub_addr, uint8_t* biquad, uint8_t number_bytes);
   bool tas58xx_read_bytes_(uint8_t a_register, uint8_t* data, uint8_t number_bytes);
   bool tas58xx_write_byte_(uint8_t a_register, uint8_t data);
   bool tas58xx_write_bytes_(uint8_t a_register, uint8_t *data, uint8_t number_bytes);

   //// variables
   size_t i2s_prime_success_count_{0};

   EqMode configured_eq_mode_; // derived from YAML

   enum ErrorCode {
     NONE = 0,
     CONFIGURATION_FAILED,
   } error_code_{NONE};

   float tas58xx_analog_gain_; // configured in YAML

   uint8_t tas58xx_channel_preset_[NUMBER_CHANNELS]{0};
   int8_t tas58xx_channel_volume_[NUMBER_CHANNELS]{0};

   ControlState tas58xx_control_state_; // initialised in setup

   DacMode tas58xx_dac_mode_; // configured in YAML

#if defined(USE_TAS58XX_EQ_GAINS) || defined(USE_TAS58XX_EQ_PRESETS)
   bool eq_configured_{true};
#else
   bool eq_configured_{false};
#endif

#ifdef USE_TAS58XX_BINARY_SENSOR
   std::array<FaultBinarySensorProperties, MAX_FAULT_SENSORS> active_fault_sensors_{};
#endif
   uint8_t active_fault_sensor_count_{0};

   int8_t tas58xx_eq_gain_[NUMBER_CHANNELS][NUMBER_EQ_BANDS]{0}; // used if eq gain numbers are defined in YAML

   EqMode tas58xx_eq_mode_{EQ_OFF}; // current selected eq mode = EQ_OFF or EqMode configured_eq_mode_

   InputMixerMode tas58xx_input_mixer_mode_; // YAML default = STEREO

   ModulationScheme tas58xx_modulation_scheme_; // YAML default = BD Mode

   uint8_t tas58xx_raw_volume_max_; // maximum volume as digital volume register range 254 to 0
   uint8_t tas58xx_raw_volume_min_; // minimum volume as digital volume register range 254 to 0

   int8_t tas58xx_volume_max_;  // YAML configured maximum volume dB
   int8_t tas58xx_volume_min_;  // YAML configured maximum volume dB

   uint32_t times_faults_cleared_{0}; // counts number of times the faults register is cleared (used for publishing to sensor)

   int i2c_error_{0}; // last i2c error

   size_t number_registers_configured_{0}; // number tas58xx registers configured during 'setup'

};

}  // namespace esphome::tas58xx
