# ESPHome tas58xx component for audio boards by Sonocotta with TAS5805M or TAS5825M Audio DAC
This ESPHome external component is based on work by Andriy Malyshenko at
https://github.com/sonocotta which is licenced under GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007.
Information from his repositories has also been used/reproduced in this read.me to provide
a better understanding of how to use this component to generate firmware using Esphome Builder.

# Usage: tas58xx component on Github
This component requires Esphome version 2026.10.0 or later.

The following yaml can be used so ESPHome accesses the component files:
```
external_components:
  - source: github://mrtoy-me/esphome-tas58xx@dev_update_eq_proto
    components: [ tas58xx ]
    refresh: 0s
```

**Breaking change on this branch:** the EQ gain number keys `left_eq_gain_<frequency>Hz` and
`right_eq_gain_<frequency>Hz` are replaced by the `left_bands:` and `right_bands:` lists described in
"EQ Bands and Fixed Filters" below, so EQ YAML from `main` must be converted.

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
- set EQ Mode, EQ bands (filter type, frequency, Q factor and an adjustable gain) and fixed notch / band pass filters
- get fault states
- automatically clear fault states
- set Analog Gain
- set Volume
- set Mute state

YAML configuration includes:
- 1 optional Switch configuration - Enable DAC
- up to 15 EQ filters per channel: optional EQ band gain Numbers (`left_bands:` / `right_bands:`) and optional
  fixed filters (`left_fixed_filters:` / `right_fixed_filters:` under `audio_dac:`)
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


## Modulation Mode
Modulation mode affects DAC efficiency. While running under lower VCC (12V or less), it is not justified to change from the default BD Mode. On higher VCC, lower efficiency affects DAC stability, the DAC cannot sustain full power without some kind of active cooling (heatsink, fan or both), so in this case, it would be better to choose a more power efficient modulation mode.

BD Mode is default with each output switching from 0 volts to the supply voltage. The 1SPW mode alters the normal modulation scheme in order to achieve higher efficiency with a slight penalty in THD degradation. Hybrid Modulation is designed to minimize power loss without compromising the THD+N performance, however this mode is not currently implemented in this component.


## Mixer Mode
Mixer mode allows mixing of channel signals and route them to the appropriate audio
channel. The typical setup for the mixer is to send Left channel audio to the Left driver,
and Right channel to the Right. A common alternative is to combine both channels into
true Mono (you need to reduce both to -3dB to compensate for signal doubling).
In BTL Dac Mode, the mixer mode can be set to STEREO, INVERSE_STEREO, MONO, LEFT or RIGHT while
in PBTL Dac Mode, the mixer mode can be set to MONO, LEFT or RIGHT.


## EQ Bands and Fixed Filters
These DACs have 15 biquad filters (BQs) per channel, run one after the other. The component calculates the
biquad coefficients on the ESP32 from the filter type, frequency, Q factor and gain set in YAML. The
coefficients match TI PurePath Console to within 1 LSB of the DSP's 5.27 fixed point format, which is inaudible.

There are two kinds of EQ filter:
- **EQ bands** (`left_bands:` / `right_bands:` under the tas58xx **number:** platform): each band has a gain
  Number (slider) in Home Assistant. Any filter with a gain input is an EQ band.
- **Fixed filters** (`left_fixed_filters:` / `right_fixed_filters:` under **audio_dac:**): notch and band pass
  filters, which have no gain input, so no slider. They are set in YAML only.

| eq_filter | Kind | Uses gain | Uses frequency | Uses q_factor | Default q_factor |
|---|---|---|---|---|---|
| `equalizer` (default) | EQ band | boost / cut around the frequency | yes | yes | 2.144908 |
| `peaking` | EQ band | boost / cut around the frequency | yes | yes | 2.144908 |
| `low_shelf` | EQ band | boost / cut below the frequency | yes | yes | 0.707 |
| `high_shelf` | EQ band | boost / cut above the frequency | yes | yes | 0.707 |
| `low_pass` | EQ band | level of the frequencies passed | yes | no, fixed at 0.707 (2nd order Butterworth) | - |
| `high_pass` | EQ band | level of the frequencies passed | yes | no, fixed at 0.707 (2nd order Butterworth) | - |
| `notch` | fixed filter | no | yes | yes, bandwidth = frequency / q_factor | required |
| `band_pass` | fixed filter | no | yes | yes, bandwidth = frequency / q_factor | required |

