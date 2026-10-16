# Development notes: on_audio_started rework and path to esphome/tas58xx

Notes from the session of 2026-10-10 so work can continue in a later session, either locally
or in the cloud. Read this first. It covers the goal, the requirements set so far, the current
branch state, the design, a to-check list and the plan forward.

## Where we stopped (2026-10-14)
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
5. **Upstream PRs to `esphome/tas58xx`,** small and targeted. A possible order:
   1. channel volume numbers
   2. mixer mode select
   3. EQ mode plus 15-band gains
   4. biamp
   5. presets
   6. register 0x46 comments in `model_tas5805m.cpp` and `model_tas5825m.cpp`: both say 0x46 is not
      documented, which is only true for TAS5805M. TAS5825M 0x46 is DSP_CTRL (datasheet 9.6.1.16), 0x11 =
      96 kHz processing with default coefficients. Checked against PPC 2.0 dumps at 96 and 48 kHz (2026-10-16);
      the esphome-tas58xx startup tables already carry the corrected comments.

   Each PR reuses beta's activation mechanism plus the `can_write_dsp_()` rule, and needs:
   - upstream structure (`ModelInfo`, EQ addresses per model)
   - lowercase keys
   - a single gain class
   - an EQ restriction on ESP8266/RP2040
   - clang-format, ruff and clang-tidy clean

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
