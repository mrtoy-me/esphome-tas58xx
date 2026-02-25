# ESPHome tas58xx component for audio boards by Sonocotta with TAS5805M or TAS5825M Audio DAC
This ESPHome external component is based on work by Andriy Malyshenko at
https://github.com/sonocotta which is licenced under GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007.
Information from his repositories has also been used/reproduced in this read.me to provide
a better understanding of how to use this component to generate firmware using Esphome Builder.

# Usage: tas58xx component on Github
This component requires Esphome version 2026.2.0 or later.

The following yaml can be used so ESPHome accesses the component files:
```
external_components:
  - source: github://mrtoy-me/esphome-tas58xxm@beta
    components: [ tas58xx ]
    refresh: 0s
```

# Overview
This component uses the Esphome ESP_IDF framwork and must be configured accordingly.
While this component works with board having ESP32, the ESP32-S3
has larger PSRAM and hence has better performance than the ESP32.

The component allows many features of the TAS5805m or TAS5825M DAC to be configured
prior to firmware generation or configured/controlled through Homeassistant.
Fault sensors can also be configured, so their status is visible in Homeassistant.

The component YAML configuration uses the esphome Audio DAC Core component,
and is configured under **audio_dac:** as **- platform: tas58xx**

These DACs are controlled by I2C so the component requires configuration of
**i2c:** with **sda:** and **scl:** pins.

