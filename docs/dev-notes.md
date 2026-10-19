# Development notes: on_audio_started rework and path to esphome/tas58xx

> **All further work has moved (2026-10-18)** to branch `dev_eq_test` of `mrtoy-me/esphome` (ESPHome's own
> tas58xx). The current notes, to-do list and EQ control proposal are in `esphome/components/tas58xx/docs/` on that
> branch. This branch (`dev_update_eq_proto`) is the frozen proof of concept: only fixes for bugs found in testing.

Notes from the session of 2026-10-10 so work can continue in a later session, either locally
or in the cloud. Read this first. It covers the goal, the requirements set so far, the current
branch state, the design, a to-check list and the plan forward.

## Upstream PR plan (2026-10-17)
Goal: EQ in ESPHome's own tas58xx (`esphome/esphome`, `esphome/components/tas58xx`). This component is the
proof of concept and the wider user test bed.

**Scope:** start from upstream tas58xx as it is on `dev`. Raise the branches already waiting in `mrtoy-me/esphome`,
then bring EQ across as in `docs/TAS58xx_EQ_Control_Proposal_v1.docx`. No other features from this component (no
mixer select, channel volume numbers, enable switch or fault sensor).

**Rules:**
- Based on ESPHome `dev`, not a beta or release. Local checks use the `dev` worktree (`D:/NAS/github/esphome-dev`).
- One upstream PR open at a time. Each PR is under 1000 lines, preferably under 500, with tests in
  `tests/components/tas58xx` and an esphome.io docs PR where users see a change.
- The EQ design is implemented and hardware tested on this component first; the upstream PRs port proven behaviour.
- Upstream code is written for upstream's structure (`ModelInfo` per model, `model_*.cpp`), reusing this
  component's math, register tables and test cases, not copying its files.
