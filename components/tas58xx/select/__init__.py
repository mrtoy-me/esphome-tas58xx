import esphome.codegen as cg
from esphome.components import select
from esphome.core import CORE
import esphome.config_validation as cv
import esphome.final_validate as fv

from esphome.const import (
  CONF_AUDIO_DAC,
  CONF_ID,
  CONF_NUMBER,
  CONF_PLATFORM,
  ENTITY_CATEGORY_CONFIG,
)

from ..audio_dac import (
  CONF_MIXER_MODE,
  PLATFORM_TAS58XX,
  DAC_MODE_BTL,
  CONF_DAC_MODE,
  CONF_EQ_MODE,
  CONF_EQ_PRESET_LEFT_CHANNEL,
  CONF_EQ_PRESET_RIGHT_CHANNEL,
  CONF_LEFT_EQ_BANDS,
  CONF_RIGHT_EQ_BANDS,
  KEY_SELECT_EQ,
  KEY_LEFT_EQ_PRESET,
  KEY_RIGHT_EQ_PRESET,
)

from ..audio_dac import find_matching_config

from ..audio_dac import CONF_TAS58XX_ID, Tas58xxComponent, tas58xx_ns

EqModeSelect = tas58xx_ns.class_("EqModeSelect", select.Select, cg.Component)
MixerModeSelect = tas58xx_ns.class_("MixerModeSelect", select.Select, cg.Component)
EqPresetLeftSelect = tas58xx_ns.class_("EqPresetLeftSelect", select.Select, cg.Component)
EqPresetRightSelect = tas58xx_ns.class_("EqPresetRightSelect", select.Select, cg.Component)

# CONF_EQ_MODE = "eq_mode"
# CONF_MIXER_MODE = "mixer_mode"
# CONF_EQ_PRESET_LEFT_CHANNEL = "eq_preset_left_channel"
# CONF_EQ_PRESET_RIGHT_CHANNEL = "eq_preset_right_channel"
# CONF_LEFT_EQ_BANDS = tuple(f"left_eq_band_{i}" for i in range(1, 16))
# CONF_RIGHT_EQ_BANDS = tuple(f"right_eq_band_{i}" for i in range(1, 16))

# PLATFORM_TAS58XX = "tas58xx"
# DAC_MODE = "dac_mode"
# DAC_MODE_BTL = "BTL"
LEFT_EQ_GAIN_20HZ = "left_eq_band_1"