Appropriate configuration of psram, i2s_audio, speaker and mediaplayer are required.
Typical YAML configurations are provided by sonocotta in
[Esparagus Media Center repository](https://github.com/sonocotta/esparagus-media-center/tree/main/firmware/esphome)

## Component Features
The component communicates with DAC by I2C and provides the following features:
- initialise  DAC
- enable/disable DAC
- adjust  maximum and minimum volume level
- set DAC mode
- set Mixer mode
- set EQ Control state and EQ gains
- get fault states
- automatically clear fault states
- set Analog Gain
- set Volume
- set Mute state

YAML configuration includes:
- 1 optional Switch configuration - Enable DAC
- 30 optional EQ Gain Numbers to control EQ gains
- 2 optional Channel Volume Numbers to individually control the Channel Volumes
- 2 optional Select for setting Mixer Mode and EQ Mode
- 2 optional Select for each channel to set EQ High-pass and Low-pass filter presets
- 12 optional Binary Sensors corresonding to DAC fault codes (all optional)
- an optional Sensor providing the number of times a DAC fault was detected and DAC fault cleared

# TAS5805M and TAS5825M Features
## Analog Gain and Digital Volume
The analog gain setting ensures that the output signal is not clipped at different supply voltage levels.
With analog gain set at the appropriate level, the digital volume
is used to set the audio volume. Keep in mind, it is perfectly safe to set the
analog gain at a lower level.
Note: that the component allows defining analog gain in YAML and cannot be altered at runtime.

## DAC Mode
These DACs have a bridge mode of operation, that causes both output drivers to synchronize
and push out the same audio with double the power. Typical setup for each of the
Dac Modes is shown in the following table.
Note: the component allows defining Dac Mode in YAML and cannot be altered at runtime.

|   | BTL (default, STEREO) | PBTL (MONO, rougly double power) |
|---|-----------------------|---------------------------|
| Descriotion | Bridge Tied Load, Stereo | Parallel Bridge Tied Load, Mono |
| Rated Power | 2×23W (8-Ω, 21 V, THD+N=1%) | 45W (4-Ω, 21 V, THD+N=1%) |
| Schematics | ![image](https://github.com/sonocotta/esp32-audio-dock/assets/5459747/e7ada8c0-c906-4c08-ae99-be9dfe907574) | ![image](https://github.com/sonocotta/esp32-audio-dock/assets/5459747/55f5315a-03eb-47c8-9aea-51e3eb3757fe)
| Speaker Connection | ![image](https://github.com/user-attachments/assets/8e5e9c38-2696-419b-9c5b-d278c655b0db) | ![image](https://github.com/user-attachments/assets/8aba6273-84c4-45a8-9808-93317d794a44)


## Mixer Mode
Mixer mode allows mixing of channel signals and route them to the appropriate audio
channel. The typical setup for the mixer is to send Left channel audio to the Left driver,
and Right channel to the Right. A common alternative is to combine both channels into
true Mono (you need to reduce both to -3dB to compensate for signal doubling).
In BTL Dac Mode, the mixer mode can be set to STEREO, INVERSE_STEREO, MONO, LEFT or RIGHT while
in PBTL Dac Mode, the mixer mode can be set to MONO, LEFT or RIGHT.


## EQ Band Gains
These DACs have a powerful 15-channel EQ that allows defining each channel's transfer function
using BQ coefficients. For practical purposes, the audio range is split into 15 bands,
defining for each a -15 to +15 dB gain adjustment range and appropriate bandwidth to
cause mild overlap. This keeps the curve flat enough to not cause distortions
even in extreme settings, but also allows a wide range of transfer characteristics.

| Band | Center Frequency (Hz) | Frequency Range (Hz) | Q-Factor (Approx.) |
|------|-----------------------|----------------------|--------------------|
| 1    | 20                    | 10–30                | 2                  |
| 2    | 31.5                  | 20–45                | 2                  |
| 3    | 50                    | 35–70                | 1.5                |
| 4    | 80                    | 55–110               | 1.5                |
| 5    | 125                   | 85–175               | 1                  |
| 6    | 200                   | 140–280              | 1                  |
| 7    | 315                   | 220–440              | 0.9                |
| 8    | 500                   | 350–700              | 0.9                |
| 9    | 800                   | 560–1120             | 0.8                |
| 10   | 1250                  | 875–1750             | 0.8                |
| 11   | 2000                  | 1400–2800            | 0.7                |
| 12   | 3150                  | 2200–4400            | 0.7                |
| 13   | 5000                  | 3500–7000            | 0.6                |
| 14   | 8000                  | 5600–11200           | 0.6                |


## EQ Presets - High-pass and Low-pass filter

These presets allow quick setup of Subwoofer in Bi-amp configuration. These presets cover 4-order High-pass and Low-pass filters, using 2 EQ bands, 2nd order Chebyshev filter type each.
For HF profiles additional gain compensation was applied (3rd EQ band) to flatten the response.
Available profiles:

| Number | Name      |
|--------|-----------|
| 0      | FLAT      |
| 1      | LF_60HZ   |
| 2      | LF_70HZ   |
| 3      | LF_80HZ   |
| 4      | LF_90HZ   |
| 5      | LF_100HZ  |
| 6      | LF_110HZ  |
| 7      | LF_120HZ  |
| 8      | LF_130HZ  |
| 9      | LF_140HZ  |
| 10     | LF_150HZ  |
| 11     | HF_60HZ   |
| 12     | HF_70HZ   |
| 13     | HF_80HZ   |
| 14     | HF_90HZ   |
| 15     | HF_100HZ  |
| 16     | HF_110HZ  |
| 17     | HF_120HZ  |
| 18     | HF_130HZ  |
| 19     | HF_140HZ  |
| 20     | HF_150HZ  |


## Fault States
A fault detection system that allows these DACs to self-diagnose issues with
power, data signal, short circuits, overheating etc. The general pattern for
fault detection is periodic check of fault registers, and when there are any
faults, provide notification through sensor/s and clear any fault afterwards.

# Activation of Mixer mode and EQ Gains or EQ Presets
For software configuration of the Mixer and EQ Gains, the TAS5805M and TAS5825M
must have received a stable I2S signal. If EQ Band Gain Numbers or Eq Presets
are configured, what this means for this component is that before the component
writes these settings, the DAC must have received some audio.

## Typical Use Case - Speaker Mediaplayer
The typical way of handling this requirement is where speaker mediaplayer component
is configured to play audio during boot. In this case, a short sound file is
configured under **mediaplayer:** and configuration added under **esphome:**
to play that short sound at the correct point in the boot process.

Two alternative flac sound files are provided which have a duration of about 0.5 second.
A substition at the start of the YAML as show below can be used to reference by
simply commenting out the sound file not required.
You can use your own boot sound by creating a flac file of about 0.5 second duration and
reference it appropriately in the YAML substitution.
```
substitutions:
  sync_dac_i2s_sound: "https://github.com/mrtoy-me/esphome-tas5805m/raw/main/components/tas58xx/tas58xx_boot.flac"

  #use instead if you don't want an audible boot sound
  #sync_dac_i2s_sound: "https://github.com/mrtoy-me/esphome-tas5805m/raw/main/components/tas58xx/silent_boot.flac"
```

The YAML configuration required under **mediaplayer:** to reference this file is:
```
files:
      id: startup_sync_sound
      file: file: ${sync_dac_i2s_sound}
```

YAML Configuration required to be included under **esphome:** YAML:
```
on_boot:
    priority: 220.0
    then:
      media_player.play_media:
        id: external_media_player # speaker media player id
        media_url: file://startup_sync_sound
```
The **audio_dac:** has an optional configuration variable called **refresh_eq:**
The default configuration of **refresh_eq: AUTO** matches the above use case and
therefore can be omitted from the **audio_dac:** YAML configuration.

## Use Case where Speaker Mediaplayer is not used (eg using a SnapCast client component)
Another use case, is use of Snapcast client component instead of Speaker Mediaplayer component
to produce the required audio. In this use case, the following "workaround" is necessary
to play audio before the component writes the Mixer Mode and EQ Gain settings to the DAC.
This workaround requires the user to start playing audio
then using the EQ Mode Select to move from Off to choose the relevant EQ Mode.

The following changed configuration is required:

1) Configure **audio_dac:** with optional configuration variable and value **refresh_eq: MANUAL**
2) Configure **select: - platform: tas58xx** with **eq_mode:** as follows:

```
select:
  - platform: tas58xx
    eq_mode:
      name: EQ Mode```

3) After Louder has booted, manually initiate playing of some audio
4) Turn EQ Mode select from Off to relevant Eq Mode


