import logging

import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
import esphome.final_validate as fv

from esphome.const import (
    DEVICE_CLASS_SOUND_PRESSURE,
    ENTITY_CATEGORY_CONFIG,
    UNIT_DECIBEL,
)

_LOGGER = logging.getLogger(__name__)

SELECT_COMPONENT = "select"
DAC_MODE = "dac_mode"
DAC_MODE_BTL = "BTL"
EQ_MODE = "eq_mode"
EQ_PRESET_LEFT_CHANNEL = "eq_preset_left_channel"

CONF_CHANNEL_VOLUME_LEFT = "channel_volume_left"
CONF_CHANNEL_VOLUME_RIGHT = "channel_volume_right"

ICON_VOLUME_SOURCE = "mdi:volume-source"

from ..audio_dac import (
    CONF_LEFT_EQ_GAINS,
    CONF_RIGHT_EQ_GAINS,
    CONF_TAS58XX_ID,
    Tas58xxComponent,
    find_matching_config,
    get_audio_dac_config,
    has_eq_gains,
    tas58xx_ns,
)

ChannelVolumeLeft = tas58xx_ns.class_("ChannelVolumeLeft", number.Number, cg.Component)
ChannelVolumeRight = tas58xx_ns.class_("ChannelVolumeRight", number.Number, cg.Component)
EqGainNumber = tas58xx_ns.class_("EqGainNumber", number.Number, cg.Component)
Channels = tas58xx_ns.enum("Channels")

# the gain keys are in band order, so a key's position in the tuple is its band index
EQ_GAIN_CHANNELS = (
    (Channels.LEFT_CHANNEL, CONF_LEFT_EQ_GAINS, "USE_TAS58XX_EQ_GAINS"),
    (Channels.RIGHT_CHANNEL, CONF_RIGHT_EQ_GAINS, "USE_TAS58XX_EQ_BIAMP"),
)


def validate_eq_gain_numbers(config):
    have_left_gains = [key in config for key in CONF_LEFT_EQ_GAINS]
    have_right_gains = [key in config for key in CONF_RIGHT_EQ_GAINS]

    if any(have_left_gains) and not all(have_left_gains):
        raise cv.Invalid("All 15 Left EQ Gain numbers must be configured")
    if any(have_right_gains) and not all(have_right_gains):
        raise cv.Invalid("All 15 Right EQ Gain numbers must be configured")
    if all(have_right_gains) and not all(have_left_gains):
        raise cv.Invalid("When Right EQ Gain Numbers are configured, all 15 Left EQ Gain numbers must also be configured")
    return config


def _final_validate(config):
    full_conf = fv.full_config.get()

    this_number_id = config[CONF_TAS58XX_ID]
    have_this_number_eq_gains = has_eq_gains(config, CONF_LEFT_EQ_GAINS + CONF_RIGHT_EQ_GAINS)

    if have_this_number_eq_gains:
        # the tas58xx select config with the same audio_dac ID as this number
        select_conf = find_matching_config(full_conf, this_number_id, SELECT_COMPONENT)
        have_select_eq_mode = select_conf is not None and EQ_MODE in select_conf
        have_select_eq_preset = select_conf is not None and EQ_PRESET_LEFT_CHANNEL in select_conf

        # have_this_number_eq_gains and
        if (not have_select_eq_mode):
            raise cv.Invalid("Select eq_mode is required with Left EQ Gain numbers - add Select eq_mode to YAML configuration")

        # have_this_number_eq_gains and
        if (have_select_eq_preset):
            raise cv.Invalid("EQ Gain numbers are not allowed with Select eq_presets - remove one set of those configurations")


    # the audio_dac config declaring the ID used by this number
    matching_audio_dac = get_audio_dac_config(full_conf, this_number_id)
    if matching_audio_dac is not None:
        is_dac_mode_btl = matching_audio_dac.get(DAC_MODE) == DAC_MODE_BTL

        have_this_number_channel_volume_left = CONF_CHANNEL_VOLUME_LEFT in config
        have_this_number_channel_volume_right = CONF_CHANNEL_VOLUME_RIGHT in config

        if is_dac_mode_btl:
            if (have_this_number_channel_volume_left and not have_this_number_channel_volume_right):
                raise cv.Invalid("channel_volume_right is required with channel_volume_left - add channel_volume_right to YAML configuration")

            if (have_this_number_channel_volume_right and not have_this_number_channel_volume_left):
                raise cv.Invalid("channel_volume_left is required with channel_volume_right - add channel_volume_left to YAML configuration")
        else:
            if (have_this_number_channel_volume_right):
                # warn rather than fail, so YAML that was valid in earlier releases still validates
                _LOGGER.warning("channel_volume_right is not used when dac_mode is PBTL - it can be removed from YAML configuration")

    return config

FINAL_VALIDATE_SCHEMA = _final_validate


def _eq_gain_schema():
    return number.number_schema(
        EqGainNumber,
        device_class=DEVICE_CLASS_SOUND_PRESSURE,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_VOLUME_SOURCE,
        unit_of_measurement=UNIT_DECIBEL,
    ).extend(cv.COMPONENT_SCHEMA)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_TAS58XX_ID): cv.use_id(Tas58xxComponent),

        cv.Optional(CONF_CHANNEL_VOLUME_LEFT): number.number_schema(
            ChannelVolumeLeft,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_CHANNEL_VOLUME_RIGHT): number.number_schema(
            ChannelVolumeRight,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        **{cv.Optional(key): _eq_gain_schema() for key in CONF_LEFT_EQ_GAINS + CONF_RIGHT_EQ_GAINS},
    }
).add_extra(validate_eq_gain_numbers)

async def to_code(config):
    tas58xx_component = await cg.get_variable(config[CONF_TAS58XX_ID])

    if channel_volume_left_config := config.get(CONF_CHANNEL_VOLUME_LEFT):
        cg.add_define("USE_TAS58XX_CHANNEL_VOLUMES")
        n = await number.new_number(
           channel_volume_left_config, min_value=-24, max_value=24, step=1
        )
        await cg.register_component(n, channel_volume_left_config)
        await cg.register_parented(n, tas58xx_component)

    if channel_volume_right_config := config.get(CONF_CHANNEL_VOLUME_RIGHT):
        n = await number.new_number(
           channel_volume_right_config, min_value=-24, max_value=24, step=1
        )
        await cg.register_component(n, channel_volume_right_config)
        await cg.register_parented(n, tas58xx_component)

    # left gains, then right gains (EQ BiAMP)
    for channel, gain_keys, define in EQ_GAIN_CHANNELS:
        if any(key in config for key in gain_keys):
            cg.add_define(define)
        for band, key in enumerate(gain_keys):
            if gain_config := config.get(key):
                n = await number.new_number(
                    gain_config, channel, band, min_value=-15, max_value=15, step=1
                )
                await cg.register_component(n, gain_config)
                await cg.register_parented(n, tas58xx_component)
