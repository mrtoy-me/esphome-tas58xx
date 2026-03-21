# YAML configuration

## TAS5805 Audio Dac - Speaker 1.1 Configuration

This feature could be ypically used with Sonocotta Louder-ESP32 or Louder Esparagus. Note that this feature is only available for TAS5805M audio boards. The additional YAML audio_dac: YAML configuration available is described as follows:

Under audio_dac: configuration, speaker_config: is available for hardware setup as speaker 1.1

An example audio_dac configuration:
```
audio_dac:
  - platform: tas58xx
    id: tas5805_dac
    tas85xx_dac: TAS5805M
    enable_pin: GPIOxx
    analog_gain: -9db
    volume_max: 0dB
    volume_min: -60db
    speaker_config:
      crossover_frequency: 1000Hz   # required 1Hz - 25000Hz
      mono_mixer_mode: STEREO_SUB   # default so can be omitted if this is desired setting
      crossbar_left_amp: FROM_LEFT  # default so can be omitted if this is desired setting
      crossbar_right_amp: FROM_SUB  # default so can be omitted if this is desired setting
      crossbar_left_i2s: FROM_RIGHT # default so can be omitted if this is desired setting
      crossbar_right_i2s: FROM_SUB  # default so can be omitted if this is desired setting
    update_interval: 1s
```
**speaker_config:** configuration variables:
- **crossover_frequency:** (*Required*): valid values from 1Hz to 25000Hz. The crossover frequency is used to calculate extra low pass filters for a connected woofer.

- **mono_mixer_mode:** (*Optional*): default STEREO_SUB with valid values LEFT_SUB, RIGHT_SUB, STEREO_SUB,    LEFT_EQ_SUB, RIGHT_EQ_SUB. The mono mixer configures subchannel mixing of the digital audio data going to the connected woofer. The "_SUB" at the end of each mode stands for "sub channel" and is used to differentiate mono mixer modes from input mixer modes.

- **crossbar_left_amp:** (*Optional*): default FROM_LEFT with valid values FROM_LEFT, FROM_RIGHT and FROM_SUB

- **crossbar_right_amp:** (*Optional*): default FROM_SUB with valid values FROM_LEFT, FROM_RIGHT and FROM_SUB

- **crossbar_left_i2s:** (*Optional*): default FROM_RIGHT with valid values FROM_LEFT, FROM_RIGHT and FROM_SUB

- **crossbar_right_i2s:** (*Optional*): default FROM_SUB with valid values FROM_LEFT, FROM_RIGHT and FROM_SUB

  The above crossbar configuration provides an easy way to configure what finally appears on amplifier outputs and I2S(digital) SDOUT. FROM_LEFT is an abbreviation for "from left channel", FROM_RIGHT is an abbreviation for "from right channel" and FROM_SUB is an abbreviation for "from sub channel" that originates from the mono mixer (sometimes called "sub channel mixer").

For more technical information on these settings see "Process Flow 3" section of the Texas Instruments Application Report SLOA263A - TAS5805M, TAS5806M and TAS5806MD Process Flows.