- `equalizer` is the TI PurePath Console "Equalizer (Q Factor)" filter; `peaking` is its "Peaking EQ" filter
  (the Audio EQ Cookbook form). Both are bell shaped, the curves differ slightly at the same Q.
- On `low_pass` and `high_pass` the gain sets the level of the frequencies the filter passes, as TI PurePath
  Console does. At 0 dB they are pure filters.
- All pass is not offered: it is used internally for flat (unused) biquads.

### Default Q factor and a standard 15 band graphic EQ
The default `q_factor` of `equalizer` and `peaking` is 2.144908, the constant Q of a 2/3-octave graphic
equalizer, Q = √(2ᴺ) / (2ᴺ − 1) with N = 2/3. Bands at the ISO 2/3-octave centre frequencies
(ISO 266:1997 Acoustics, Preferred frequencies) with this Q make a standard constant-Q 15 band graphic EQ, as used by
commercial 15 band equalizers (eg the dbx 3215, "15 ISO standard center, 2/3 octave constant Q frequency bands"):

25, 40, 63, 100, 160, 250, 400, 630, 1000, 1600, 2500, 4000, 6300, 10000, 16000 Hz

For such a graphic EQ only `name:` and `frequency:` are needed for each band. The shelves default to 0.707, a shelf
with no overshoot.

### Limits
| | EQ bands | Fixed filters (`notch`, `band_pass`) |
|---|---|---|
| `frequency` | required, 20 Hz to 16000 Hz, whole Hz (eg `80Hz`, `1.25kHz`); a fraction is rounded to the nearest Hz with a warning, eg `31.5Hz` is built as 32 Hz | required, 20 Hz to 16000 Hz, whole Hz, rounded the same way |
| `q_factor` | 0.1 to 20, default by filter (table above); a warning if given for `low_pass` / `high_pass` | required, 0.6 to 30 |
| gain | -24 dB to +24 dB; `high_shelf` and `high_pass` -24 dB to +18 dB | none |
| `gain_min` / `gain_max` | optional slider limits, whole dB; platform level -24 dB to +24 dB, band level within the filter's gain range above (see "Limiting the gain range") | none |
| count | EQ bands + fixed filters at most 15 per channel, at most 12 with an EQ Preset on that channel (more is an error) | |

- `high_shelf` and `high_pass` are limited to +18 dB because above that one of their coefficients
  (about -2 × the linear gain) overflows the DSP's 5.27 coefficient range (just under +16).
- Fixed filter Q is at least 0.6 because close to Q 0.5 the band pass lower edge (frequency − bandwidth / 2)
  reaches 0 Hz and the filter is no longer stable in 5.27 fixed point.
- Copying a notch or band pass from TI PurePath Console: PPC sets these filters by bandwidth (its QVal is not used
  for them), so use `q_factor` = frequency / bandwidth, eg 50 Hz with a 23 Hz bandwidth is `q_factor: 2.1739`.
  A rounded Q gives a slightly different bandwidth.

### Biquad order
Biquads are allocated by the component, so no band or biquad numbers are needed in YAML. Filters in series give the
same overall response in any order, only the signal level between biquads changes, so the component places the
cuts first:
1. `high_pass`
2. `low_pass`, `notch`, `band_pass`
3. `low_shelf`, `high_shelf`, `equalizer`, `peaking`

Within each group the YAML order is kept. EQ bands and fixed filters of a channel are packed from biquad 1
(after the 3 biquads of an EQ Preset, if the channel has one). The biquad given to each filter is shown in the log
at startup (dump config). Only the filters in YAML are listed and written; biquads not listed keep their power
on values, typically termed "flat" (all pass).

