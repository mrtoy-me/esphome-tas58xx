# Development notes: on_audio_started rework and path to esphome/tas58xx

Notes from the session of 2026-10-10 so work can continue in a later session, either locally
or in the cloud. Read this first. It covers the goal, the requirements set so far, the current
branch state, the design, a to-check list and the plan forward.

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

### Activation of DSP settings (`dev_update_with_onaudio` and `_split`)
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
- Single `EqGainNumber` class instead of 30 per-band classes: probably soon, but **not**
  before hardware testing.

### Git
- `dev_update`: tidied loop design (lint fixes, `main`-compatible YAML). Requires 2026.2.0.
  Not changed by the on_audio_started work.
- `dev_update_with_onaudio_split`: the on_audio_started design, the branch to hardware test.
  Requires ESPHome 2026.10.0.

## Branch state (2026-10-11)
| Branch | Contents |
|---|---|
| `main`, `beta`, `dev` | unchanged by this work (loop design) |
| `dev_update` | loop design. Lint fixes (ci-custom clean), example YAML fixes, `main`-compatible YAML (warnings instead of errors, `pcdd_` aliases), README key and modulation fixes. Requires 2026.2.0. |
| `dev_update_with_onaudio` | earlier stage of the on_audio_started design, before the setter split, the startup delay removal and the EQ Mode select change |
| `dev_update_with_onaudio_split` | the on_audio_started design described below, for hardware testing. Requires 2026.10.0. |
| `claude/jolly-ramanujan-7zxbba` | fully contained in `dev_update`; can be deleted |

## Design on `dev_update_with_onaudio_split`
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
   channel volumes were never written to the DAC) is fixed on `dev_update_with_onaudio_split`.
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
5. **After testing:** decide how `dev_update_with_onaudio_split` is brought into `dev_update`
   or `beta`. The two branches differ by one equivalent README commit (`2257e2f` on
   `dev_update`, `aef0fd6` on the split branch), so a merge is clean.
6. **Other selects:** the mixer mode and EQ preset selects still build their options at
   runtime. Both could pass `options=` from codegen like the EQ Mode select; the mixer select
   also uses two non-const globals (`MAX_SELECT_INDEX`, `MIN_MIXER_MODE`).
7. **EQ Mode select state is not restored after a reboot** (it starts at the configured mode).
   Decide whether it should be.

## Plan forward
1. **Hardware test `dev_update_with_onaudio_split`.**
   - Speaker media player: AUTO, presets, 15-band and biamp configs.
   - snapclient.
   - Changes while idle, and the `enable_dac` off/on case.
   - Also with the speaker's default `timeout` (remove `timeout: never`).
2. **Fix anything found, then decide on point 1 and point 5 of the to-check list.**
3. **Refactor the 30 EQ gain number classes into one parameterised class.** YAML keys stay
   the same.
4. **Bring in the separate EQ redesign branch** (lowercase keys, optional gains, and so on).
5. **Upstream PRs to `esphome/tas58xx`,** small and targeted. A possible order:
   1. channel volume numbers
   2. mixer mode select
   3. EQ mode plus 15-band gains
   4. biamp
   5. presets

   Each PR reuses beta's activation mechanism plus the `can_write_dsp_()` rule, and needs:
   - upstream structure (`ModelInfo`, EQ addresses per model)
   - lowercase keys
   - a single gain class
   - an EQ restriction on ESP8266/RP2040
   - clang-format, ruff and clang-tidy clean

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
- **C++ check:** a clang-tidy host-header parse against ESPHome **beta** headers, run for these
  define sets:
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
