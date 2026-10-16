import logging

import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.const import CONF_NUMBER, ENTITY_CATEGORY_CONFIG

_LOGGER = logging.getLogger(__name__)

from ..audio_dac import (
    CONF_DAC_MODE,
    CONF_EQ_MODE,
    CONF_MIXER_MODE,
    CONF_TAS58XX_ID,
    DAC_MODE_BTL,
    EQ_MODE_OPTIONS,
    CONF_EQ_PRESET_LEFT_CHANNEL,
    CONF_EQ_PRESET_RIGHT_CHANNEL,
    Tas58xxComponent,
    find_matching_config,
    get_audio_dac_config,
    get_eq_channels,
    get_eq_modes,
    has_eq,
    tas58xx_ns,
)

EqModeSelect = tas58xx_ns.class_("EqModeSelect", select.Select, cg.Component)
MixerModeSelect = tas58xx_ns.class_("MixerModeSelect", select.Select, cg.Component)
EqPresetLeftSelect = tas58xx_ns.class_("EqPresetLeftSelect", select.Select, cg.Component)
EqPresetRightSelect = tas58xx_ns.class_("EqPresetRightSelect", select.Select, cg.Component)

def validate_eq_presets(config):
    has_preset = CONF_EQ_PRESET_LEFT_CHANNEL in config or CONF_EQ_PRESET_RIGHT_CHANNEL in config
    if has_preset and CONF_EQ_MODE not in config:
        raise cv.Invalid("Select eq_mode is required with eq_presets - add Select eq_mode to YAML configuration")
    return config


def _select_schema(select_class):
    return select.select_schema(select_class, entity_category=ENTITY_CATEGORY_CONFIG)


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_TAS58XX_ID): cv.use_id(Tas58xxComponent),
        cv.Optional(CONF_EQ_MODE): _select_schema(EqModeSelect),
        cv.Optional(CONF_MIXER_MODE): _select_schema(MixerModeSelect),
        cv.Optional(CONF_EQ_PRESET_LEFT_CHANNEL): _select_schema(EqPresetLeftSelect),
        cv.Optional(CONF_EQ_PRESET_RIGHT_CHANNEL): _select_schema(EqPresetRightSelect),
    }
).add_extra(validate_eq_presets)


def _final_validate(config):
    full_conf = fv.full_config.get()
    audio_dac_id = config[CONF_TAS58XX_ID]

    # the tas58xx number and audio_dac configs with the same audio_dac ID as this select
    # EQ checks (eq_mode required, bands with presets, biquad count) are done by the audio_dac final validation
    number_conf = find_matching_config(full_conf, audio_dac_id, CONF_NUMBER)
    audio_dac_conf = get_audio_dac_config(full_conf, audio_dac_id)
    eq_channels = get_eq_channels(number_conf, config, audio_dac_conf)

    # warn rather than fail, so YAML that was valid in earlier releases still validates
    if audio_dac_conf is not None:
        if audio_dac_conf.get(CONF_DAC_MODE) != DAC_MODE_BTL and eq_channels.right_preset:
            _LOGGER.warning("Select eq_preset_right_channel is not used when dac_mode is PBTL - it can be removed from YAML configuration")

    if CONF_EQ_MODE in config and not has_eq(eq_channels):
        _LOGGER.warning("Select eq_mode only applies when EQ bands, fixed filters or EQ Presets are configured - it can be removed from YAML configuration")

    return config


FINAL_VALIDATE_SCHEMA = _final_validate


async def _new_select(config, parent, options):
    s = await select.new_select(config, options=options)
    await cg.register_component(s, config)
    await cg.register_parented(s, parent)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_TAS58XX_ID])

    if eq_mode_config := config.get(CONF_EQ_MODE):
        # EQ Off plus each EQ mode that can be selected, derived from the YAML configuration
        eq_modes = get_eq_modes(config[CONF_TAS58XX_ID])
        options = [EQ_MODE_OPTIONS[0], *(EQ_MODE_OPTIONS[mode] for mode in eq_modes)]
        await _new_select(eq_mode_config, hub, options)

    # options are filled at runtime
    if mixer_mode_config := config.get(CONF_MIXER_MODE):
        await _new_select(mixer_mode_config, hub, [])

    for key in (CONF_EQ_PRESET_LEFT_CHANNEL, CONF_EQ_PRESET_RIGHT_CHANNEL):
        if preset_config := config.get(key):
            cg.add_define("USE_TAS58XX_EQ_PRESETS")
            await _new_select(preset_config, hub, [])