### Sample rate
The biquads are calculated for the 96 kHz rate the DSP processes at. With the TAS5805M the 96 kHz process flow
converts the I2S input to 96 kHz; with the TAS5825M register 0x46 (DSP_CTRL) sets 96 kHz processing. A 48 kHz I2S
`sample_rate:` (as in the example YAMLs) is recommended. With 44.1 kHz family input the TAS5805M may process at
88.2 kHz, which would move every filter about 8% lower in frequency.


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

# Activation of Mixer mode and EQ settings
The TAS5805M and TAS5825M only accept the Mixer Mode, Left/Right Channel Volumes,
EQ Mode, EQ bands, fixed filters and EQ Presets settings once they are receiving an I2S signal.
Until then, these settings are saved and the component writes them to the DAC
the first time audio is played. No boot sound or additional YAML is required.

## Speaker Mediaplayer
When the I2S speaker is configured with **audio_dac:** referencing the tas58xx audio dac,
the speaker notifies the component when audio starts and the settings are written straight away:
```
speaker:
  - platform: i2s_audio
    audio_dac: tas58xx_dac   # id of the tas58xx audio_dac
```

## Use Case where Speaker Mediaplayer is not used (eg using a SnapCast client component)
Snapcast client components drive I2S directly and do not notify the component when audio starts.
In this case, the component checks at each **update_interval:** whether the DAC is playing,
and writes the settings once it is. This also applies to a speaker without **audio_dac:** configured.
No additional configuration is required.

## Changing settings when audio is not playing
Settings changed in Homeassistant while the DAC is playing are written straight away.
Settings changed while no audio is playing are saved and written when audio next starts.


# YAML configuration

## Audio Dac

Example configuration:
```
audio_dac:
  - platform: tas58xx
    id: tas5825_dac
    tas58xx_dac: TAS5825M # for Tas5805m DAC use tas58xx_dac: TAS5805M
    enable_pin: GPIOxx
    analog_gain: -9db
    modulation: BD_MODE # default can be omitted; for 1SPW Mode use modulation: 1SPW_MODE
    dac_mode: BTL
    mixer_mode: STEREO # default can be omitted
    volume_max: 0dB
    volume_min: -60db
    ignore_fault: CLOCK_FAULT # default can be omitted
    update_interval: 1s
    right_fixed_filters:       # optional, notch and band_pass only (left_fixed_filters for the left channel)
      - eq_filter: notch
        frequency: 3150Hz
        q_factor: 10           # bandwidth 315 Hz
```
Configuration variables:
- **tas58xx_dac:** (*Optional*): valid values TAS5805M or TAS5825M. Defaults to TAS5805M.
  **model:** is also accepted, as used by ESPHome's own tas58xx component.

- **enable_pin:** (*Required*): GPIOxx, enable pin

- **analog_gain:** (*Optional*): dB values from -15.5dB to 0dB in 0.5dB increments.
  Defaults to -15.5dbB. A setting of -15.5db is typical when 5v power supply.

- **modulation:** (*Optional*): valid values BD_MODE or 1SPW_MODE. Defaults to BD_MODE.

- **dac_mode:** (*Optional*): valid values BTL or PBTL. Defaults to BTL.

- **mixer_mode:** (*Optional*): values STEREO, INVERSE_STEREO, MONO, LEFT or RIGHT
  Defaults to STEREO. Note: for PBTL Dac Mode, only MONO, LEFT or RIGHT are valid.

- **volume_max:** (*Optional*): whole dB values from -103dB to 24dB. Defaults to 24dB.
  **volume_max_db:** is also accepted, as used by ESPHome's own tas58xx component.

- **volume_min:** (*Optional*): whole dB values from -103dB to 24dB. Defaults to -103dB.
  **volume_min_db:** is also accepted, as used by ESPHome's own tas58xx component.