# YAML configuration

## Audio Dac

Example configuration:
```
audio_dac:
  - platform: tas58xx
    id: tas5825_dac
    tas85xx_dac: TAS5825M # for Tas5805m DAC use tas85xx_dac: TAS5805M
    enable_pin: GPIOxx
    analog_gain: -9db
    dac_mode: BTL
    mixer_mode: STEREO # default can be omitted
    volume_max: 0dB
    volume_min: -60db
    ignore_fault: CLOCK_FAULT # default can be omitted
    refresh_eq: AUTO # default can be omitted
    update_interval: 1s
```
Configuration variables:
- **tas85xx_dac:** (*Required*): valid values TAS5805M or TAS5825M. Defaults to TAS5805M

- **enable_pin:** (*Required*): GPIOxx, enable pin

- **analog_gain:** (*Optional*): dB values from -15.5dB to 0dB in 0.5dB increments.
  Defaults to -15.5dbB. A setting of -15.5db is typical when 5v is used to power the Louder.

- **dac_mode:** (*Optional*): valid values BTL or PBTL. Defaults to BTL.

- **mixer_mode:** (*Optional*): values STEREO, INVERSE_STEREO, MONO, LEFT or RIGHT
  Defaults to STEREO. Note: for PBTL Dac Mode, only MONO, LEFT or RIGHT are valid.

- **volume_max:** (*Optional*): whole dB values from -103dB to 24dB. Defaults to 24dB.

- **volume_min:** (*Optional*): whole dB values from -103dB to 24dB. Defaults to -103dB.

- **ignore_fault:** (*Optional*): Valid options are **CLOCK_FAULT** and **NONE**. Default is **CLOCK_FAULT**.
  That is, by default clock faults are ignored when determining if fault registers require clearing. To trigger clearing of fault registers on any fault condition, specify **ignore_fault: NONE**

- **refresh_eq:** (*Optional*): valid values **AUTO** or **MANUAL**. Default is **AUTO**.
  This setting is not required if you are using Speaker Mediaplayer component as the default matches this use case. The setting is mainly intended when the Snapcast client component is used instead of Speaker Mediaplayer. When a Snapcast client component is configured, the BY_SWITCH setting should be used. See information
  under "Activation of Mixer mode and EQ Gains" section above.

- **update_interval:** (*Optional*): defines the interval (seconds) at which faults will be
  checked and then if detected, the clearing of the fault registers will occur at next interval. Defaults to 1s. **Note:** update interval cannot be reduced below 1s.


## Selects for Mixer Mode and EQ Mode
Several selects can be configured to provide select dropdowns in Homeassistant.
- EQ Mixer Mode allow the input mixer mode to be changed between STEREO, INVERSE_STEREO, MONO, LEFT or RIGHT (BTL)
- Eq Mode Select allows changing from EQ Mode Off to the YAML configured EQ Mode.

```select:
  - platform: tas58xx
    mixer_mode:
      name: _Mixer Mode
    eq_mode:
      name: EQ Mode
```
The desired Mixer Mode can also be defined under audio_dac: but if Select mixer_mode is defined
then the last selected Mixer Mode is saved and will be reloaded at next re-boot. If a Select Mixer Mode has been saved,
this will supercede and be used instead of the Mixer Mode defined under audio_dac:

For any configuration with EQ Gains or EQ Presets configured, there only two possible EQ Modes
being Off and depending on configuration EQ 15 Band or EQ BIAMP 15 Band or EQ Presets.

