import esphome.codegen as cg
from esphome.core import CORE
from esphome.components import number
import esphome.config_validation as cv
import esphome.final_validate as fv

from esphome.const import (
    CONF_AUDIO_DAC,
    CONF_ID,
    CONF_PLATFORM,
    DEVICE_CLASS_SOUND_PRESSURE,
    ENTITY_CATEGORY_CONFIG,
    UNIT_DECIBEL,
)

SELECT_COMPONENT = "select"
PLATFORM_TAS58XX = "tas58xx"
DAC_MODE = "dac_mode"
DAC_MODE_BTL = "BTL"
EQ_MODE = "eq_mode"
EQ_PRESET_LEFT_CHANNEL = "eq_preset_left_channel"
EQ_PRESET_RIGHT_CHANNEL = "eq_preset_right_channel"

CONF_CHANNEL_VOLUME_LEFT = "channel_volume_left"
CONF_CHANNEL_VOLUME_RIGHT = "channel_volume_right"
CONF_LEFT_EQ_BANDS = tuple(f"left_eq_band_{i}" for i in range(1, 16))
CONF_RIGHT_EQ_BANDS = tuple(f"right_eq_band_{i}" for i in range(1, 16))

CONF_GAIN = "gain"

MAX_GAIN = 24
MIN_GAIN = -24

ICON_VOLUME_SOURCE = "mdi:volume-source"

from ..audio_dac import CONF_TAS58XX_ID, Tas58xxComponent, tas58xx_ns

Channel = tas58xx_ns.enum("Channel")
CHANNELS = {
    "left": Channel.LEFT_CHANNEL,
    "right": Channel.RIGHT_CHANNEL,
}
# CHANNELS = {"left": 0, "right": 1}
# EqFilter = tas58xx_ns.enum("EqFilter")
# EQ_FILTERS = {
#     "Equalizer": EqFilter.EQUALIZER,
#     "High Pass": EqFilter.LOW_PASS,
#     "Low Pass": EqFilter.HIGH_PASS,
#     "High Shelf": EqFilter.HIGH_SHELF,
#     "Low Shelf": EqFilter.LOW_SHELF,
#     "Peaking Eq": EqFilter.PEAKING_EQ,
# }

ChannelVolumeLeft = tas58xx_ns.class_("ChannelVolumeLeft", number.Number, cg.Component)
ChannelVolumeRight = tas58xx_ns.class_("ChannelVolumeRight", number.Number, cg.Component)
EqBandGain = tas58xx_ns.class_("EqBandGain", number.Number, cg.Component)


KEY_NUMBER_EQ = "tas58xx_number_eq"
KEY_LEFT_EQ_GAINS = "left_eq_gains"
KEY_RIGHT_EQ_GAINS = "right_eq_gains"
DICT_NO_EQ = {KEY_LEFT_EQ_GAINS: False, KEY_RIGHT_EQ_GAINS: False}

def find_matching_select(full_conf, dac_id):
    for select_conf in full_conf.get(SELECT_COMPONENT, []):
        if select_conf.get(CONF_PLATFORM) != PLATFORM_TAS58XX:
            continue
        if select_conf.get(CONF_TAS58XX_ID) == dac_id:
            return select_conf
    return None

def _final_validate(config):
    full_conf = fv.full_config.get()

    this_number_id = config[CONF_TAS58XX_ID]
    # audio_dac_id_matches_number_id = False
    matching_audio_dac = None

    # # find the audic dac ID that matches the number ID
    # all_audio_dac = full_conf.get(CONF_AUDIO_DAC, [])
    # for audio_dac_conf in all_audio_dac:
    #    if audio_dac_conf.get(CONF_PLATFORM) == PLATFORM_TAS58XX:
    #        if audio_dac_conf.get(CONF_ID) == this_number_id:
    #             audio_dac_id_matches_number_id = True
    #             matching_audio_dac = audio_dac_conf
    #             break
    try:
        dac_path = full_conf.get_path_for_id(this_number_id)[:-1]
        matching_audio_dac = full_conf.get_config_for_path(dac_path)
    except KeyError:
        raise cv.Invalid("YAML validation error - no audio_dac with same id as number")

    # is_dac_mode_btl = matching_audio_dac.get(DAC_MODE) == DAC_MODE_BTL
    # if audio_dac_id_matches_number_id:

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
            raise cv.Invalid("channel_volume_right is not required when dac_mode is PBTL - remove channel_volume_right from YAML configuration")

    # # find the select ID that matches the number ID
    # select_eq_mode_configured = False
    # select_left_eq_preset_configured = False
    # select_right_eq_preset_configured = False
    # select_confs = full_conf.get(SELECT_COMPONENT, [])
    # for select_conf in select_confs:
    #     if select_conf.get(CONF_PLATFORM) == PLATFORM_TAS58XX:
    #         if select_conf.get(CONF_TAS58XX_ID) == this_number_id:
    #             select_eq_mode_configured = EQ_MODE in select_conf
    #             select_left_eq_preset_configured = EQ_PRESET_LEFT_CHANNEL in select_conf
    #             select_right_eq_preset_configured = EQ_PRESET_RIGHT_CHANNEL in select_conf
    #             break

    matching_select = find_matching_select(full_conf, this_number_id)
    select_eq_mode_configured = matching_select is not None and EQ_MODE in matching_select
    select_left_eq_preset_configured = matching_select is not None and EQ_PRESET_LEFT_CHANNEL in matching_select
    select_right_eq_preset_configured = matching_select is not None and EQ_PRESET_RIGHT_CHANNEL in matching_select

    have_left_eq_gains = any(k in config for k in CONF_LEFT_EQ_BANDS)
    have_right_eq_gains = any(k in config for k in CONF_RIGHT_EQ_BANDS)

    eq_configured = have_left_eq_gains or have_right_eq_gains or select_left_eq_preset_configured or select_right_eq_preset_configured

    if eq_configured and not select_eq_mode_configured:
        raise cv.Invalid("Select eq_mode is required with EQ Gain numbers and/or EQ Preset Select - add Select eq_mode to YAML configuration")

    if have_left_eq_gains and select_left_eq_preset_configured:
        raise cv.Invalid("Left EQ Gain numbers are not allowed with Left Select eq_presets - remove one set of those configurations")

    if have_right_eq_gains and select_right_eq_preset_configured:
            raise cv.Invalid("Right EQ Gain numbers are not allowed with Right Select eq_presets - remove one set of those configurations")

    entry = CORE.data.setdefault(KEY_NUMBER_EQ, {}).setdefault(this_number_id, dict(DICT_NO_EQ))
    if have_left_eq_gains:
        entry[KEY_LEFT_EQ_GAINS] = True
    if have_right_eq_gains:
        entry[KEY_RIGHT_EQ_GAINS] = True

    return config