- **volume_follows:** (*Optional*): the id of another tas58xx **audio_dac** (the leader) whose volume and mute this
  DAC (a follower) copies.
  For a second DAC on the same I2S output: a speaker only drives the one **audio_dac** it names, so without this
  the second DAC stays at its **volume_min**. The volume is copied as a position from 0% to 100%, so each DAC maps
  it onto its own **volume_min** to **volume_max**. The leader also passes on audio start, so the second DAC's
  DSP settings (mixer, EQ, channel volumes) are written straight away. A DAC cannot follow itself, cannot follow a
  DAC that itself follows another, and cannot also be the **audio_dac** of a speaker.

  ```yaml
  audio_dac:
    - platform: tas58xx
      id: dac1            # the DAC named by the speaker's audio_dac
      enable_pin: GPIO33
    - platform: tas58xx
      id: dac2
      address: 0x2C
      enable_pin: GPIO32
      volume_follows: dac1
  ```
  This replaces media player **on_volume** / **on_mute** / **on_unmute** lambdas copying the volume to the
  second DAC.

- **ignore_fault:** (*Optional*): Valid options are **CLOCK_FAULT** and **NONE**. Default is **CLOCK_FAULT**.
  That is, by default clock faults are ignored when determining if fault registers require clearing. To trigger clearing of fault registers on any fault condition, specify **ignore_fault: NONE**

- **refresh_eq:** (*Optional*): no longer used and can be removed. It is still accepted, so existing
  configurations remain valid. See "Activation of Mixer mode and EQ settings" section above.

- **left_fixed_filters:** / **right_fixed_filters:** (*Optional*): a list of fixed filters for each channel, with no
  gain and no Number in Home Assistant. Each entry:
  - **eq_filter:** (*Required*): `notch` or `band_pass`
  - **frequency:** (*Required*): 20 Hz to 16000 Hz, whole Hz; a fraction is rounded to the nearest Hz with a
    warning
  - **q_factor:** (*Required*): 0.6 to 30, bandwidth = frequency / q_factor

  A channel with fixed filters counts as having EQ, so Select **eq_mode:** is required and EQ Mode changes apply to
  the fixed filters too (see "Selects for Mixer Mode and EQ Mode"). With dac_mode PBTL right_fixed_filters are not
  used (a warning is given). See "EQ Bands and Fixed Filters" above for limits and biquad order.

- **update_interval:** (*Optional*): defines the interval (seconds) at which faults will be
  checked and then if detected, the clearing of the fault registers will occur at next interval. Defaults to 1s. **Note:** update interval cannot be reduced below 1s.


## Selects for Mixer Mode and EQ Mode
Several selects can be optionally configured to provide select dropdowns in Homeassistant.
- EQ Mixer Mode allows the input mixer mode to be changed between STEREO, INVERSE_STEREO, MONO, LEFT or RIGHT for BTL dac mode
or for dac mode PBTL between MONO, LEFT or RIGHT
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

If any EQ bands, fixed filters or EQ Presets are configured then Eq Mode Select must be defined.

The EQ Mode select options (in addition to EQ Off) are determined by the YAML configuration as follows:

| Config | Select options |
|---|---|
| Left EQ bands and/or left fixed filters only | EQ On Right=Left |
| Lone left EQ Preset | EQ Left On |
| Right only (bands, fixed filters or preset) | EQ Right On |
| Left and Right both configured (any combination of bands, fixed filters or presets) | EQ On Right=Left, EQ Left On, EQ Right On, EQ BIAMP On |

- **EQ On Right=Left**: the right channel uses the left channel biquads. Right bands and right_fixed_filters are
  not used; only the left bands, left_fixed_filters (or left preset) are used, for both channels
- **EQ Left On**: only the left channel filters are used, the right channel is flat
- **EQ Right On**: only the right channel filters are used, the left channel is flat
- **EQ BIAMP On**: each channel uses its own filters
- **EQ Off**: the DSP bypasses all EQ biquads of both channels, fixed filters included

A channel set flat has its EQ bands (and preset) set to all pass; its fixed filters stay. So fixed filters are
affected by EQ Mode as follows:

| EQ Mode | Fixed filters |
|---|---|
| EQ BIAMP On, EQ Left On, EQ Right On | stay, including on the channel the mode makes flat |
| EQ On Right=Left | right fixed filters not used; left fixed filters apply to both channels |
| EQ Off | bypassed, with everything else |

