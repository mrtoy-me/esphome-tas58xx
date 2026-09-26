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

namespace esphome::tas58xx {

enum ControlState : uint8_t {
      CTRL_DEEP_SLEEP = 0x00, // Deep Sleep
      CTRL_SLEEP      = 0x01, // Sleep
      CTRL_HI_Z       = 0x02, // Hi-Z
      CTRL_PLAY       = 0x03, // Play
  };

enum DacMode : uint8_t {
  BTL  = 0, // Bridge tied load
  PBTL = 1, // Parallel load
};

enum InputMixerMode : uint8_t {
  STEREO = 0,
  STEREO_INVERSE,
  MONO,
  RIGHT,
  LEFT,
};

class Tas58xxComponent final : public audio_dac::AudioDac, public PollingComponent, public i2c::I2CDevice, public i2s_audio::I2SAudioOut {

 public:
  void setup() override;

  void update() override;

  void dump_config() override;

  float get_setup_priority() const override { return setup_priority::IO; }

  void set_dout_pin(int pin) { this->dout_pin_ = static_cast<gpio_num_t>(pin); }

  void set_enable_pin(GPIOPin *enable) { this->enable_pin_ = enable; }

  // optional YAML config

  void config_analog_gain(float analog_gain) { this->tas58xx_analog_gain_ = analog_gain; }

  void config_dac_mode(DacMode dac_mode) {this->tas58xx_dac_mode_ = dac_mode; }

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

  i2s_chan_handle_t sync_tx_handle_{}; // channel open if NOT null

  void enable_dac(bool enable);

  bool is_muted() override { return this->is_muted_; }
  bool set_mute_off() override;
  bool set_mute_on() override;

  float volume() override;
  bool set_volume(float value) override;

 protected:

  enum ErrorCode {
      NONE = 0,
      CONFIGURATION_FAILED,
    } error_code_{NONE};


#ifdef USE_TAS58XX_BINARY_SENSOR
  struct FaultBinarySensorProperties {
    binary_sensor::BinarySensor *fault_sensor{nullptr};
    uint8_t register_index{0};
    uint8_t bit_mask{0};
    bool last_state{false};
  };

  static constexpr size_t MAX_FAULT_SENSORS = 18; // maximum possible on TAS5825

#endif
   GPIOPin* enable_pin_{nullptr};

   void configure_active_fault_sensors_();
   bool configure_registers_();

   bool set_analog_gain_(float gain_db);
   bool set_dac_mode_(DacMode mode);
   bool set_deep_sleep_off_();
   bool set_deep_sleep_on_();
   bool set_input_mixer_mode_(InputMixerMode mode);
   bool set_state_(ControlState state);

   bool i2s_sync_(size_t* bytes_written, size_t* sync_attempts);
   bool i2s_open_channel_();
   void i2s_close_channel_();

   bool set_book_and_page_(uint8_t book, uint8_t page);

   bool i2s_sync_successful_{false};
   size_t i2s_sync_byte_count_{0};
   size_t i2s_sync_attempts_{0};

   float tas58xx_analog_gain_; // configured in YAML

   ControlState tas58xx_control_state_; // initialised in setup

   DacMode tas58xx_dac_mode_; // configured in YAML

#ifdef USE_TAS58XX_BINARY_SENSOR
   std::array<FaultBinarySensorProperties, MAX_FAULT_SENSORS> active_fault_sensors_{};
#endif
   uint8_t active_fault_sensor_count_{0};

   InputMixerMode tas58xx_input_mixer_mode_; // YAML default = STEREO

   uint8_t tas58xx_raw_volume_max_; // maximum volume as digital volume register range 254 to 0
   uint8_t tas58xx_raw_volume_min_; // minimum volume as digital volume register range 254 to 0

   int8_t tas58xx_volume_max_;  // YAML configured maximum volume dB
   int8_t tas58xx_volume_min_;  // YAML configured maximum volume dB

   int i2c_error_{0}; // last i2c error

   size_t number_registers_configured_{0}; // number tas58xx registers configured during 'setup'

};

}  // namespace esphome::tas58xx