The EQ Mode select option (in addition to Off) is determined based on the YAML configuration as follows:
- 15 x Left EQ Gains configured -> **EQ 15 Band**
- 15 x Left EQ Gains and 15 x Right EQ Gains configured -> **EQ BIAMP 15 Band**
- EQ Preset Left Channel and EQ Preset Right Channel frequency cutoffs configured -> **EQ Presets**

If the audio_dac: refresh_eq: option is MANUAL then on start the EQ Mode Select is initially selected Off.
Whereas, if audio_dac: option refresh_eq: AUTO then on start the EQ Mode Select is initially selected the relevant Eq Mode.


# EQ Control configuration

## Left/Right Channel Volume and EQ Band Gain Numbers

Left and Right Channel Volume control can be configured.
If configured, both channels must be configured as follows:

```
number:
- platform: tas58xx
    channel_volume_left:
      name: _Volume Left
    channel_volume_right:
      name: _Volume Right
```

For each channel 15 EQ Band Gain Numbers can be configured for controlling the gain of each EQ Band
in Home Assistant. The number configuration heading for each number is shown below
with an example name. Defining **number: -platform: tas58xx** requires
all 15 EQ Gain Band headings for each Channel to be configured.

If only 15 left EQ Gains are configured then they are applied to both the left and right channels.
To control left and right channels individually, the 15 left EQ Gains and 15 Right EQ Gains need
be configured. Configuration of 15 Right EQ Gains requires the 15 left EQ Gains to be configured.
For EQ Band Gains to activate correctly requires some addition YAML configuration, refer to the
"Activation of Mixer mode and EQ Gains" section above.

Example configuration of tas58xx platform (Band Gain) Numbers for each channel:
```
number:
  - platform: tas58xx
    left_eq_gain_20Hz:
      name: Left Gain ---20Hz
    left_eq_gain_31.5Hz:
      name: Left Gain ---31.5Hz
    left_eq_gain_50Hz:
      name: Left Gain ---50Hz
    left_eq_gain_80Hz:
      name: Left Gain ---80Hz
    left_eq_gain_125Hz:
      name: Left Gain --125Hz
    left_eq_gain_200Hz:
      name: Left Gain --200Hz
    left_eq_gain_315Hz:
      name: Left Gain --315Hz
    left_eq_gain_500Hz:
      name: Left Gain --500Hz
    left_eq_gain_800Hz:
      name: Left Gain --800Hz
    left_eq_gain_1250Hz:
      name: Left Gain -1250Hz
    left_eq_gain_2000Hz:
      name: Left Gain -2000Hz
    left_eq_gain_3150Hz:
      name: Left Gain -3150Hz
    left_eq_gain_5000Hz:
      name: Left Gain -5000Hz
    left_eq_gain_8000Hz:
      name: Left Gain -8000Hz
    left_eq_gain_16000Hz:
      name: Left Gain 16000Hz

    right_eq_gain_31.5Hz:
      name: Right ---31.5Hz
    right_eq_gain_50Hz:
      name: Right ---50Hz
    right_eq_gain_80Hz:
      name: Right ---80Hz
    right_eq_gain_125Hz:
      name: Right --125Hz
    right_eq_gain_200Hz:
      name: Right --200Hz
    right_eq_gain_315Hz:
      name: Right --315Hz
    right_eq_gain_500Hz:
      name: Right --500Hz
    right_eq_gain_800Hz:
      name: Right --800Hz
    right_eq_gain_1250Hz:
      name: Right -1250Hz
    right_eq_gain_2000Hz:
      name: Right -2000Hz
    right_eq_gain_3150Hz:
      name: Right -3150Hz
    right_eq_gain_5000Hz:
      name: Right -5000Hz
    right_eq_gain_8000Hz:
      name: Right -8000Hz
    right_eq_gain_16000Hz:
      name: Right 16000Hz
```

All Numbers configured in Homeassistant are saved every minute and restored at next reboot.

## EQ Presets
Provides 21 possible Frequency Cutoff selections for left and right channels of
Flat, Low Frequency 50Hz to 150Hz and High Frequency 50Hz to 150Hz

An example configuration of EQ Presets adds to the typical select configuration as follows:
```
select:
  - platform: tas58xx
    mixer_mode:
      name: _Mixer Mode
    eq_mode:
      name: EQ Mode
    eq_preset_left_channel:
      name: EQ Preset Left Cutoff
    eq_preset_right_channel:
      name: EQ Preset Right Cutoff
```
Note: Either EQ Presets OR EQ Gains can be configured not both.