The DSP has no per channel EQ switch, so when the selection changes the bands of a channel that is not used are
set flat and the filters of a channel that becomes used are written again.
With dac_mode PBTL only the left channel is output, so right channel EQ bands, fixed filters and Presets are ignored
(with a warning) and the left channel always gives EQ Left On.

On startup the EQ Mode Select starts at the last option available, ie EQ BIAMP On if both channels are configured.


# EQ Control configuration

## Left/Right Channel Volume and EQ Band Numbers

Left and Right Channel Volume control can be optionally configured.
If configured, both channels must be configured as follows:

```
number:
- platform: tas58xx
    channel_volume_left:
      name: _Volume Left
    channel_volume_right:
      name: _Volume Right
```

EQ bands are configured as a list for each channel, `left_bands:` and `right_bands:`. Each entry is a gain Number
in Home Assistant (with the usual Number options such as `name:`) plus its filter:

- **eq_filter:** (*Optional*): `equalizer`, `peaking`, `low_shelf`, `high_shelf`, `low_pass` or `high_pass`.
  Defaults to `equalizer`.
- **frequency:** (*Required*): 20 Hz to 16000 Hz, whole Hz, eg `80Hz` or `1.25kHz`. A fraction is rounded to the
  nearest Hz (half rounds up) with a warning in the config log, eg `31.5Hz` is built as 32 Hz. The range is checked
  after rounding.
- **q_factor:** (*Optional*): 0.1 to 20. Defaults to 2.144908 for `equalizer` and `peaking` and 0.707 for the
  shelves. Not used by `low_pass` and `high_pass` (fixed Q 0.707), a warning is given if it is set for them.

The gain of each band is -24 dB to +24 dB (`high_shelf` and `high_pass`: -24 dB to +18 dB) in 1 dB steps. A band
starts at 0 dB the first time, after that its last value is saved and restored at reboot. Saved values belong to
the Number's name, so keep a band's name when editing the YAML to keep its saved gain.

#### Limiting the gain range
The slider range of the band gains can be narrowed with `gain_min` and `gain_max` (whole dB, eg `-12dB`), at two levels:
- **Platform level**: on the `number: - platform: tas58xx` block. They apply to every band in `left_bands` and
  `right_bands` of that block, not to `channel_volume_left` / `channel_volume_right`. Each must be from -24 dB to +24 dB.
- **Band level**: on a band entry. They override the platform value for that band, in either direction, but must be
  within the filter's full range (-24 dB to +24 dB; `high_shelf` and `high_pass` -24 dB to +18 dB).

`gain_min` and `gain_max` are each worked out as: the band's own value, else the platform value, else the filter's
full range. After that `gain_min` must be less than `gain_max`. A range that does not contain 0 dB is allowed.
A value outside its allowed range, or `gain_min` not less than `gain_max`, is a config error naming the band.

```yaml
number:
  - platform: tas58xx
    gain_min: -12dB
    gain_max: 12dB
    left_bands:
      - name: Left 100Hz
        frequency: 100Hz
      - name: Left Treble
        eq_filter: high_shelf
        frequency: 8kHz
        gain_max: 6dB
```
Here Left 100Hz is -12 dB to +12 dB and Left Treble is -12 dB to +6 dB.

If the platform `gain_max` is above +18 dB, `high_shelf` and `high_pass` bands that do not set their own `gain_max` use
+18 dB as max, and one warning per number block lists them, eg: `gain_max 24 dB is above +18 dB, the limit for
high_shelf / high_pass bands - +18 dB is used as max for 'Left Treble', 'Left Air'`.

A saved gain is clamped into the band's range at boot (so is the first-boot 0 dB if the range does not contain 0 dB),
so a narrowed range takes effect after a reboot. The clamped value is published and applied but not saved again until
the slider is moved.

Any number of bands can be configured on either channel, up to 15 per channel together with that channel's fixed
filters (12 with an EQ Preset). The component allocates the biquads (see "Biquad order" above); only the filters
configured are written to the DAC, the other biquads stay at the DAC power on state which is flat. Each band's
filter, frequency, Q factor (defaulted values included) and biquad are listed in the log at startup.

