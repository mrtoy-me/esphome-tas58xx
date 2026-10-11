import logging

import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
import esphome.final_validate as fv

from esphome.const import (
  CONF_NUMBER,
  ENTITY_CATEGORY_CONFIG,
)

_LOGGER = logging.getLogger(__name__)

from ..audio_dac import (
    CONF_LEFT_EQ_GAINS,
    CONF_TAS58XX_ID,
    Tas58xxComponent,
    find_matching_config,
    get_audio_dac_config,
    get_eq_mode,
    has_eq_gains,
    tas58xx_ns,
)

EqModeSelect = tas58xx_ns.class_("EqModeSelect", select.Select, cg.Component)
MixerModeSelect = tas58xx_ns.class_("MixerModeSelect", select.Select, cg.Component)
EqPresetLeftSelect = tas58xx_ns.class_("EqPresetLeftSelect", select.Select, cg.Component)
EqPresetRightSelect = tas58xx_ns.class_("EqPresetRightSelect", select.Select, cg.Component)

CONF_EQ_MODE = "eq_mode"
CONF_MIXER_MODE = "mixer_mode"
CONF_EQ_PRESET_LEFT_CHANNEL = "eq_preset_left_channel"
CONF_EQ_PRESET_RIGHT_CHANNEL = "eq_preset_right_channel"

DAC_MODE = "dac_mode"
DAC_MODE_BTL = "BTL"

# EQ Mode select options, index matches C++ EqMode (Off, EQ 15 Band, EQ BIAMP, EQ Presets)
EQ_MODE_OPTIONS = ["Off", "EQ 15 Band", "EQ BIAMP 15 Band", "EQ Presets"]

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

def _final_validate(config):
    full_conf = fv.full_config.get()

    this_select_id = config[CONF_TAS58XX_ID]
    have_this_select_eq_mode = CONF_EQ_MODE in config
    have_this_select_eq_preset_left = CONF_EQ_PRESET_LEFT_CHANNEL in config
    have_this_select_eq_preset_right = CONF_EQ_PRESET_RIGHT_CHANNEL in config

    # the tas58xx number config with the same audio_dac ID as this select, and whether it has left EQ gains
    number_conf = find_matching_config(full_conf, this_select_id, CONF_NUMBER)
    have_number_left_eq_gain = has_eq_gains(number_conf, CONF_LEFT_EQ_GAINS)

    if have_number_left_eq_gain:
        # have_number_left_eq_gain and
        if have_this_select_eq_preset_left or have_this_select_eq_preset_right:
            raise cv.Invalid("Select eq_presets are not allowed with EQ Gain numbers - remove one set of those configurations")

        # have_number_left_eq_gain and
        if not have_this_select_eq_mode:
            raise cv.Invalid("Select eq_mode is required with EQ Gain numbers - add Select eq_mode to YAML configuration")

    # the audio_dac config declaring the ID used by this select
    matching_audio_dac = get_audio_dac_config(full_conf, this_select_id)
    if matching_audio_dac is not None:
        is_dac_mode_btl = matching_audio_dac.get(DAC_MODE) == DAC_MODE_BTL
        if is_dac_mode_btl:
            if have_this_select_eq_preset_left and not have_this_select_eq_preset_right:
                raise cv.Invalid("Select eq_preset_right is required with eq_preset_left - add Select eq_preset_right to YAML configuration")
            if have_this_select_eq_preset_right and not have_this_select_eq_preset_left:
                raise cv.Invalid("Select eq_preset_left is required with eq_preset_right - add Select eq_preset_left to YAML configuration")

        if not is_dac_mode_btl:
            if have_this_select_eq_preset_right:
                # warn rather than fail, so YAML that was valid in earlier releases still validates
                _LOGGER.warning("Select eq_preset_right is not used when dac_mode is PBTL - it can be removed from YAML configuration")

    # wait to validate until after other validations
    if have_this_select_eq_mode and (not have_number_left_eq_gain) and (not have_this_select_eq_preset_left):
        # warn rather than fail, so YAML that was valid in earlier releases still validates
        _LOGGER.warning("Select eq_mode only applies when Select EQ Presets or EQ Gain numbers are configured - it can be removed from YAML configuration")

    return config

FINAL_VALIDATE_SCHEMA = _final_validate

async def to_code(config):
    tas58xx_component = await cg.get_variable(config[CONF_TAS58XX_ID])
    if eq_mode_config := config.get(CONF_EQ_MODE):
        # "Off" plus the EQ mode derived from the YAML configuration, if any
        eq_mode = get_eq_mode(config[CONF_TAS58XX_ID])
        options = [EQ_MODE_OPTIONS[0]] + ([EQ_MODE_OPTIONS[eq_mode]] if eq_mode else [])
        s = await select.new_select(
            eq_mode_config,
            options=options,
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
