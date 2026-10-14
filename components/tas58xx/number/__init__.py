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

CONF_CHANNEL_VOLUME_LEFT = "channel_volume_left"
CONF_CHANNEL_VOLUME_RIGHT = "channel_volume_right"

ICON_VOLUME_SOURCE = "mdi:volume-source"

from ..audio_dac import (
    CONF_DAC_MODE,
    CONF_EQ_MODE,
    CONF_LEFT_EQ_GAINS,
    CONF_RIGHT_EQ_GAINS,
    CONF_TAS58XX_ID,
    DAC_MODE_BTL,
    SELECT_COMPONENT,
    Tas58xxComponent,
    find_matching_config,
    get_audio_dac_config,
    get_eq_channels,
    has_eq_gains,
    tas58xx_ns,
    validate_eq_channels,
)

ChannelVolumeLeft = tas58xx_ns.class_("ChannelVolumeLeft", number.Number, cg.Component)
ChannelVolumeRight = tas58xx_ns.class_("ChannelVolumeRight", number.Number, cg.Component)
EqGainNumber = tas58xx_ns.class_("EqGainNumber", number.Number, cg.Component)
Channels = tas58xx_ns.enum("Channels")

# the gain keys are in band order, so a key's position in the tuple is its band index
EQ_GAIN_CHANNELS = (
    (Channels.LEFT_CHANNEL, CONF_LEFT_EQ_GAINS),
    (Channels.RIGHT_CHANNEL, CONF_RIGHT_EQ_GAINS),
)

# Future (from the sand_box branch): filter type, frequency and Q of each band set in YAML, with the gain number
# nested under the band, eg left_eq_band_1: {filter_type: Low Shelf, frequency: 100, q_factor: 0.7, gain: {...}}.
# The C++ side is ready: tas58xx_helpers::FilterType and Tas58xxComponent::config_eq_bands(channel, table).
# The flat keys (left_eq_gain_20Hz) stay as an Equalizer at the default frequency and Q; a band defined in both
# forms is an error. sand_box had the High Pass and Low Pass values swapped, corrected here.
# FilterType = tas58xx_ns.enum("FilterType")
# EQ_FILTERS = {
#     "Equalizer": FilterType.FILTER_TYPE_EQUALIZER,
#     "Peaking Eq": FilterType.FILTER_TYPE_PEAKING,
#     "Low Shelf": FilterType.FILTER_TYPE_LOW_SHELF,
#     "High Shelf": FilterType.FILTER_TYPE_HIGH_SHELF,
#     "Low Pass": FilterType.FILTER_TYPE_LOW_PASS,
#     "High Pass": FilterType.FILTER_TYPE_HIGH_PASS,
# }
# CONF_FILTER_TYPE = "filter_type"
# CONF_FREQUENCY = "frequency"
# CONF_Q_FACTOR = "q_factor"
# CONF_GAIN = "gain"
#
# def _eq_band_schema():
#     return cv.Schema(
#         {
#             cv.Optional(CONF_FILTER_TYPE, default="Equalizer"): cv.enum(EQ_FILTERS, upper=False),
#             # below half the 96 kHz DSP sample rate
#             cv.Optional(CONF_FREQUENCY, default=1000): cv.All(cv.int_, cv.int_range(1, 20000)),
#             # not used by Low Pass and High Pass, their Q is fixed
#             cv.Optional(CONF_Q_FACTOR, default=1.0): cv.float_,
#             cv.Optional(CONF_GAIN): _eq_gain_schema(),
#         }
#     )
#
# CONFIG_SCHEMA additions:
#     **{cv.Optional(f"{side}_eq_band_{band}"): _eq_band_schema() for side in ("left", "right") for band in range(1, 16)},


def _final_validate(config):
    full_conf = fv.full_config.get()

    this_number_id = config[CONF_TAS58XX_ID]
    have_this_number_eq_gains = has_eq_gains(config, CONF_LEFT_EQ_GAINS + CONF_RIGHT_EQ_GAINS)

    if have_this_number_eq_gains:
        # the tas58xx select config with the same audio_dac ID as this number
        select_conf = find_matching_config(full_conf, this_number_id, SELECT_COMPONENT)
        have_select_eq_mode = select_conf is not None and CONF_EQ_MODE in select_conf

        if not have_select_eq_mode:
            raise cv.Invalid("Select eq_mode is required with EQ Gain numbers - add Select eq_mode to YAML configuration")

        validate_eq_channels(get_eq_channels(config, select_conf))

    # the audio_dac config declaring the ID used by this number
    matching_audio_dac = get_audio_dac_config(full_conf, this_number_id)
    if matching_audio_dac is not None:
        is_dac_mode_btl = matching_audio_dac.get(CONF_DAC_MODE) == DAC_MODE_BTL

        have_this_number_channel_volume_left = CONF_CHANNEL_VOLUME_LEFT in config
        have_this_number_channel_volume_right = CONF_CHANNEL_VOLUME_RIGHT in config

        if is_dac_mode_btl:
            if (have_this_number_channel_volume_left and not have_this_number_channel_volume_right):
                raise cv.Invalid("channel_volume_right is required with channel_volume_left - add channel_volume_right to YAML configuration")

            if (have_this_number_channel_volume_right and not have_this_number_channel_volume_left):
                raise cv.Invalid("channel_volume_left is required with channel_volume_right - add channel_volume_left to YAML configuration")
        else:
            # warn rather than fail, so YAML that was valid in earlier releases still validates
            if (have_this_number_channel_volume_right):
                _LOGGER.warning("channel_volume_right is not used when dac_mode is PBTL - it can be removed from YAML configuration")
            if has_eq_gains(config, CONF_RIGHT_EQ_GAINS):
                _LOGGER.warning("Right EQ Gain numbers are not used when dac_mode is PBTL - they can be removed from YAML configuration")

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
)

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

    if has_eq_gains(config, CONF_LEFT_EQ_GAINS + CONF_RIGHT_EQ_GAINS):
        cg.add_define("USE_TAS58XX_EQ_GAINS")

    # Future: for each left_eq_band_N / right_eq_band_N in YAML, put its filter_type, frequency and q_factor in a
    # constant EqBand table for the channel (the default band for bands not in YAML), pass it with
    # config_eq_bands(channel, table) and create the gain number of the band as below.

    # left gains, then right gains
    for channel, gain_keys in EQ_GAIN_CHANNELS:
        for band, key in enumerate(gain_keys):
            if gain_config := config.get(key):
                n = await number.new_number(
                    gain_config, channel, band, min_value=-15, max_value=15, step=1
                )
                await cg.register_component(n, gain_config)
                await cg.register_parented(n, tas58xx_component)