If only Left EQ bands are configured then EQ On Right=Left applies them to both the left and right channels.
To control left and right channels individually, configure Right EQ bands as well. Right EQ bands can
also be configured without Left EQ bands, in which case the left channel is flat.
A channel has either EQ bands or an EQ Preset, not both (fixed filters can be used with either).
With dac_mode: PBTL only the left channel is used, right channel numbers and selects are ignored with a warning.
For EQ bands to activate correctly see the "Activation of Mixer mode and EQ settings" section above.

Example of a standard 15 band graphic EQ (ISO 2/3-octave centres with the default Q 2.144908) on the left
channel, and a few bands with other filters on the right channel:
```
number:
  - platform: tas58xx
    left_bands:
      - name: Left ---25Hz
        frequency: 25Hz
      - name: Left ---40Hz
        frequency: 40Hz
      - name: Left ---63Hz
        frequency: 63Hz
      - name: Left --100Hz
        frequency: 100Hz
      - name: Left --160Hz
        frequency: 160Hz
      - name: Left --250Hz
        frequency: 250Hz
      - name: Left --400Hz
        frequency: 400Hz
      - name: Left --630Hz
        frequency: 630Hz
      - name: Left -1000Hz
        frequency: 1000Hz
      - name: Left -1600Hz
        frequency: 1600Hz
      - name: Left -2500Hz
        frequency: 2500Hz
      - name: Left -4000Hz
        frequency: 4000Hz
      - name: Left -6300Hz
        frequency: 6300Hz
      - name: Left 10000Hz
        frequency: 10000Hz
      - name: Left 16000Hz
        frequency: 16000Hz

    right_bands:
      - name: Right Subsonic
        eq_filter: high_pass
        frequency: 30Hz
      - name: Right Bass
        eq_filter: low_shelf
        frequency: 80Hz
      - name: Right Mid
        frequency: 1kHz       # equalizer
        q_factor: 1.4
      - name: Right Treble
        eq_filter: high_shelf
        frequency: 8kHz
```

With this example and the 3150 Hz right notch from the audio_dac example above, the right channel's biquads are:
1 Right Subsonic (high pass), 2 the 3150 Hz notch (a cut), then 3 Right Bass, 4 Right Mid and 5 Right Treble in YAML
order. Biquads 6 to 15 stay flat.

All Numbers configured in Homeassistant are saved every minute and restored at next reboot.

## EQ Presets Select
EQ Presets Select provides 21 possible Frequency Cutoff selections for left and right channels of
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
Note: Each channel can have either an EQ Preset OR EQ bands, not both. For example, EQ bands on the left
channel with an EQ Preset on the right channel (a subwoofer) is valid. An EQ Preset can be configured for
one channel only, the other channel then stays at the DAC power on state which is flat.
An EQ Preset uses biquads 1 to 3 of its channel; fixed filters on the same channel follow from biquad 4
(at most 12 fixed filters on a channel with a preset).


## DAC Enable Switch
A switch can be configured to provide an Enable-Disable DAC switch in Homeassistant.
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

**pvdd_over_voltage:** and **pvdd_under_voltage:**
  - The original misspelled keys **pcdd_over_voltage:** and **pcdd_under_voltage:** are still accepted.

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
    clock_fault:
      name: I2S Clock Fault
    pvdd_over_voltage:
      name: PVDD Over Voltage
    pvdd_under_voltage:
      name: PVDD Under Voltage
    over_temp_shutdown:
      name: Over Temperature Shutdown Fault
    over_temp_warning:
      name: Over Temperature Warning
      id: over_temperature_warning
```

## Sensor
An optional tas58xx platform sensor can be defined configuration heading **faults_cleared:** can be optionally configured.
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
An Announce Volume template number can be used in
conjuction with the **mediaplayer:** YAML configurations for adjusting the
announcement pipeline audio volume separately to the media pipeline volume.
This is useful for a Text-to-Speech announcements that may have a different
volume level to the audio playing through the media pipeline.


# YAML examples in this Repository
Extensive Esphome YAML examples are provided on github by sonocotta in
[Esparagus Media Center repository](https://github.com/sonocotta/esparagus-media-center/tree/main/firmware/esphome)