def validate_eq_presets(config):
    have_select_eq_mode = CONF_EQ_MODE in config
    have_select_eq_preset_left = CONF_EQ_PRESET_LEFT_CHANNEL in config
    have_select_eq_preset_right = CONF_EQ_PRESET_RIGHT_CHANNEL in config

    if not have_select_eq_mode and (have_select_eq_preset_left or have_select_eq_preset_right):
         raise cv.Invalid("Select eq_mode is required with eq_presets - add Select eq_mode to YAML configuration")

    return config

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_TAS58XX_ID): cv.use_id(Tas58xxComponent),
        cv.Optional(CONF_EQ_MODE): select.select_schema(
            EqModeSelect,
            entity_category=ENTITY_CATEGORY_CONFIG,
         ),
        cv.Optional(CONF_MIXER_MODE): select.select_schema(
            MixerModeSelect,
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
        cv.Optional(CONF_EQ_PRESET_LEFT_CHANNEL): select.select_schema(
            EqPresetLeftSelect,
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
        cv.Optional(CONF_EQ_PRESET_RIGHT_CHANNEL): select.select_schema(
            EqPresetRightSelect,
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
    }
).add_extra(validate_eq_presets)

def find_matching_config(full_conf, dac_id, CONF_NUMBER):
    for select_conf in full_conf.get(CONF_NUMBER, []):
        if select_conf.get(CONF_PLATFORM) != PLATFORM_TAS58XX:
            continue
        if select_conf.get(CONF_TAS58XX_ID) == dac_id:
            return select_conf
    return None


def _final_validate(config):
    full_conf = fv.full_config.get()

    this_select_id = config[CONF_TAS58XX_ID]
    have_eq_mode = CONF_EQ_MODE in config
    have_left_eq_preset = CONF_EQ_PRESET_LEFT_CHANNEL in config
    have_right_eq_preset = CONF_EQ_PRESET_RIGHT_CHANNEL in config

    # find the number ID that matches this select ID and flag if the number configuration has LEFT_EQ_GAIN_20HZ
    # have_number_left_eq_gain = False
    # number_confs = full_conf.get(CONF_NUMBER, [])
    # for number_conf in number_confs:
    #     if number_conf.get(CONF_PLATFORM) == PLATFORM_TAS58XX:
    #       if number_conf.get(CONF_TAS58XX_ID) == this_select_id:
    #           have_number_left_eq_gain = LEFT_EQ_GAIN_20HZ in number_conf
    #           break

    matching_number_conf = find_matching_config(full_conf, this_select_id, CONF_NUMBER)
    have_left_eq_gains = any(k in matching_number_conf for k in CONF_LEFT_EQ_BANDS)
    have_right_eq_gains = any(k in matching_number_conf for k in CONF_RIGHT_EQ_BANDS)

    # if have_number_left_eq_gain:
        # have_number_left_eq_gain and
    if have_left_eq_gains:
        if have_left_eq_preset or have_right_eq_preset:
            raise cv.Invalid("Select eq_presets are not allowed with EQ Gain numbers - remove one set of those configurations")

        # have_number_left_eq_gain and
        if not have_eq_mode:
            raise cv.Invalid("Select eq_mode is required with EQ Gain numbers - add Select eq_mode to YAML configuration")

    # audio_dac_id_matches_select_id = False
    # matching_audio_dac = None
    # # find the audic dac ID that matches the select ID
    # all_audio_dac = full_conf.get(CONF_AUDIO_DAC, [])
    # for audio_dac_conf in all_audio_dac:
    #    if audio_dac_conf.get(CONF_PLATFORM) == PLATFORM_TAS58XX:
    #        if this_select_id == audio_dac_conf.get(CONF_ID):
    #           audio_dac_id_matches_select_id = True
    #           matching_audio_dac = audio_dac_conf
    #           break
    try:
        dac_path = full_conf.get_path_for_id(this_select_id)[:-1]
        matching_audio_dac = full_conf.get_config_for_path(dac_path)
    except KeyError:
        raise cv.Invalid("YAML validation error - no audio_dac with same id as number")

    is_dac_mode_btl = matching_audio_dac.get(CONF_DAC_MODE) == DAC_MODE_BTL
    if is_dac_mode_btl:
        if have_left_eq_preset and not have_right_eq_preset:
            raise cv.Invalid("Select eq_preset_right is required with eq_preset_left - add Select eq_preset_right to YAML configuration")
        if have_right_eq_preset and not have_left_eq_preset:
            raise cv.Invalid("Select eq_preset_left is required with eq_preset_right - add Select eq_preset_left to YAML configuration")

    if not is_dac_mode_btl:
        if have_right_eq_preset:
            raise cv.Invalid("Select eq_preset_right is not required when dac_mode is PBTL - remove Select eq_preset_right from YAML configuration")

    # wait to validate until after other validations
    if have_eq_mode and (not have_left_eq_gains) and (not have_left_eq_preset):
        raise cv.Invalid("Select eq_mode applies only when Select EQ Presets or EQ Gain numbers are configured - remove Select eq_mode from YAML configuration")

    CORE.data.setdefault(KEY_SELECT_EQ, {})[config[CONF_TAS58XX_ID]] = {
            KEY_LEFT_EQ_PRESET: have_left_eq_preset,
            KEY_RIGHT_EQ_PRESET: have_right_eq_preset,
        }
    return config

FINAL_VALIDATE_SCHEMA = _final_validate

async def to_code(config):
    tas58xx_component = await cg.get_variable(config[CONF_TAS58XX_ID])
    if eq_mode_config := config.get(CONF_EQ_MODE):
        s = await select.new_select(
            eq_mode_config,
            options=[],
        )
        await cg.register_component(s, eq_mode_config)
        await cg.register_parented(s, tas58xx_component)

    if mixer_mode_config := config.get(CONF_MIXER_MODE):
        s = await select.new_select(
            mixer_mode_config,
            options=[],
        )
        await cg.register_component(s, mixer_mode_config)
        await cg.register_parented(s, tas58xx_component)

    if eq_preset_left_config := config.get(CONF_EQ_PRESET_LEFT_CHANNEL):
        cg.add_define("USE_TAS58XX_EQ_PRESETS")
        s = await select.new_select(
            eq_preset_left_config,
            options=[],
        )
        await cg.register_component(s, eq_preset_left_config)
        await cg.register_parented(s, tas58xx_component)

    if eq_preset_right_config := config.get(CONF_EQ_PRESET_RIGHT_CHANNEL):
        s = await select.new_select(
            eq_preset_right_config,
            options=[],
        )
        await cg.register_component(s, eq_preset_right_config)
        await cg.register_parented(s, tas58xx_component)