def _band_schema():
    # schema for each band's EQ configuration

    return cv.All(
        cv.Schema(
            {
                # cv.Optional("filter_type", default="Equalizer"): cv.enum(
                #     EQ_FILTERS, upper=False
                # ),
                # cv.Optional("frequency", default=1000): cv.All(
                #     cv.int_, cv.int_range(1, 20000)
                # ),
                # cv.Optional("q_factor", default=1.0): cv.float_,
                cv.Optional(CONF_GAIN): number.number_schema(
                    EqBandGain,
                    device_class=DEVICE_CLASS_SOUND_PRESSURE,
                    entity_category=ENTITY_CATEGORY_CONFIG,
                    icon=ICON_VOLUME_SOURCE,
                    unit_of_measurement=UNIT_DECIBEL,
                ),
            }
        ),
    )

FINAL_VALIDATE_SCHEMA = _final_validate

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_TAS58XX_ID): cv.use_id(Tas58xxComponent),

        cv.Optional(CONF_CHANNEL_VOLUME_LEFT): number.number_schema(
            ChannelVolumeLeft,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        ),

        cv.Optional(CONF_CHANNEL_VOLUME_RIGHT): number.number_schema(
            ChannelVolumeRight,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        ),

        cv.Optional("left_eq_band_1"): _band_schema(),
        cv.Optional("left_eq_band_2"): _band_schema(),
        cv.Optional("left_eq_band_3"): _band_schema(),
        cv.Optional("left_eq_band_4"): _band_schema(),
        cv.Optional("left_eq_band_5"): _band_schema(),
        cv.Optional("left_eq_band_6"): _band_schema(),
        cv.Optional("left_eq_band_7"): _band_schema(),
        cv.Optional("left_eq_band_8"): _band_schema(),
        cv.Optional("left_eq_band_9"): _band_schema(),
        cv.Optional("left_eq_band_10"): _band_schema(),
        cv.Optional("left_eq_band_11"): _band_schema(),
        cv.Optional("left_eq_band_12"): _band_schema(),
        cv.Optional("left_eq_band_13"): _band_schema(),
        cv.Optional("left_eq_band_14"): _band_schema(),
        cv.Optional("left_eq_band_15"): _band_schema(),
        cv.Optional("right_eq_band_1"): _band_schema(),
        cv.Optional("right_eq_band_2"): _band_schema(),
        cv.Optional("right_eq_band_3"): _band_schema(),
        cv.Optional("right_eq_band_4"): _band_schema(),
        cv.Optional("right_eq_band_5"): _band_schema(),
        cv.Optional("right_eq_band_6"): _band_schema(),
        cv.Optional("right_eq_band_7"): _band_schema(),
        cv.Optional("right_eq_band_8"): _band_schema(),
        cv.Optional("right_eq_band_9"): _band_schema(),
        cv.Optional("right_eq_band_10"): _band_schema(),
        cv.Optional("right_eq_band_11"): _band_schema(),
        cv.Optional("right_eq_band_12"): _band_schema(),
        cv.Optional("right_eq_band_13"): _band_schema(),
        cv.Optional("right_eq_band_14"): _band_schema(),
        cv.Optional("right_eq_band_15"): _band_schema(),
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

    for channel_txt, channel_enum in CHANNELS.items():
    # for channel_txt in ("left", "right"):
        for band_num in range(1, 15):
            eq_band_config = config.get(f"{channel_txt}_eq_band_{band_num}")
            if eq_band_config is None:
                continue

            # configuration values (validated by schema)
            # filter_type = band_config["filter_type"]
            # frequency = band_config["frequency"]
            # q_factor = band_config["q_factor"]

            # Create the runtime-adjustable Gain number
            if gain_config := eq_band_config.get(CONF_GAIN):
                n = await number.new_number(
                    gain_config,
                    min_value=float(MIN_GAIN),
                    max_value=float(MAX_GAIN),
                    step=1,
                )
                await cg.register_component(n, gain_config)
                await cg.register_parented(n, tas58xx_component)
                cg.add(n.set_channel(channel_enum))
                # cg.add(n.set_channel(CHANNELS[channel_txt]))
                cg.add(n.set_band(band_num - 1))
                # cg.add(n.set_filter_type(filter_type))
                # cg.add(n.set_frequency(frequency))
                # cg.add(n.set_q_factor(q_factor))