## DAC Enable Switch
A switch can be configured to provide and Enable-Disable DAC switch in Homeassistant.
- Enable Dac Switch, more specifically places the DAC into Play mode or
  into low power Sleep mode

An **interval:** and **mediaplayer:** YAML configuration can be used to
trigger Enable Louder Switch Off when there is no music player activity (idle or paused)
for the defined time and when music player activity is detected (by mediaplayer),
the Enable Louder Switch is triggered On. The example interval configuration also
requires configuration of **mediaplayer:**

Configuration of tas58xx platform Switches in typical use case:

```switch:
  - platform: tas58xx
    enable_dac:
      name: Enable Louder
      id: enable_louder
      restore_mode: ALWAYS_ON
```
Configuration headers:
- **enable_dac:** (*Optional*): allows the definition of a switch to enable/disable
  the DAC. Switch On (enabled) places the DAC into Play mode while
  Switch Off (disabled) places the DAC into low power Sleep mode.

  Configuration variables:
    - **restore_mode:** (optional but recommended): **ALWAYS_ON** is recommended.


## Binary Sensors
Binary sensors can be configured which correspond to DAC fault codes.
The tas58xx binary sensor platform has configuration headings for each binary sensor
as shown below with an example name configured.
All 12 binary sensors can be optionally defined but it is recommended that at minimum,
one binary sensor **have_fault:** is configured.

**have_fault:** Configuration variable:
  - The **have_fault:** binary sensor turns ON if any DAC faults conditions are ON, however note that by default clock faults are excluded.

    Configuration variables:
    - **exclude:** (optional): Allows excluding defined faults from have_fault binary sensor.
      Valid options are **NONE** and **CLOCK_FAULT**. Default is **CLOCK_FAULT** which excludes clock faults from **have_fault** binary sensor. To include all faults, specify **exclude: NONE**.
      Excluding clock faults by default is implemented since a clock fault is essentially a warning about unexpected behavior of the I2S clock and Esphome idf mediaplayers generate clock faults because I2S is manipulated to guarentee timing.

**over_temp_warning:**
  - To attempt to mitigate an over temperature upon receiving a over temperature, the volume can be decreased using **interval:** configuration.
    For this YAML to take effect, the **mediaplayer:**  configuration must include configuration of the **volume_increment:**.
    Typically 5-10% should be suitable but depends on the dB range defined by the **volume_max:** and **volume_min:** under **audio_dac:**. The % equivalent to around 6dB decrease
    should have a benficial effect, but also depends on the update interval for checking faults.
    **Note:** all binary sensors are updated at the **update interval:** defined under **audio_dac:**

Example configuration of tas58xx platform Binary Sensors:
```
binary_sensor:
  - platform: tas58xx
    have_fault:
      name: Any Faults
      exclude: CLOCK_FAULT
    left_channel_dc_fault:
      name: Left Channel DC Fault
    right_channel_dc_fault:
      name: Right Channel DC Fault
    left_channel_over_current:
      name: Left Channel Over Current
    right_channel_over_current:
      name: Right Channel Over Current
    otp_crc_check:
      name: CRC Check Fault
    bq_write_failed:
      name: BQ Write Failure
    clock fault:
      name: I2S Clock Fault
    pcdd_over_voltage:
      name: PCDD Over Voltage
    pcdd_under_voltage:
      name: PCDD Under Voltage
    over_temp_shutdown:
      name: Over Temperature Shutdown Fault
    over_temp_warning:
      name: Over Temperature Warning
      id: over_temperature_warning
```

## Sensor
One optional tas58xx platform sensor can be defined configuration heading **faults_cleared:** can be optionally configured.
This sensor counts the number of times a fault was detected and subsequently cleared by the component.
```
sensor:
  - platform: tas58xx
    faults_cleared:
      name: "Times Faults Cleared"
```
Configuration variables:
- **update interval:** (*Optional*): The interval at which the sensor is updated. Defaults to 60s.


## Announce Volume Template Number
An Announce Volume template number which can be used in
conjuction with the **mediaplayer:** YAML configurations for adjusting the
announcement pipeline audio volume separately to the media pipeline volume.
This is useful for a Text-to-Speech announcements that may have a different
volume level to the audio playing through the media pipeline.


# YAML examples in this Repository
Extensive Esphome YAML examples are provide on github by sonocotta in
[Esparagus Media Center repository](https://github.com/sonocotta/esparagus-media-center/tree/main/firmware/esphome)