- Follow current `dev` conventions: `DOMAIN`, `.options()` for enum option dicts (#20910 / #20911),
  `status_comm_result` (#20863) and `get_declaring_config` (#20912) once merged; clang-format, ruff and clang-tidy clean.
- EQ on ESP32 only (ESP32-S3 recommended); reject or warn on ESP8266 and RP2040, which upstream tas58xx also builds for.

| # | PR | Source / content | Est. size |
|---|---|---|---|
| 1 | `[tas58xx] Add fault_output to choose the FAULTZ pin` | branch `add_fault_output` (`ee421f4`) | ~110 lines |
| 2 | `[tas58xx] Add modulation option` (`bd_mode` / `1spw_mode`) | branch `add_modulation_option` (`dd23c49`) | ~30 lines |
| 3 | `[tas58xx] Correct the register 0x46 comments` | TAS5825M 0x46 is DSP_CTRL (datasheet 9.6.1.16), 0x11 = 96 kHz processing with default coefficients; "not documented" is only true for TAS5805M. Checked against PPC 2.0 dumps at 96 and 48 kHz (2026-10-16) | ~5 lines |
| 4 | EQ foundation (no user change) | EQ addresses per model in `ModelInfo` (biquad base left and right, EQ bypass flag); DSP write across pages (mixer write moved to it); "write once audio starts" generalised from `mixer_written_`; `make_biquad()` and 5.27 conversion with C++ unit tests against the PPC reference | 350-450 |
| 5 | EQ bands | number platform: `left_bands` / `right_bands`, filter types, frequency (rounded) and Q rules, gain limits, biquad allocation, values restored | 450-550 |
| 6 | Stereo bands and EQ switch | `stereo_bands` (one slider writes both channels); switch platform `eq_mode` On / Off with the bypass flag, `RESTORE_DEFAULT_ON`. **The switch is optional: without it EQ is always On** | 300-450 |
| 7 | Fixed filters | `audio_dac` `left_fixed_filters` / `right_fixed_filters` (notch, band pass), `fixed_filters_always_on` (warning if set without the switch), the write-flat EQ Off path | 250-350 |
| 8 | EQ presets | select platform `eq_preset_left_channel` / `eq_preset_right_channel`, preset table, presets mixed with bands (preset biquads 1-3) | 300-400 |

Notes:
- 1 and 2 each apply cleanly to `dev` but conflict with each other (same lines in `audio_dac.py`, `tas58xx.cpp`
  and the tests): raise 1, then rebase 2 after 1 merges. Both need updating to `dev` first (`.options()` for their
  option dicts) and an esphome.io docs PR.
- 7 and 8 could be one PR if the port comes out under about 600 lines.
- 5 may approach 500 lines; if it goes over, split it into "number platform and allocation" and "biquad writes and
  restore".
- Sizes are estimates from this component, including tests.

**To-do for the `dev_eq_test` branch in ESPHome's tas58xx (`mrtoy-me/esphome`, based on `dev`; the same list is in
`TAS58XX_TODO.md` on that branch):**
- `volume_follows: <audio_dac id>` option: a DAC that no speaker drives (eg a second DAC on the same I2S output)
  copies volume and mute from the given DAC, at boot and on every change, so no media player `on_volume` /
  `on_mute` / `on_unmute` lambdas are needed. Until then the YAML workaround is those three triggers copying
  `id(dac1).volume()` and `is_muted()` to the second DAC (added 2026-10-17).
- `FS_MON` (0x37) check: once audio starts, read the detected input sample rate and log a warning if it is not
  48 kHz or 96 kHz (eg an unexpected `sample_rate:` of 16k or 32k). It shows the I2S input rate, not the DSP
  processing rate, and cannot tell 44.1 kHz from 48 kHz (44.1k has no code and reads back as 48k), so 44.1k input
  still needs the acoustic check (narrow +12 dB peak at 1 kHz, measured). Codes: 48k `1001`, 96k `1011`
  (added 2026-10-18).

**Possible extra PRs (not planned):**
- **Mixer mode select** (branch `add_mixer_select`, in progress 2026-10-17): `select:` `mixer_mode` on upstream's
  mixer code, restored after reboot, PBTL offers only mono / left / right.
- **Channel volume numbers** (branch `add_channel_volume`, planned 2026-10-17): `number:` `channel_volume_left` /
  `channel_volume_right`, -24 to +24 dB, DSP channel volume per model.
- **Chip ID check:** `I2CDevice::check_chip_id()` (#20941) could confirm the device is a TAS5805M / TAS5825M at
  setup. First check the datasheets for a usable ID register and its value per model.
- **Hex log macros:** `ESP_LOGV_HEX` / `ESP_LOGD_HEX` (#20931) for verbose dumps of DSP writes or coefficients.
  Minor; only if the EQ code logs raw bytes.

## EQ band and fixed filter YAML rework (plan, 2026-10-16, branch `dev_update_eq_proto`, named `dev_update_eq_test` until 2026-10-18)
Branch `dev_update_eq_proto` is based on `dev_update_with_onaudio_plus_eq` (`0a1bd72`). **Breaking YAML changes
against `main` are accepted on this branch only.**
Status (2026-10-16): implemented (`d4b104f` code, `a350fbd` range fixes, `2a38076` examples, `bf66b5f` compat, then
README). Checked with ESPHome 2026.10.0b4 `config` and `compile --only-generate` (all 41 compat cases, the 4 example
YAMLs and the README examples); helpers checked in a Python port against PPC and TI's formulas. **Not yet compiled
for ESP32 or hardware tested** (no host or ESP-IDF toolchain on the PC used).

### Final YAML
```yaml
audio_dac:
  - platform: tas58xx
    id: tas58xx_dac
    # ...existing options...
    left_fixed_filters:              # optional, notch and band_pass only (no gain input)
      - eq_filter: notch             # required
        frequency: 50Hz              # required
        q_factor: 5                  # required, bandwidth = frequency / q_factor
    right_fixed_filters:
      - eq_filter: notch
        frequency: 3150Hz
        q_factor: 10

number:
  - platform: tas58xx
    tas58xx_id: tas58xx_dac
    left_bands:                      # optional, one gain number (slider) per entry
      - name: "Left Subsonic"
        eq_filter: high_pass
        frequency: 30Hz              # q_factor not used by low_pass / high_pass (warning if given)
      - name: "Left Bass"
        eq_filter: low_shelf
        frequency: 80Hz
      - name: "Left Mid"
        frequency: 1kHz              # eq_filter defaults to equalizer
        q_factor: 1.4                # equalizer and peaking default 2.144908, shelves 0.707
      - name: "Left Treble"
        eq_filter: high_shelf
        frequency: 8kHz
    right_bands:
      - name: "Right Bass"
        eq_filter: low_shelf
        frequency: 80Hz

select:
  - platform: tas58xx
    tas58xx_id: tas58xx_dac
    eq_mode:                         # required with any bands, preset or fixed filters
      name: "EQ Mode"
```

### Number bands (`left_bands` / `right_bands`, number platform)
- Lists (ESPHome precedent: `kamstrup_kmp` and `vbus` `custom:` lists of entities extended with options).
  Each entry is the gain number (`EqGainNumber` schema) plus:
  - `eq_filter`: `equalizer` (default), `peaking`, `low_shelf`, `high_shelf`, `low_pass`, `high_pass`.
    `all_pass` is not offered (used internally for flat biquads).
  - `frequency`: required, ESPHome frequency (`80Hz`, `1.25kHz`), whole Hz, 20 Hz to 16 kHz. A fraction such
    as `31.5Hz` is rounded to the nearest Hz (half up) with a warning, range checked after rounding (changed
    2026-10-17 from an error: before ESPHome 2026.11 (#20889) `cv.frequency` multiplied, so 175 whole-Hz values
    written in kHz, eg `1.001kHz`, `2.01kHz`, parsed as 1000.9999.. and were rejected; the warning has a 1e-6
    tolerance so those do not warn). The C++ keeps `uint16_t` frequency.
  - `q_factor`: 0.1 to 20. Default 2.144908 for `equalizer` and `peaking`, the constant Q of a 2/3-octave graphic
    EQ, Q = sqrt(2^N) / (2^N - 1) with N = 2/3 (sources: ISO 266:1997 preferred frequencies; constant-Q 15 band
    equalizers such as the dbx 3215, "15 ISO standard center, 2/3 octave constant Q frequency bands" 25 Hz to
    16 kHz). Default 0.707 for `low_shelf` and `high_shelf`. Warning if given for `low_pass` / `high_pass` (fixed
    Q 0.707). A full 15 band graphic EQ is the ISO 2/3-octave centres 25, 40, 63, 100, 160, 250, 400, 630, 1000,
    1600, 2500, 4000, 6300, 10000, 16000 Hz with only `name` and `frequency` given.
  - History: `6b98ca4` (2026-04-10) tested these centres with Q 2.144908 for every band (`EQ_BAND_QFACTOR[0]`);
    the descending 2.0 to 0.5 table is not a standard.
- Gain ±24 dB (was ±15), except `high_shelf` and `high_pass`: -24 to +18 dB. A 5.27 coefficient holds just under
  +16 and for those two filters b1 is about -2 x the linear gain, which overflows above +18 dB (checked 2026-10-16
  from 20 Hz to 16 kHz and Q 0.1 to 20; the other filters do not overflow at +24 dB; all filters stable). The hub
  also clamps the gain of those filters to +18 dB. Starts at 0 dB on first boot, then the saved value is restored. Saved values are tied to
  the number's name, not its biquad. On `low_pass` / `high_pass` the gain sets the passband level (PPC does the same).
- Any filter with a gain input is defined only here, so always has a slider. No fixed gain, no initial gain.
- The `left_eq_gain_<freq>Hz` / `right_eq_gain_<freq>Hz` keys are removed.

### Fixed filters (`left_fixed_filters` / `right_fixed_filters`, audio_dac)
- Gain-less filters only: `notch`, `band_pass`. `eq_filter`, `frequency` and `q_factor` are all required
  (0.707 is a poor default for a notch). Frequency 20 Hz to 16 kHz, whole Hz.
- `q_factor` 0.6 to 30. Near Q 0.5 TI's band pass lower edge (frequency - bandwidth / 2) reaches 0 Hz and its pole
  reaches the unit circle: 0.99999974 at Q 0.5001 from the formula itself, exactly 1 after 5.27 rounding (not
  stable). At Q 0.6 the largest pole radius from 20 Hz to 16 kHz is 0.99973 (a 38 ms time constant). Notch has no
  such problem but uses the same limit. With frequency at most 16 kHz the upper edge stays under 48 kHz, so no
  upper edge check is needed. Q at most 30 is a practical limit, not a precision one: measured at the real null,
  a 20 Hz notch is still over 72 dB deep at Q 100, the null moved by under 0.03 Hz (checked 2026-10-16).
- C++ notch and band pass checked against TI's formula over 85 frequencies from 20 Hz to 16 kHz and 16 Q values
  (2720 filters): 0 LSB difference everywhere.
- The component works out bandwidth = frequency / q_factor (as a double). PPC takes a bandwidth and ignores its
  QVal field for these filters: to copy a PPC design use q_factor = frequency / bandwidth.

### Biquad allocation and order
- The 15 biquads of a channel run in series, so order does not change the response, only the level between
  biquads. Filters are sorted at compile time by type (stable, YAML order kept within a group):
  1. `high_pass`
  2. `low_pass`, `notch`, `band_pass`
  3. `low_shelf`, `high_shelf`, `equalizer`, `peaking`
- Bands and fixed filters are packed together from biquad 1 upward, so each channel is written in one run.
  A channel with a preset keeps the preset in biquads 1 to 3, fixed filters follow.
- More than 15 filters on a channel (12 with a preset) is an error. Bands and a preset on the same channel is
  still an error.

### EQ modes
- A channel with bands, a preset or fixed filters counts as having EQ: the `eq_mode` select is required and the
  EQ modes are derived as now. EQ mode changes affect fixed filters (document this).
- EQ Off bypasses every biquad, fixed filters included.
- A channel set flat by a mode (right in EQ Left On, left in EQ Right On) has its band biquads set to all pass;
  its fixed filters stay.
- EQ On Right=Left: the right channel uses the left channel's biquads. Right bands and right_fixed_filters are
  not used; only left bands and left_fixed_filters are used, for both channels.
- PBTL: right bands and right_fixed_filters log a "not used" warning.
- For the docs, fixed filters by EQ mode:

  | EQ mode | Fixed filters |
  |---|---|
  | EQ BIAMP On, EQ Left On, EQ Right On | stay, including on the channel the mode makes flat |
  | EQ On Right=Left | right fixed filters not used; left fixed filters apply to both channels |
  | EQ Off | bypassed, with everything else |

### C++
- Helpers: `FILTER_TYPE_BAND_PASS`, `FILTER_TYPE_NOTCH`, a shared `bandwidth_filter()` and
  `make_bandwidth_biquad(type, sample_rate, frequency, bandwidth)`. Old helpers are in `c1dbeac`; the optimised
  form drops TI's C = tan(Wc / 2), which cancels, and uses 1 / cos(pi * bw / Fs) = sqrt(1 + t * t).
  Checked (2026-10-16): original, optimised and TI's formula are identical in 5.27 for the 12 band pass and notch
  cases in "filters PPC tests results Claude sorted.cfg", all within 1 LSB of PPC (PPC's own rounding: 19 of
  240 coefficients across all filters in that file differ by 1 LSB, nearest rounding is clearly what PPC uses).
- One generated constant table per channel describing each used biquad (filter type, frequency, Q or bandwidth,
  band or fixed), replacing `EQ_DEFAULT_BANDS`. Each `EqGainNumber` gets its biquad index.
- `write_eq_channels_()`: filters state writes every used biquad; flat writes all pass to the band biquads only.
- Dump: each number logs its biquad, filter, frequency and Q, defaulted values included; the hub logs the fixed
  filters. Only filters in YAML are listed; the docs note that unlisted biquads keep their power-on (flat) values.

### Docs, examples and checks
- README EQ section: the YAML above, filters and what each uses, defaults (with the ISO 266 / 2/3-octave
  constant-Q source for 2.144908), ordering, flat and mode behaviour (the fixed filter table above), PPC Q
  conversion, unlisted biquads flat, and a validation table:

  | | Number bands | Fixed filters (`notch`, `band_pass`) |
  |---|---|---|
  | `frequency` | required, 20 to 16000 Hz, whole Hz (fraction rounded, warning) | required, 20 to 16000 Hz, whole Hz (fraction rounded, warning) |
  | `q_factor` | 0.1 to 20; default 2.144908 (equalizer, peaking) or 0.707 (shelves); warning if given for low/high pass | required, 0.6 to 30 |
  | count | bands + fixed filters at most 15 per channel (12 with a preset), else an error | |
- Convert the 4 example YAMLs and the `docs/compat` cases; add cases for: q_factor warning, Q out of range,
  more than 15 filters, fixed filters with a preset, fixed filters only, PBTL right.
- `docs/tools/ppc_check`: add the notch and band pass cases from the PPC reference file.
- Hardware: EQ modes with fixed filters (flat keeps them, EQ Off removes them, Right=Left uses left only),
  saved gains restored, no I2C errors.

## Where we stopped (2026-10-14, branch `dev_update_with_onaudio_plus_eq`)
Branch `dev_update_with_onaudio_plus_eq` is based on `dev_update_with_onaudio` (`cf23bf0`, hardware testing
had gone well) and holds the EQ rework, in 4 commits that can be checked or reverted separately, plus follow-up commits
`7493875` (EQ modes), `f7e75d1`, `712d989` (EQ Left/Right/Right=Left modes), `78d21c2` (PBTL left is always EQ Left On).
Not yet hardware tested. `dev_update_with_onaudio` itself is unchanged.
Later on 2026-10-14 (newest first, HEAD `0653923`): `0653923` 5.27 constants inside `double_to_fixed_5_27`, `20c886b`
mixer select offset per instance (two DACs), `12b34f0` helpers in `esphome::tas58xx` with `static` file local
functions, `fbdbc6b` helper verbose logs removed, `ae320d0` notes, `f274aeb` Q as a double, `2220468` sand_box future band options
kept as comments in `number/__init__.py`, `d0e3499` / `926e1c0` PPC reference check, `7801822` I2CDevice functions
and one I2C write per DSP page segment, `23bfaa1` DSP writes in runs without the heap, `2749ff8` TAS5825M EQ flag byte
order, `db3b8a1` helper cleanup; before those the EQ mode state tracking (`62d138f`) and logs (`9edb28d`, `8570b14`).
See the sections "Helper and DSP write rework", "Mixer values checked", "Helpers against the TI PurePath Console
reference" and "Open items for next time" at the end of this file.
1. **`e826a35` EQ gains and presets per channel, only defined biquads written.**
   - A channel has EQ gain numbers or an EQ preset, not both. Any number of gain bands per channel (not all
     15), right gains need no left gains, a lone preset on either channel is valid (the other channel keeps
     the DSP power on state).
   - Only biquads defined in YAML are written (a per-band bitmask set by `set_eq_gain` and a per channel
     flag set by `set_eq_preset`). Unwritten biquads are at B0 = 1.0 (`0x08000000` in 5.27), B1, B2, A1, A2 = 0
     (flat) from the reset that `init_()` does every start: TI memory maps (SLOA263A Table 6 for TAS5805M,
     SLAA786A Table 9 for TAS5825M) list these defaults for all 30 biquads, RESET_CTRL bit 4 resets the DSP.
   - `USE_TAS58XX_EQ_BIAMP` is gone: `USE_TAS58XX_EQ_GAINS` for any gain number, `USE_TAS58XX_EQ_PRESETS`
     for a preset select on either channel.
   - EQ Mode options are "EQ Off", "EQ On", "EQ BIAMP On" (a second compatibility break against `main`:
     it was "Off", "EQ 15 Band", "EQ BIAMP 15 Band", "EQ Presets"). Superseded by the EQ mode rule below.
     `derive_eq_modes()` in `audio_dac.py`; per channel checks in `get_eq_channels()` and `validate_eq_channels()`.
   - EQ mode rule (latest): `EqMode` = EQ_OFF, EQ_ON (ganged, shown as "EQ On Right=Left"), EQ_LEFT_ON,
     EQ_RIGHT_ON, EQ_BIAMP_ON. Left gains only: EQ On. Lone left preset: EQ Left On. Right only: EQ Right On.
     Both channels: all four. PBTL ignores the right channel and always offers EQ Left On for the left channel (datasheets: PBTL input is the left I2S frame, and the crossbar
     defaults route the left output to analog left; TI does not say outright which channel's filters drive PBTL). The three non ganged modes share register values; a channel not used is set flat
     and rewritten when used again. Mode changes write only what changes: `eq_filters_written_` per channel
     records whether the DSP holds the channel's filters or is flat, `eq_channel_need()` says what a mode needs
     (filters, flat, or any), `write_eq_channels_()` writes the difference. EQ Off and EQ On Right=Left from a mode
     with left filters write only the mode register. A gain or preset change is written only when the channel's
     filters are in the DSP. A rewrite after the clock was lost rewrites the filters in the DSP.
     Debug log: one "<Left|Right> Channel EQ >> flat" entry for each channel set flat, and "EQ Ganged >> On" or "EQ Ganged >> Off"
     whenever the ganging written to the DSP changes (`eq_ganged_`). Select index to mode through
     the `config_eq_modes` mask.
   - Right EQ gain numbers with `dac_mode: PBTL` log a warning. Compat cases 19 to 28 added.
2. **`80dd98b` presets calculated with the helpers.** `eq_profiles.h` (42 coefficient tables, 446 lines) is
   replaced by a 21 row `EQ_PRESETS` table: BQ1 and BQ2 are the same low pass (LF) or high pass (HF) filter at
   the cutoff, BQ3 is a high shelf (HF: 200 Hz for 60 to 100, 220 to 300 Hz for 110 to 150, +1 dB, Q 0.5) or
   all pass. `FilterType` and `make_biquad()` added to the helpers. `sand_box` computes presets the same way
   but its HF branch uses a low pass and the code does not compile, so it was rewritten, not copied.
3. **`2f51b49` header reorganisation.** `tas58xx_eq_common.h` is split into `tas58xx_defs.h` (enums, counts,
   text, registers shared with the entities) and `tas58xx_eq.h` (EQ only, included by `tas58xx.cpp` only).
   Four biquad address tables are replaced by `eq_biquad_address()` (checked against all 60 old entries on both
   chips). `tas58xx_minimal.h` is renamed `tas58xx_startup.h`.
4. **`275dae4` EQ band table.** `EqBand` (type, frequency, Q) in a constant table, read through a per channel
   pointer (`config_eq_bands()`), so per band filter type, frequency and Q can be added later as a generated
   constant table. Coefficients unchanged.
- **Future per band filters (from `sand_box`):** `filter_type`, `frequency`, `q_factor` per band. The current
  keys contain the frequency (`left_eq_gain_20Hz`), so the nested `left_eq_band_N` form `sand_box` comments
  out is the one that can work; keep the flat keys as "Equalizer at the default frequency and Q" and reject a
  band defined in both forms. `sand_box`'s commented `EQ_FILTERS` map has High Pass and Low Pass swapped. Low and
  high pass have a fixed Q, so `q_factor` is not valid for them. Frequency must stay below 48 kHz.
- **To check on hardware** (only what the datasheets and `main` cannot settle; non ganged biquad behaviour and
  reset defaults are settled from the datasheets: DSP_MISC bit 3 / Gang EQ flag, flat reset coefficients):
  1. Mode switching in BTL: EQ Left On, EQ Right On, EQ BIAMP On, EQ On Right=Left, EQ Off. Each channel
     should be filtered or flat as the mode says, and returning to a mode restores that channel's filters.
  2. Any audible click while biquads are rewritten on a mode change (the code does not mute around the writes).
  3. PBTL: select EQ Left On and confirm a left gain change is audible (`main` only ever ran ganged there).
  4. Saved gains are restored after flashing, and the presets sound the same as before (the computed
     coefficients were checked against the old tables by you, not re-run here).
  PBTL is otherwise the same as `main`, which ran the same EQ writes in PBTL as in BTL.
  5. TAS5825M only: EQ Off bypasses the EQ and EQ On Right=Left gangs the channels. The Gang EQ and Bypass EQ
     flags are now sent as 00 00 00 01 (datasheet big endian value 1); before they were sent as 01 00 00 00,
     which only worked if the DSP treats any non zero value as true.
- **Checks run for each commit:** `cpp_check.sh` clang-tidy parse in 5 define sets (0 diagnostics, 0 fatal
  include errors), ci-custom (0 findings), compat matrix. For commit 1 the generated `main.cpp` of 16 configs
  is unchanged apart from the EQ Mode option labels.

## Earlier stop (2026-10-14)
- **Single EQ gain number class (`dev_update_with_onaudio` only):** the 30 per-band classes
  (60 files) are replaced by `number/eq_gain_number.h/.cpp`, `EqGainNumber(Channels channel,
  uint8_t band)`. Channel and band are constructor parameters because they never change.
  `number/__init__.py` shrinks from about 650 to about 170 lines: the schema and `to_code` are
  driven by `CONF_LEFT_EQ_GAINS` / `CONF_RIGHT_EQ_GAINS` from `audio_dac.py`, the position in
  the tuple being the band index. YAML keys, the 15 required bands per side, the validation
  messages, the range (+/-15 dB), the defines and the hub are unchanged. `dev_update` is not
  changed.
  - **Checked:** generated `main.cpp` for the 4 example YAMLs and 5 EQ test configs is identical
    to before apart from the class name, the constructor arguments, the auto-generated variable
    names (`tas58xx_lefteqgain20hz_id` -> `tas58xx_eqgainnumber_id`, `_2`, ...) and the
    loop-override count, which is now one term per class (`30 * HasLoopOverride<EqGainNumber>`);
    `main`-compatibility matrix unchanged; the three EQ gain validation errors unchanged;
    clang-tidy parse in the 5 define sets: 0 diagnostics; ci-custom: 0 findings.
  - **To check on hardware:** gains saved by the previous build are restored after flashing.
    Saved values are keyed on the entity name (not the class), so they should be; confirm it.
    The log lines changed: one line per gain, `Tas58xx Left EQ Gain Number: 31.5Hz Band 'name'`,
    instead of a header plus indented lines.
  - **Next for gains:** making the bands optional (drop the all-15 rule, let any band set
    `USE_TAS58XX_EQ_GAINS` / `USE_TAS58XX_EQ_BIAMP`, decide whether right bands need left bands,
    update the README).

## Earlier stop (2026-10-12)
- **Upstream PR branch ready for review:** `add_fault_output` on the fork `mrtoy-me/esphome`
  (commit `ee421f4`, based on upstream `esphome/esphome` `dev` `9acc07a`). It adds the
  `fault_output:` option to ESPHome's own tas58xx. Not yet raised as a PR. Details in
  "fault_output (upstream PR)" below.
- **TAS5825M fault pin fix in this component:** `tas58xx_minimal.h` wrote `{0x60, 0x10}`, a
  reserved bit, so GPIO1 never became an output and FAULTZ never reached the pin. Now
  `{0x60, 0x02}` (GPIO1_OE) on both `dev_update` (`00cbe49`) and `dev_update_with_onaudio`
  (`a6c67fe`). Worth checking on hardware if the board wires GPIO1.
- **Not reviewed:** two commits made on GitHub, `7591cba` and `8b2e4e2` ("Update
  audio_dac.py"), are on `dev_update_with_onaudio` before `a6c67fe`.
- **Your fork's `dev` is behind upstream** (`94ed178`, 2026-09-30, no `model_tas5825m.cpp`).
  Sync it on GitHub if you want it current; `add_fault_output` does not depend on it.

## Earlier stop (2026-10-11)
- **Next step:** hardware test both `dev_update` (loop design, ESPHome 2026.2.0+, boot sound
  YAML) and `dev_update_with_onaudio` (on_audio_started design, ESPHome 2026.10.0+, no boot
  sound). Use the to-check list below during testing.
- **Pending (you):** delete `dev_update_with_onaudio_split` (identical to
  `dev_update_with_onaudio`) and `claude/jolly-ramanujan-7zxbba` (contained in `dev_update`).
  The cloud session's git proxy refuses branch deletion (HTTP 403); use the GitHub Branches
  page or `git push origin --delete <branch>`.
- **Checks passed on `dev_update_with_onaudio`:** the `main`-compatibility matrix, the 4
  example YAMLs, the generated EQ Mode select options for each EQ mode, ci-custom (0
  findings), and the C++ parse against ESPHome beta in 5 define sets (0 errors, 0 fatal
  include errors).
- **Latest change (both `dev_update` and `dev_update_with_onaudio`):** the `audio_dac:` keys
  used by ESPHome's own tas58xx are accepted as aliases, so the same YAML works when switching
  between ESPHome beta and these branches: `model` → `tas58xx_dac`, `volume_min_db` →
  `volume_min`, `volume_max_db` → `volume_max` (`cv.rename_key`; both forms together is an
  error). Only these three were asked for. Not added: fractional dB volumes, optional
  `enable_pin`/`ignore_enable_pin_warning`, and beta's TAS5825M-only binary sensor keys.
- **To resume:** check out `dev_update_with_onaudio`, read this file, and ask Claude to read it
  too. Locally, `esphome compile` of an example YAML is the best build check; it wasn't
  possible in the cloud session.

## Goal
- **Ultimate objective:** migrate the EQ functionality to ESPHome's own `esphome/tas58xx`
  (CODEOWNERS `@mrtoy-me @remcom`).
- **First:** build the functionality into this external component (`esphome-tas58xx`) as a
  reference, then hardware test and refine it.
- **Then:** develop small, targeted PRs to bring it across to `esphome/tas58xx`.
- This external component is **not** being upstreamed as-is. Lint tidy-up is "as far as
  feasible"; lint issues are ignored where fixing them would break YAML compared with `main`.

## Requirements set so far
### YAML compatibility (compared with `main`)
- No breaking YAML changes compared with `main`, except one accepted break: binary sensor
  `clock fault:` is now `clock_fault:`.
- ESPHome tas58xx `audio_dac:` keys `model`, `volume_min_db` and `volume_max_db` are accepted
  as aliases of `tas58xx_dac`, `volume_min` and `volume_max` (both branches).
- `pvdd_over_voltage` / `pvdd_under_voltage` are the canonical keys. The original `pcdd_` keys
  are still accepted (`cv.rename_key`). Specifying both is an error.
- The mixed-case EQ gain keys (`left_eq_gain_20Hz`, `left_eq_gain_31.5Hz`, ...) stay as they
  are, with `# NOLINT`. Lowercase keys and other EQ redesign come later; that work is in a
  separate, unfinished branch.
- Validation rules added on `dev_update` that rejected configs valid on `main` are now
  warnings, not errors:
  - `channel_volume_right` with PBTL
  - `eq_preset_right_channel` with PBTL
  - `eq_mode` select with no EQ gains or presets
- `refresh_eq: AUTO|MANUAL` is still accepted so `main` configs validate. On
  `dev_update_with_onaudio` it generates no code and logs "refresh_eq is no longer used and
  can be removed".

### Activation of DSP settings (`dev_update_with_onaudio`)
- **Reference design:** ESPHome beta `tas58xx` (2026.10). Use `on_audio_started()` plus a
  POWER_STATE = PLAY fallback in `update()`. Match or improve on beta's resilience.
- **No boot sound:** the boot sound and its YAML (on_boot, files, substitutions, .flac files)
  are removed.
- **Minimum version:** ESPHome 2026.10.0 (enforced with `cv.require_esphome_version`).
  `dev_update` stays at 2026.2.0.
- **No dependence on entities:** nothing relies on a particular EQ gain entity. EQ gains are
  to become fully optional later.
- **Speaker `timeout`:** must not matter (beta's tests use the default 500 ms). DSP settings
  are only written with a running I2S clock.
- **Accepted assumption:** POWER_STATE (register 0x68) reading PLAY means the I2S clock is
  running, and leaves PLAY when the clock stops. Accepted until user experience shows
  otherwise.
- **snapclient** (luar123's esphome PR #14389 / PR #8350 / c-MM fork) drives I2S itself and
  never calls `on_audio_started()`. It is covered by the `update()` fallback; MANUAL is not
  needed.

### Structure and platforms
- Keep the existing structure in `esphome-tas58xx` (`#ifdef USE_TAS5805M_DAC` model
  selection). Do **not** adopt upstream's `ModelInfo` structs here.
- EQ runs on ESP32 only (ESP32-S3 recommended). Upstream PRs should warn about or reject EQ on
  ESP8266 and RP2040; upstream tas58xx builds for all three.
- clang-format, ruff and full clang-tidy clean-up are done per upstream PR, not on this
  component.
- Single `EqGainNumber` class instead of 30 per-band classes: done on `dev_update_with_onaudio`
  (2026-10-14), after hardware testing went well. See "Where we stopped".

### Git
- `dev_update`: tidied loop design (lint fixes, `main`-compatible YAML). Requires 2026.2.0.
  Not changed by the on_audio_started work.
- `dev_update_with_onaudio`: the on_audio_started design, the branch to hardware test.
  Requires ESPHome 2026.10.0.

## Branch state (2026-10-11)
| Branch | Contents |
|---|---|
| `main`, `beta`, `dev` | unchanged by this work (loop design) |
| `dev_update` | loop design. Lint fixes (ci-custom clean), example YAML fixes, `main`-compatible YAML (warnings instead of errors, `pcdd_` aliases), README key and modulation fixes, ESPHome tas58xx key aliases. Requires 2026.2.0. |
| `dev_update_with_onaudio` | the on_audio_started design described below, for hardware testing. Requires 2026.10.0. |
| `dev_update_with_onaudio_split` | review branch for the setter split, startup delay removal and EQ Mode select change; fast-forwarded into `dev_update_with_onaudio`, can be deleted |
| `claude/jolly-ramanujan-7zxbba` | fully contained in `dev_update`; can be deleted |

## Design on `dev_update_with_onaudio`
- **Setters and writers:** each DSP setting has a setter, called only by the numbers and
  selects (`setup()` with the restored value, `control()` on a change), and a `write_..._()`
  function that only writes the saved value:
  `set_eq_mode_`/`write_eq_mode_`, `set_input_mixer_mode`/`write_input_mixer_mode_`,
  `set_channel_volume`/`write_channel_volume_`, `set_eq_gain`/`write_eq_gain_`,
  `set_eq_preset`/`write_eq_preset_`.
  A setter validates and saves its value, then writes only if `can_write_dsp_()` returns true:
  `dsp_ready_` is set and the DAC is playing (POWER_STATE read). If the DAC isn't playing, the
  value is saved and `dsp_ready_` is cleared, so everything is rewritten at the next trigger.
- **`write_dsp_settings_()`** calls the `write_..._()` functions for all saved values, in this
  order: EQ mode, input mixer, L/R channel volumes, all EQ bands (bands without a number are
  written as 0 dB), presets. `dsp_ready_` is set only when all writes succeed, so a failure is
  retried at the next trigger.
- **Triggers:**
  - `on_audio_started()`, called by the i2s speaker when its clock starts; the speaker needs
    `audio_dac:` set
  - `update()`, when `!dsp_ready_` and POWER_STATE reads PLAY (snapclient, speakers without
    `audio_dac:`, retries)
- **Removed:** `loop()`, `LoopSetupStage`, `refresh_eq_settings()`, the 16000 Hz and
  EqModeSelect triggers, and the MANUAL code.
- **`write_input_mixer_mode_()`** is the equivalent of beta's `write_mixer_()`: same registers,
  same coefficients.
- **Faults:** no startup delay. Faults latched while the DAC powers up are cleared at the end
  of setup (as beta does); every binary sensor is published on the first `update()`, then
  only on change. Without a boot sound there is no I2S clock until the first playback, so a
  configured `clock_fault` sensor reads ON until then.
- **EQ Mode select options:** derived at validation, following ESPHome's `i2s_audio` and
  `logger` patterns. `audio_dac.py` `_final_validate` derives the EQ mode (any gain key, not
  only 20 Hz) and stores it in `CORE.data[DOMAIN][str(id)]`; `get_eq_mode()` reads it in both
  `audio_dac` and `select` `to_code`. The select passes `options=` to `new_select` ("Off" plus
  the mode), so ESPHome stores them in flash; `EqModeSelect::setup()` only sets the initial
  index. Shared helpers in `audio_dac.py`: `find_matching_config()`,
  `get_audio_dac_config()` (uses `get_path_for_id()`), `has_eq_gains()`, and the
  `CONF_LEFT_EQ_GAINS`/`CONF_RIGHT_EQ_GAINS` key tuples.

## To-check list
1. **The old mixer bug** (with no `eq_mode` select and no EQ gains, `mixer_mode` and the
   channel volumes were never written to the DAC) is fixed on `dev_update_with_onaudio`.
   It is still present on `main`, `beta`, `dev` and `dev_update`.
2. **Re-enabling the DAC.** With `timeout: never` the speaker never stops, so
   `on_audio_started()` fires once only. A change made while `enable_dac` is off (deep sleep)
   is deferred. After switching it back on, the `update()` fallback should rewrite everything
   within one `update_interval`. Include this in hardware testing.
3. **What to look for in the hardware logs:**
   - At the first audio start: "Audio started", then the settings, then "DSP settings
     written", once only.
   - A change made while idle: "DAC not playing, settings written at next audio start".
   - Watch for an ESPHome "took a long time" warning. Writing every band at once is roughly
     30–40 ms at 400 kHz; this is an estimate, not measured. If it appears, split the writes
     with `defer()`.
   - snapclient: settings should be written within one `update_interval` of playback.
4. **Old branch:** `claude/jolly-ramanujan-7zxbba` can be deleted.
5. **After testing:** decide how `dev_update_with_onaudio` is brought into `dev_update`
   or `beta`. The two branches differ by one equivalent README commit (`2257e2f` on
   `dev_update`, `aef0fd6` on `dev_update_with_onaudio`), so a merge is clean.
6. **Other selects:** the mixer mode and EQ preset selects still build their options at
   runtime. Both could pass `options=` from codegen like the EQ Mode select; the mixer select
   also uses two non-const globals (`MAX_SELECT_INDEX`, `MIN_MIXER_MODE`).
7. **EQ Mode select state is not restored after a reboot** (it starts at the configured mode).
   Decide whether it should be.

## Plan forward
1. **Hardware test `dev_update` and `dev_update_with_onaudio`.**
   - Speaker media player: AUTO, presets, 15-band and biamp configs.
   - snapclient.
   - Changes while idle, and the `enable_dac` off/on case.
   - Also with the speaker's default `timeout` (remove `timeout: never`).
2. **Fix anything found, then decide on point 1 and point 5 of the to-check list.**
3. ~~Refactor the 30 EQ gain number classes into one parameterised class.~~ Done on
   `dev_update_with_onaudio`; YAML keys unchanged.
4. **Bring in the separate EQ redesign branch** (lowercase keys, optional gains, and so on).
5. **Upstream PRs to `esphome/tas58xx`:** replaced by "Upstream PR plan" at the top of this file (2026-10-17).

## fault_output (upstream PR)
Branch `add_fault_output` on `mrtoy-me/esphome`, one commit `ee421f4`
("[tas58xx] Add fault_output to choose the FAULTZ pin"), based on upstream `dev` `9acc07a`.

**YAML:** `fault_output:` under `audio_dac: - platform: tas58xx`. Values per model:
`tas5805m`: `adr` (default) or `none`; `tas5825m`: `gpio0`, `gpio1` (default), `gpio2` or
`none`. Case-insensitive. `none` writes nothing, so the pin keeps its power-on state. The
defaults match the old hard-coded behaviour. The key is `fault_output`, not `fault_pin`,
because `*_pin` in ESPHome means an MCU pin.

**Design (chosen after comparing a computed `0x61+n` version and a sparse per-model list):**
- `tas58xx.h`: `enum FaultOutput { ADR, GPIO0, GPIO1, GPIO2, COUNT, NONE = 0xFF }`, the index
  into each model's table. ADR is first so TAS5805M lists one row. New pins are added just
  before `COUNT` so the existing tables keep their positions.
- `struct FaultOutputInfo { select_register; output_mask; }`. `select_register == 0` means the
  model does not have that pin (`{}` or zero-filled tail).
- `ModelInfo` gains `fault_output_ctrl_register` and `fault_output_function` (per model, as
  they could differ on another chip) and `fault_outputs[FAULT_OUTPUT_COUNT]`.
- `model_*.cpp`: the fault pin lines are removed from `STARTUP_SEQUENCE`. TAS5805M table:
  `{0x61, 0x01}` (ADR), rest zero-filled. TAS5825M: `{}` (ADR), `{0x61,0x01}`, `{0x62,0x02}`,
  `{0x63,0x04}`. Both: ctrl `0x60`, function `0x0B`.
- `init_()`: after the startup sequence, if not NONE, write the select register with the
  function, then the ctrl register with the mask. A pin the model lacks (only reachable from
  C++) logs a warning. `dump_config` prints "Fault Output".
- Python: `FAULT_OUTPUTS` maps names to the C++ enumerators (matched by name; codegen emits
  `set_fault_output(tas58xx::FAULT_OUTPUT_GPIO1)`, values come from C++). `Model` gains
  `fault_outputs` (allowed names) and `default_fault_output`; `typed_schema` gives each model
  only its names.
- Known costs: one `{}` row for chips without ADR; a chip whose pins do not start at GPIO0
  (for example only GPIO3/GPIO4) would need leading `{}` rows. The ctrl register is per model;
  if a chip ever has enable bits in more than one register, move it into `FaultOutputInfo`.
  The ctrl register is written plainly (not read-modify-write), which is right while only one
  pin is an output. `ModelInfo` is in RAM on ESP8266 (+10 bytes, selected model only).

**Datasheet facts (TAS5805M SLASEH5D §7.6.1.24–25, TAS5825M SLASEH7H §9.6.1.33–36):**
- TAS5805M: `0x60` ADR_PIN_CTRL bit 0 ADR_OE; `0x61` ADR_PIN_CONFIG, `01011` = FAULTZ. No GPIO
  pins.
- TAS5825M: `0x60` GPIO_CTRL bits 0–2 = GPIO0_OE..GPIO2_OE (bits 7–3 reserved); `0x61`/`0x62`/
  `0x63` GPIO0_SEL/GPIO1_SEL/GPIO2_SEL, `1011` = FAULTZ, `1000` = WARNZ. ADR is an analog input
  (address resistor) only. `0x64` is GPIO_INPUT_SEL, so select registers are not assumed to
  continue past `0x63`.

**Checks run (all pass):** `script/test_build_components.py -e config -c tas58xx` (4 configs);
invalid pins rejected (`gpio1` on tas5805m, `adr` on tas5825m, `gpio3`); defaults generate
`FAULT_OUTPUT_ADR` / `FAULT_OUTPUT_GPIO1`; clang-tidy and clang++/g++ `-Wall -Wextra` on the
tas58xx files (no tas58xx diagnostics, negative control fails as expected); clang-format
13.0.1, ruff 0.16.10, flake8 and `script/ci-custom.py` clean. Not done: a real target build
and hardware test.

**Before raising the PR:** hardware test the TAS5825M GPIO pins if possible, use the
`.github/PULL_REQUEST_TEMPLATE.md` template with title `[tas58xx] Add fault_output option`, and
add the docs entry to `esphome/esphome.io`.

## How the work was checked, and how to repeat it locally
- **`main`-compatibility matrix:** `docs/compat/cases/*.yaml` (18 configs) and
  `docs/compat/run.sh`. It needs `esphome` on PATH; use 2026.10.0+ for this branch.
  ```
  docs/compat/run.sh                                     # this branch
  git worktree add /tmp/tas58xx-main main
  docs/compat/run.sh /tmp/tas58xx-main/components        # main, for comparison
  ```
  Expected on this branch: everything VALID (some with warnings) except
  `17_main_clock_fault_space_key` (the accepted break) and `18_both_pcdd_and_pvdd_keys`
  (deliberate error). On `main`, 03/16/18 fail and the rest are valid.
- **ESPHome lint (ci-custom):** clone `esphome/esphome`, copy `components/tas58xx` to
  `esphome/components/tas58xx` (without `Example YAML`), then run
  `python script/ci-custom.py $(find esphome/components/tas58xx -type f)`.
  Expected: 0 findings, apart from the `.flac` files on `dev_update`.
- **C++ check:** `docs/tools/cpp_check.sh <esphome-clone> [arduinojson-src]`. It runs a
  clang-tidy host-header parse against ESPHome headers (beta or dev), for these define sets:
  - `USE_TAS5805M_DAC`
  - `USE_TAS5825M_DAC`
  - `USE_TAS58XX_EQ_GAINS`
  - `USE_TAS58XX_EQ_BIAMP`
  - `USE_TAS58XX_EQ_PRESETS`
  - `USE_TAS58XX_CHANNEL_VOLUMES`
  - `USE_TAS58XX_BINARY_SENSOR`

  It needs ArduinoJson on the include path. **Remove every `USE_LVGL` line from the ESPHome
  copy's `esphome/core/defines.h`** (or install LVGL): `tas58xx.cpp`'s includes otherwise reach
  `lvgl.h`, clang stops with a fatal "file not found", and the function bodies are never
  checked even though no tas58xx errors are shown. Check the output has no "file not found".
  A real ESP32 build (`esphome compile`) was not
  possible in the cloud session because the PlatformIO registry was blocked; locally,
  `esphome compile` on an example YAML is the better check.
- **Examples:** `esphome config` on each file in `components/tas58xx/Example YAML/`. Point
  `external_components` at a local path for testing.


## Helper and DSP write rework (2026-10-14, after the EQ mode work)
- `db3b8a1` helper cleanup: only `make_biquad`, `gain_to_fixed_9_23` and `ALL_PASS` are public; shared shelf and
  pass filter code; 5.27 conversion without int64; coefficients identical bit for bit
  (`docs/tools/biquad_check/run.sh`, 642194 biquads, md5 compared with the previous helpers).
- `2749ff8` TAS5825M Gang EQ and Bypass EQ flags sent as 00 00 00 01 (big endian value 1), was 01 00 00 00.
- `23bfaa1` + `7801822` DSP write path: `dsp_write_bytes_` replaces `book_page_write_bytes_` and `biquad_write_bytes_`. It writes any
  length from book, page and subaddress, continues at subaddress 0x08 on the next page, sends each page segment in
  one `I2CDevice::write()` of the subaddress followed by up to 120 data bytes (from a 121 byte stack buffer, no heap;
  `write_register` has a 17 byte buffer and a 20 byte biquad write used the heap) and always resets to book 0 page 0,
  also after an error. Consecutive gain bands and the 3 preset biquads are written as one run: a full 15 band
  channel takes about 12 I2C transactions instead of about 108. EQ mode registers are calculated from
  `eq_mode_is_ganged()` and EQ Off with named DSP_MISC bits instead of two 5 entry tables.
- `7801822` The `tas58xx_write_byte_`, `tas58xx_write_bytes_` and `tas58xx_read_bytes_` wrappers are gone, the component
  uses the `I2CDevice` functions (`write_byte`, `read_byte`, `read_bytes`, `write`) as ESPHome's tas58xx does. The
  wrapper logged every failure with its error code and register; now each call site logs its own failure, so every
  failed I2C read or write logs one error naming what was being read or written (DSP writes also log the I2C error
  code, book, page and address; book/page selection logs book and page). `i2c_error_` is removed. The old read
  wrapper also sent the register address twice (write, then `read_register`), now it is one write-read.
  Checked: `docs/tools/dsp_write_check/run.sh` (480 runs, same DSP memory as the old writer on both chips);
  biquad addresses checked against the TI memory maps (SLOA263A TAS5805M: 150 coefficients, 5 biquads split over
  two pages; SLAA786A TAS5825M BasePro 2.0 96k: no biquad split over two pages), no mismatch.
  Hardware: EQ writes at startup and on gain changes behave as before, no I2C errors in the log (TAS5805M has
  biquads split over two pages), and the Power State read in `is_dac_playing_` still works (now one write-read).
- EQ mode changes (`62d138f`) write only what changes: `eq_filters_written_` per channel and `eq_channel_need()`.
  Logs: "<Left|Right> Channel EQ >> flat" for each channel set flat, "EQ Ganged >> On|Off" when ganging changes.

- Helpers tidy (`fbdbc6b`, `12b34f0`, `0653923`): the verbose coefficient logs (5.27 and 9.23), log tag and log include
  are removed (proven by the host tools). The helpers are in the component namespace `esphome::tas58xx` (no more
  `tas58xx_helpers::` prefix; the files `tas58xx_helpers.h/.cpp` stay). File local functions are `static` (ESPHome's
  usual style, not an anonymous namespace); constants used by one function live inside it, shared ones
  (`TWO_PI`, `INVERSE_SQRT2`, `LN10_DIV_*`) at file scope.

## Two DACs of the same type on one board (goal, 2026-10-14)
- Wanted later: two `audio_dac: - platform: tas58xx` entries of the same model with different I2C addresses.
- Fixed (`20c886b`): the mixer select's option offset was a file global shared by all mixer selects (a BTL and a
  PBTL DAC would break each other); it is now a member `min_mixer_mode_`. `MAX_SELECT_INDEX` (unused) removed.
- Already fine: helpers are pure functions; per DAC data is keyed by the DAC id (EQ modes in `CORE.data`, matching
  number/select lookups) or is component state; `USE_TAS58XX_*` defines only say what is compiled in.
- Not possible by design: two different models on one build (`USE_TAS5805M_DAC` / `USE_TAS5825M_DAC`).
- Still to check when it is done: test YAML with two DACs (ids, numbers and selects for each, preferences per
  entity), and that nothing else is file global (a search found only the mixer select globals).

## Mixer values checked (2026-10-14, no change)
- Mixer registers are 9.23 big endian (TAS5805M book 0x8C page 0x29 from 0x18, TAS5825M book 0x8C page 0x0B from
  0x14, order left to left, right to left, left to right, right to right). Our constants are stored byte swapped:
  `0x00008000` = 1.0 (0 dB), `0x00004000` = 0.5 (-6.02 dB), 0 = mute. Same bytes, addresses and mode table as
  ESPHome's tas58xx (`write_mixer_`, model files).
- MONO uses exactly 0.5 per input (true average, a full scale mono signal cannot overflow). `gain_to_fixed_9_23(-6)`
  would give 0x004026E7 (0.501187, +0.02 dB over full scale on identical inputs). Decision: keep 0.5. If it is ever
  changed, the stored constant is the byte swapped 0xE7264000. Not known what PPC3 writes for "-6 dB" in its mixer
  (SLOA263A only says the GUI takes dB); a PPC3 export with the mixer set to mono would show it.
- MONO sounds the same as STEREO for centre panned material; test LEFT, RIGHT and STEREO INVERSE with a stereo
  test signal. The log shows "Input Mixer Mode >> ..." when the mixer is written.

## Helpers against the TI PurePath Console reference (2026-10-14)
`docs/tools/ppc_check/run.sh` compares the helpers with `ppc_reference.cfg` (PPC3 export at 96 kHz: 50, 315, 800,
1250, 3150 and 8000 Hz, 6 filter types, 180 coefficients; Band Pass and Notch are not helpers).
- Helpers as they are (Q stored as a float): 137 of 180 exact, 43 differ, largest 3 LSB (at 8000 Hz). The host
  build (x86, IEEE double) gives exactly the same 43 values as the ESP32 run in
  `ESP32_vs_PPC_Fixed527_Differences.xlsx`, so the differences are not ESP32 software double rounding.
- Cause of most: the Q is stored as a float. 0.9f is 0.89999998..., not 0.9, and PPC uses the decimal. With Q passed
  as a double: 164 of 180 exact, largest 1 LSB. The 2 and 3 LSB differences at 8000 Hz disappear.
- Left (16 coefficients, all 1 LSB): low and high pass (Q is fixed there), plus single values of the other types. In
  every pass filter case the unrounded value is 0.52 to 0.61 above a whole number and PPC rounds down, so PPC has
  a slightly different intermediate calculation. Not explained by pi precision, division instead of multiplying by
  the inverse, or rounding mode (nearest is best).
- Done (`f274aeb`): Q is a `double` in `EqBand` and in the helpers (`make_biquad(..., double q_factor)`, default
  table and preset shelf Q as double literals; `EqBand` 16 bytes instead of 8, flash only). A YAML Q (a Python float,
  the same IEEE double) reaches the helpers unchanged. Thousandths would not hold a Q such as 2.144908; millionths in a
  `uint32_t` would (8 byte `EqBand`, one division per biquad) if size ever matters.

  | Filter (30 coefficients each) | Q float: different (largest) | Q double: different (largest) |
  |---|---|---|
  | Equalizer (Q) | 14 (3) | 5 (1) |
  | Peaking EQ | 8 (3) | 1 (1) |
  | Low Shelf | 8 (3) | 2 (1) |
  | High Shelf | 6 (3) | 1 (1) |
  | Low Pass BW2 | 2 (1) | 2 (1) |
  | High Pass BW2 | 5 (1) | 5 (1) |
  | **Total** | **43 of 180 (3)** | **16 of 180 (1)** |

  Low Shelf 315 Hz b1 and Low and High Shelf 8000 Hz a2 matched before only by luck (the float error pushed them the
  right way), now they are 1 LSB off. `docs/tools/biquad_check` md5 is unchanged (it feeds float Q values, which
  convert to double exactly, so the arithmetic itself did not change).
- The ESP32 vs PPC spreadsheet (`ESP32_vs_PPC_Fixed527_Differences.xlsx`) is not in the repository; the PPC cfg is
  (`docs/tools/ppc_check/ppc_reference.cfg`).

## Open items for next time (2026-10-14)
- Hardware test the checklist above on `dev_update_with_onaudio_plus_eq` (pushed). If it passes,
  decide whether to merge it into `dev_update_with_onaudio`.
- Legacy comments left in place for you to decide: the commented subwoofer/crossbar enums, text tables and
  `USE_SPEAKER_CONFIG` constants in `tas58xx_defs.h` (now labelled "Future speaker configuration, not used yet"),
  and three commented `ESP_LOGD` debug lines in `tas58xx.cpp`.
- Per band filter YAML (nested `left_eq_band_N`) is still deferred (see "Future per band filters"). The sand_box
  schema, filter map (High Pass / Low Pass swap corrected) and to_code notes are now comments in
  `number/__init__.py` (`2220468`). When it is done: Q goes into the generated `EqBand` table as a double literal,
  add a range check for Q (Q = 0 divides by zero), frequency below 48 kHz, gain range is ±15 dB here
  (`TAS58XX_EQ_MAX_DB`) while sand_box allowed ±24 dB, and a band defined in both forms is an error.
- Hardware checks for today's rework (also: mixer select still maps its options correctly in BTL and PBTL): no I2C errors in the log at startup and on EQ changes (TAS5805M biquads that
  cross a page), EQ mode switching writes only what changes, TAS5825M EQ Off bypasses and EQ On Right=Left gangs
  (flag byte order), "DAC not playing" / audio start behaviour (Power State read is now one write-read).
- Upstream PRs for `add_fault_output` and `add_modulation_option` (branches in `mrtoy-me/esphome`) are not raised;
  each needs an esphome.io docs entry. You delete `dev_update_with_onaudio_split` and `claude/jolly-ramanujan-7zxbba`.
- Done 2026-10-16 (TAS5825M PPC dumps checked, 0x46 = 0x11 confirmed, startup comments updated): check 0x46 (expect 0x11, DSP_CTRL §9.6.1.16: 96 kHz processing, default
  coefficients), DSP_MISC/Gang EQ and Bypass EQ flag bytes (expect 00 00 00 01 for true), mixer bytes at book 0x8C
  page 0x0B from 0x14, and the biquads with `docs/tools/ppc_check`. The TAS5805M 50 Hz dump (2026-10-15) matched:
  Equalizer Q 0.71 exact, High Pass exact, Low Shelf a1 1 LSB; it writes DSP_MISC 0x86 (bit 7 reserved) where we
  write 0x06. The startup comment "0x46 not documented" is only true for the TAS5805M (same in ESPHome beta).
- Host tools added today (no hardware needed): `docs/tools/biquad_check/run.sh` (helpers bit for bit),
  `docs/tools/dsp_write_check/run.sh` (DSP memory written by `dsp_write_bytes_`), `docs/tools/ppc_check/run.sh`
  (helpers against the PPC reference). They need g++ and python3 only.
- Local checks need ESPHome's Python requirements in a venv; `docs/tools/cpp_check.sh` needs a scratch esphome
  clone and ArduinoJson v7.4.2. `docs/compat/run.sh` cases 01 to 49 (17, 18, 25, 26, 28, 32, 36, 37, 38, 41, 45 to 49 are meant to fail; 35 passes with
  the rounding warning; on
  `dev_update_eq_proto` cases 06 to 29 use `left_bands` / `right_bands`, 31 to 41 test bands and fixed filters, 42 to 49 test the `gain_min` / `gain_max` band limits, -24 dB to +18 dB for every filter).
