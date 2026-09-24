import esphome.codegen as cg
from esphome.core import CORE
from esphome.components import number
import esphome.config_validation as cv
import esphome.final_validate as fv

from esphome.const import (
    # CONF_AUDIO_DAC,
    # CONF_ID,
    # CONF_PLATFORM,
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
CONF_GAIN = "gain"

MAX_GAIN = 24
MIN_GAIN = -24
# CONF_LEFT_EQ_GAIN_20HZ = "left_eq_gain_20Hz"
# CONF_LEFT_EQ_GAIN_31P5HZ = "left_eq_gain_31.5Hz"
# CONF_LEFT_EQ_GAIN_50HZ = "left_eq_gain_50Hz"
# CONF_LEFT_EQ_GAIN_80HZ = "left_eq_gain_80Hz"
# CONF_LEFT_EQ_GAIN_125HZ = "left_eq_gain_125Hz"
# CONF_LEFT_EQ_GAIN_200HZ = "left_eq_gain_200Hz"
# CONF_LEFT_EQ_GAIN_315HZ = "left_eq_gain_315Hz"
# CONF_LEFT_EQ_GAIN_500HZ = "left_eq_gain_500Hz"
# CONF_LEFT_EQ_GAIN_800HZ = "left_eq_gain_800Hz"
# CONF_LEFT_EQ_GAIN_1250HZ = "left_eq_gain_1250Hz"
# CONF_LEFT_EQ_GAIN_2000HZ = "left_eq_gain_2000Hz"
# CONF_LEFT_EQ_GAIN_3150HZ = "left_eq_gain_3150Hz"
# CONF_LEFT_EQ_GAIN_5000HZ = "left_eq_gain_5000Hz"
# CONF_LEFT_EQ_GAIN_8000HZ = "left_eq_gain_8000Hz"
# CONF_LEFT_EQ_GAIN_16000HZ = "left_eq_gain_16000Hz"

# CONF_RIGHT_EQ_GAIN_20HZ = "right_eq_gain_20Hz"
# CONF_RIGHT_EQ_GAIN_31P5HZ = "right_eq_gain_31.5Hz"
# CONF_RIGHT_EQ_GAIN_50HZ = "right_eq_gain_50Hz"
# CONF_RIGHT_EQ_GAIN_80HZ = "right_eq_gain_80Hz"
# CONF_RIGHT_EQ_GAIN_125HZ = "right_eq_gain_125Hz"
# CONF_RIGHT_EQ_GAIN_200HZ = "right_eq_gain_200Hz"
# CONF_RIGHT_EQ_GAIN_315HZ = "right_eq_gain_315Hz"
# CONF_RIGHT_EQ_GAIN_500HZ = "right_eq_gain_500Hz"
# CONF_RIGHT_EQ_GAIN_800HZ = "right_eq_gain_800Hz"
# CONF_RIGHT_EQ_GAIN_1250HZ = "right_eq_gain_1250Hz"
# CONF_RIGHT_EQ_GAIN_2000HZ = "right_eq_gain_2000Hz"
# CONF_RIGHT_EQ_GAIN_3150HZ = "right_eq_gain_3150Hz"
# CONF_RIGHT_EQ_GAIN_5000HZ = "right_eq_gain_5000Hz"
# CONF_RIGHT_EQ_GAIN_8000HZ = "right_eq_gain_8000Hz"
# CONF_RIGHT_EQ_GAIN_16000HZ = "right_eq_gain_16000Hz"

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

# LeftEqGain20hz = tas58xx_ns.class_("LeftEqGain20hz", number.Number, cg.Component)
# LeftEqGain31p5hz = tas58xx_ns.class_("LeftEqGain31p5hz", number.Number, cg.Component)
# LeftEqGain50hz = tas58xx_ns.class_("LeftEqGain50hz", number.Number, cg.Component)
# LeftEqGain80hz = tas58xx_ns.class_("LeftEqGain80hz", number.Number, cg.Component)
# LeftEqGain125hz = tas58xx_ns.class_("LeftEqGain125hz", number.Number, cg.Component)
# LeftEqGain200hz = tas58xx_ns.class_("LeftEqGain200hz", number.Number, cg.Component)
# LeftEqGain315hz = tas58xx_ns.class_("LeftEqGain315hz", number.Number, cg.Component)
# LeftEqGain500hz = tas58xx_ns.class_("LeftEqGain500hz", number.Number, cg.Component)
# LeftEqGain800hz = tas58xx_ns.class_("LeftEqGain800hz", number.Number, cg.Component)
# LeftEqGain1250hz = tas58xx_ns.class_("LeftEqGain1250hz", number.Number, cg.Component)
# LeftEqGain2000hz = tas58xx_ns.class_("LeftEqGain2000hz", number.Number, cg.Component)
# LeftEqGain3150hz = tas58xx_ns.class_("LeftEqGain3150hz", number.Number, cg.Component)
# LeftEqGain5000hz = tas58xx_ns.class_("LeftEqGain5000hz", number.Number, cg.Component)
# LeftEqGain8000hz = tas58xx_ns.class_("LeftEqGain8000hz", number.Number, cg.Component)
# LeftEqGain16000hz = tas58xx_ns.class_("LeftEqGain16000hz", number.Number, cg.Component)

# RightEqGain20hz = tas58xx_ns.class_("RightEqGain20hz", number.Number, cg.Component)
# RightEqGain31p5hz = tas58xx_ns.class_("RightEqGain31p5hz", number.Number, cg.Component)
# RightEqGain50hz = tas58xx_ns.class_("RightEqGain50hz", number.Number, cg.Component)
# RightEqGain80hz = tas58xx_ns.class_("RightEqGain80hz", number.Number, cg.Component)
# RightEqGain125hz = tas58xx_ns.class_("RightEqGain125hz", number.Number, cg.Component)
# RightEqGain200hz = tas58xx_ns.class_("RightEqGain200hz", number.Number, cg.Component)
# RightEqGain315hz = tas58xx_ns.class_("RightEqGain315hz", number.Number, cg.Component)
# RightEqGain500hz = tas58xx_ns.class_("RightEqGain500hz", number.Number, cg.Component)
# RightEqGain800hz = tas58xx_ns.class_("RightEqGain800hz", number.Number, cg.Component)
# RightEqGain1250hz = tas58xx_ns.class_("RightEqGain1250hz", number.Number, cg.Component)
# RightEqGain2000hz = tas58xx_ns.class_("RightEqGain2000hz", number.Number, cg.Component)
# RightEqGain3150hz = tas58xx_ns.class_("RightEqGain3150hz", number.Number, cg.Component)
# RightEqGain5000hz = tas58xx_ns.class_("RightEqGain5000hz", number.Number, cg.Component)
# RightEqGain8000hz = tas58xx_ns.class_("RightEqGain8000hz", number.Number, cg.Component)
# RightEqGain16000hz = tas58xx_ns.class_("RightEqGain16000hz", number.Number, cg.Component)

# def have_left_eq_gains(config):
#     return (CONF_LEFT_EQ_GAIN_20HZ in config or
#             CONF_LEFT_EQ_GAIN_31P5HZ in config or CONF_LEFT_EQ_GAIN_50HZ in config or
#             CONF_LEFT_EQ_GAIN_80HZ in config or CONF_LEFT_EQ_GAIN_125HZ in config or
#             CONF_LEFT_EQ_GAIN_200HZ in config or CONF_LEFT_EQ_GAIN_315HZ in config or
#             CONF_LEFT_EQ_GAIN_500HZ in config or CONF_LEFT_EQ_GAIN_800HZ in config or
#             CONF_LEFT_EQ_GAIN_1250HZ in config or CONF_LEFT_EQ_GAIN_2000HZ in config or
#             CONF_LEFT_EQ_GAIN_3150HZ in config or CONF_LEFT_EQ_GAIN_5000HZ in config or
#             CONF_LEFT_EQ_GAIN_8000HZ in config or CONF_LEFT_EQ_GAIN_16000HZ in config)

# def have_right_eq_gains(config):
#     return (CONF_RIGHT_EQ_GAIN_20HZ in config or
#             CONF_RIGHT_EQ_GAIN_31P5HZ in config or CONF_RIGHT_EQ_GAIN_50HZ in config or
#             CONF_RIGHT_EQ_GAIN_80HZ in config or CONF_RIGHT_EQ_GAIN_125HZ in config or
#             CONF_RIGHT_EQ_GAIN_200HZ in config or CONF_RIGHT_EQ_GAIN_315HZ in config or
#             CONF_RIGHT_EQ_GAIN_500HZ in config or CONF_RIGHT_EQ_GAIN_800HZ in config or
#             CONF_RIGHT_EQ_GAIN_1250HZ in config or CONF_RIGHT_EQ_GAIN_2000HZ in config or
#             CONF_RIGHT_EQ_GAIN_3150HZ in config or CONF_RIGHT_EQ_GAIN_5000HZ in config or
#             CONF_RIGHT_EQ_GAIN_8000HZ in config or CONF_RIGHT_EQ_GAIN_16000HZ in config)

# def validate_eq_gain_numbers(config):
#     have_at_least_one_left_gain = (CONF_LEFT_EQ_GAIN_20HZ in config or
#                                     CONF_LEFT_EQ_GAIN_31P5HZ in config or CONF_LEFT_EQ_GAIN_50HZ in config or
#                                     CONF_LEFT_EQ_GAIN_80HZ in config or CONF_LEFT_EQ_GAIN_125HZ in config or
#                                     CONF_LEFT_EQ_GAIN_200HZ in config or CONF_LEFT_EQ_GAIN_315HZ in config or
#                                     CONF_LEFT_EQ_GAIN_500HZ in config or CONF_LEFT_EQ_GAIN_800HZ in config or
#                                     CONF_LEFT_EQ_GAIN_1250HZ in config or CONF_LEFT_EQ_GAIN_2000HZ in config or
#                                     CONF_LEFT_EQ_GAIN_3150HZ in config or CONF_LEFT_EQ_GAIN_5000HZ in config or
#                                     CONF_LEFT_EQ_GAIN_8000HZ in config or CONF_LEFT_EQ_GAIN_16000HZ in config)

#     have_all_left_gains = (CONF_LEFT_EQ_GAIN_20HZ in config and
#                             CONF_LEFT_EQ_GAIN_31P5HZ in config and CONF_LEFT_EQ_GAIN_50HZ in config and
#                             CONF_LEFT_EQ_GAIN_80HZ in config and CONF_LEFT_EQ_GAIN_125HZ in config and
#                             CONF_LEFT_EQ_GAIN_200HZ in config and CONF_LEFT_EQ_GAIN_315HZ in config and
#                             CONF_LEFT_EQ_GAIN_500HZ in config and CONF_LEFT_EQ_GAIN_800HZ in config and
#                             CONF_LEFT_EQ_GAIN_1250HZ in config and CONF_LEFT_EQ_GAIN_2000HZ in config and
#                             CONF_LEFT_EQ_GAIN_3150HZ in config and CONF_LEFT_EQ_GAIN_5000HZ in config and
#                             CONF_LEFT_EQ_GAIN_8000HZ in config and CONF_LEFT_EQ_GAIN_16000HZ in config)

#     have_at_least_one_right_gain = (CONF_RIGHT_EQ_GAIN_20HZ in config or
#                                     CONF_RIGHT_EQ_GAIN_31P5HZ in config or CONF_RIGHT_EQ_GAIN_50HZ in config or
#                                     CONF_RIGHT_EQ_GAIN_80HZ in config or CONF_RIGHT_EQ_GAIN_125HZ in config or
#                                     CONF_RIGHT_EQ_GAIN_200HZ in config or CONF_RIGHT_EQ_GAIN_315HZ in config or
#                                     CONF_RIGHT_EQ_GAIN_500HZ in config or CONF_RIGHT_EQ_GAIN_800HZ in config or
#                                     CONF_RIGHT_EQ_GAIN_1250HZ in config or CONF_RIGHT_EQ_GAIN_2000HZ in config or
#                                     CONF_RIGHT_EQ_GAIN_3150HZ in config or CONF_RIGHT_EQ_GAIN_5000HZ in config or
#                                     CONF_RIGHT_EQ_GAIN_8000HZ in config or CONF_RIGHT_EQ_GAIN_16000HZ in config)


#     have_all_right_gains = (CONF_RIGHT_EQ_GAIN_20HZ in config and
#                             CONF_RIGHT_EQ_GAIN_31P5HZ in config and CONF_RIGHT_EQ_GAIN_50HZ in config and
#                             CONF_RIGHT_EQ_GAIN_80HZ in config and CONF_RIGHT_EQ_GAIN_125HZ in config and
#                             CONF_RIGHT_EQ_GAIN_200HZ in config and CONF_RIGHT_EQ_GAIN_315HZ in config and
#                             CONF_RIGHT_EQ_GAIN_500HZ in config and CONF_RIGHT_EQ_GAIN_800HZ in config and
#                             CONF_RIGHT_EQ_GAIN_1250HZ in config and CONF_RIGHT_EQ_GAIN_2000HZ in config and
#                             CONF_RIGHT_EQ_GAIN_3150HZ in config and CONF_RIGHT_EQ_GAIN_5000HZ in config and
#                             CONF_RIGHT_EQ_GAIN_8000HZ in config and CONF_RIGHT_EQ_GAIN_16000HZ in config)

#     if (have_at_least_one_left_gain):
#        if (not have_all_left_gains):
#             raise cv.Invalid("All 15 Left EQ Gain numbers must be configured")

#     if (have_at_least_one_right_gain):
#         if (not have_all_right_gains):
#             raise cv.Invalid("All 15 Right EQ Gain numbers must be configured")

#     if (have_all_right_gains and not have_all_left_gains):
#         raise cv.Invalid("When Right EQ Gain Numbers are configured, all 15 Left EQ Gain numbers must also be configured")

#     return config


# KEY_NUMBER_EQ = "tas58xx_number_eq"
# KEY_LEFT_EQ_GAINS = "left_eq_gains"
# KEY_RIGHT_EQ_GAINS = "right_eq_gains"
# DICT_NO_EQ = {KEY_LEFT_EQ_GAINS: False, KEY_RIGHT_EQ_GAINS: False}

# def _final_validate(config):
#     full_conf = fv.full_config.get()

#     this_number_id = config[CONF_TAS58XX_ID]
#     audio_dac_id_matches_number_id = False
#     matching_audio_dac = None
#     # find the audic dac ID that matches the number ID
#     all_audio_dac = full_conf.get(CONF_AUDIO_DAC, [])
#     for audio_dac_conf in all_audio_dac:
#        if audio_dac_conf.get(CONF_PLATFORM) == PLATFORM_TAS58XX:
#            if audio_dac_conf.get(CONF_ID) == this_number_id:
#                 audio_dac_id_matches_number_id = True
#                 matching_audio_dac = audio_dac_conf
#                 break

#     if audio_dac_id_matches_number_id:
#         is_dac_mode_btl = matching_audio_dac.get(DAC_MODE) == DAC_MODE_BTL

#         have_this_number_channel_volume_left = CONF_CHANNEL_VOLUME_LEFT in config
#         have_this_number_channel_volume_right = CONF_CHANNEL_VOLUME_RIGHT in config

#         if is_dac_mode_btl:
#             if (have_this_number_channel_volume_left and not have_this_number_channel_volume_right):
#                 raise cv.Invalid("channel_volume_right is required with channel_volume_left - add channel_volume_right to YAML configuration")

#             if (have_this_number_channel_volume_right and not have_this_number_channel_volume_left):
#                 raise cv.Invalid("channel_volume_left is required with channel_volume_right - add channel_volume_left to YAML configuration")
#         else:
#             if (have_this_number_channel_volume_right):
#                 raise cv.Invalid("channel_volume_right is not required when dac_mode is PBTL - remove channel_volume_right from YAML configuration")


#     select_eq_mode_configured = False
#     select_left_eq_preset_configured = False
#     select_right_eq_preset_configured = False
#     select_confs = full_conf.get(SELECT_COMPONENT, [])
#     for select_conf in select_confs:
#         if select_conf.get(CONF_PLATFORM) == PLATFORM_TAS58XX:
#             if select_conf.get(CONF_TAS58XX_ID) == this_number_id:
#                 select_eq_mode_configured = EQ_MODE in select_conf
#                 select_left_eq_preset_configured = EQ_PRESET_LEFT_CHANNEL in select_conf
#                 select_right_eq_preset_configured = EQ_PRESET_RIGHT_CHANNEL in select_conf
#                 break

#     eq_configured = (have_left_eq_gains(config) or have_right_eq_gains(config) or select_left_eq_preset_configured or select_right_eq_preset_configured)

#     if eq_configured and not select_eq_mode_configured:
#         raise cv.Invalid("Select eq_mode is required with EQ Gain numbers and/or EQ Preset Select - add Select eq_mode to YAML configuration")

#     if have_left_eq_gains(config) and select_left_eq_preset_configured:
#         raise cv.Invalid("Left EQ Gain numbers are not allowed with Left Select eq_presets - remove one set of those configurations")

#     if have_right_eq_gains(config) and select_right_eq_preset_configured:
#             raise cv.Invalid("Left EQ Gain numbers are not allowed with Left Select eq_presets - remove one set of those configurations")

#     entry = CORE.data.setdefault(KEY_NUMBER_EQ, {}).setdefault(this_number_id, dict(DICT_NO_EQ))
#     if have_left_eq_gains(config):
#         entry[KEY_LEFT_EQ_GAINS] = True
#     if have_right_eq_gains(config):
#         entry[KEY_RIGHT_EQ_GAINS] = True

#     return config

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

# FINAL_VALIDATE_SCHEMA = _final_validate

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

        # cv.Optional(CONF_LEFT_EQ_GAIN_20HZ): number.number_schema(
        #     LeftEqGain20hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_LEFT_EQ_GAIN_31P5HZ): number.number_schema(
        #     LeftEqGain31p5hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_LEFT_EQ_GAIN_50HZ): number.number_schema(
        #     LeftEqGain50hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_LEFT_EQ_GAIN_80HZ): number.number_schema(
        #     LeftEqGain80hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_LEFT_EQ_GAIN_125HZ): number.number_schema(
        #     LeftEqGain125hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_LEFT_EQ_GAIN_200HZ): number.number_schema(
        #     LeftEqGain200hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_LEFT_EQ_GAIN_315HZ): number.number_schema(
        #     LeftEqGain315hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_LEFT_EQ_GAIN_500HZ): number.number_schema(
        #     LeftEqGain500hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_LEFT_EQ_GAIN_800HZ): number.number_schema(
        #     LeftEqGain800hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_LEFT_EQ_GAIN_1250HZ): number.number_schema(
        #     LeftEqGain1250hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_LEFT_EQ_GAIN_2000HZ): number.number_schema(
        #     LeftEqGain2000hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_LEFT_EQ_GAIN_3150HZ): number.number_schema(
        #     LeftEqGain3150hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_LEFT_EQ_GAIN_5000HZ): number.number_schema(
        #     LeftEqGain5000hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_LEFT_EQ_GAIN_8000HZ): number.number_schema(
        #     LeftEqGain8000hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_LEFT_EQ_GAIN_16000HZ): number.number_schema(
        #     LeftEqGain16000hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_RIGHT_EQ_GAIN_20HZ): number.number_schema(
        #     RightEqGain20hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_RIGHT_EQ_GAIN_31P5HZ): number.number_schema(
        #     RightEqGain31p5hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_RIGHT_EQ_GAIN_50HZ): number.number_schema(
        #     RightEqGain50hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_RIGHT_EQ_GAIN_80HZ): number.number_schema(
        #     RightEqGain80hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_RIGHT_EQ_GAIN_125HZ): number.number_schema(
        #     RightEqGain125hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_RIGHT_EQ_GAIN_200HZ): number.number_schema(
        #     RightEqGain200hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_RIGHT_EQ_GAIN_315HZ): number.number_schema(
        #     RightEqGain315hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_RIGHT_EQ_GAIN_500HZ): number.number_schema(
        #     RightEqGain500hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_RIGHT_EQ_GAIN_800HZ): number.number_schema(
        #     RightEqGain800hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_RIGHT_EQ_GAIN_1250HZ): number.number_schema(
        #     RightEqGain1250hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_RIGHT_EQ_GAIN_2000HZ): number.number_schema(
        #     RightEqGain2000hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_RIGHT_EQ_GAIN_3150HZ): number.number_schema(
        #     RightEqGain3150hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_RIGHT_EQ_GAIN_5000HZ): number.number_schema(
        #     RightEqGain5000hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_RIGHT_EQ_GAIN_8000HZ): number.number_schema(
        #     RightEqGain8000hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_RIGHT_EQ_GAIN_16000HZ): number.number_schema(
        #     RightEqGain16000hz,
        #     device_class=DEVICE_CLASS_SOUND_PRESSURE,
        #     entity_category=ENTITY_CATEGORY_CONFIG,
        #     icon=ICON_VOLUME_SOURCE,
        #     unit_of_measurement=UNIT_DECIBEL,
        # )
        # .extend(cv.COMPONENT_SCHEMA),
#     }
# ).add_extra(validate_eq_gain_numbers)

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

    # if left_gain_20hz_config := config.get(CONF_LEFT_EQ_GAIN_20HZ):
    #     cg.add_define("USE_TAS58XX_EQ_GAINS")
    #     n = await number.new_number(
    #        left_gain_20hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, left_gain_20hz_config)
    #     await cg.register_parented(n, tas58xx_component)
    #     # cg.add(tas58xx_component.set_band1(n))

    # if left_gain_31p5hz_config := config.get(CONF_LEFT_EQ_GAIN_31P5HZ):
    #     n = await number.new_number(
    #         left_gain_31p5hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, left_gain_31p5hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if left_gain_50hz_config := config.get(CONF_LEFT_EQ_GAIN_50HZ):
    #     n = await number.new_number(
    #         left_gain_50hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, left_gain_50hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if left_gain_80hz_config := config.get(CONF_LEFT_EQ_GAIN_80HZ):
    #     n = await number.new_number(
    #         left_gain_80hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, left_gain_80hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if left_gain_125hz_config := config.get(CONF_LEFT_EQ_GAIN_125HZ):
    #     n = await number.new_number(
    #         left_gain_125hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, left_gain_125hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if left_gain_200hz_config := config.get(CONF_LEFT_EQ_GAIN_200HZ):
    #     n = await number.new_number(
    #         left_gain_200hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, left_gain_200hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if left_gain_315hz_config := config.get(CONF_LEFT_EQ_GAIN_315HZ):
    #     n = await number.new_number(
    #         left_gain_315hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, left_gain_315hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if left_gain_500hz_config := config.get(CONF_LEFT_EQ_GAIN_500HZ):
    #     n = await number.new_number(
    #         left_gain_500hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, left_gain_500hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if left_gain_800hz_config := config.get(CONF_LEFT_EQ_GAIN_800HZ):
    #     n = await number.new_number(
    #         left_gain_800hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, left_gain_800hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if left_gain_1250hz_config := config.get(CONF_LEFT_EQ_GAIN_1250HZ):
    #     n = await number.new_number(
    #         left_gain_1250hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, left_gain_1250hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if left_gain_2000hz_config := config.get(CONF_LEFT_EQ_GAIN_2000HZ):
    #     n = await number.new_number(
    #         left_gain_2000hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, left_gain_2000hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if left_gain_3150hz_config := config.get(CONF_LEFT_EQ_GAIN_3150HZ):
    #     n = await number.new_number(
    #         left_gain_3150hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, left_gain_3150hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if left_gain_5000hz_config := config.get(CONF_LEFT_EQ_GAIN_5000HZ):
    #     n = await number.new_number(
    #         left_gain_5000hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, left_gain_5000hz_config)
    #     await cg.register_parented(n, tas58xx_component)


    # if left_gain_8000hz_config := config.get(CONF_LEFT_EQ_GAIN_8000HZ):
    #     n = await number.new_number(
    #         left_gain_8000hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, left_gain_8000hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if left_gain_16000hz_config := config.get(CONF_LEFT_EQ_GAIN_16000HZ):
    #     n = await number.new_number(
    #         left_gain_16000hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, left_gain_16000hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # # Right Eq Gain - EQ BiAMP
    # if right_gain_20hz_config := config.get(CONF_RIGHT_EQ_GAIN_20HZ):
    #     cg.add_define("USE_TAS58XX_EQ_BIAMP")
    #     n = await number.new_number(
    #        right_gain_20hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, right_gain_20hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if right_gain_31p5hz_config := config.get(CONF_RIGHT_EQ_GAIN_31P5HZ):

    #     n = await number.new_number(
    #         right_gain_31p5hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, right_gain_31p5hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if right_gain_50hz_config := config.get(CONF_RIGHT_EQ_GAIN_50HZ):
    #     n = await number.new_number(
    #         right_gain_50hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, right_gain_50hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if right_gain_80hz_config := config.get(CONF_RIGHT_EQ_GAIN_80HZ):
    #     n = await number.new_number(
    #         right_gain_80hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, right_gain_80hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if right_gain_125hz_config := config.get(CONF_RIGHT_EQ_GAIN_125HZ):
    #     n = await number.new_number(
    #         right_gain_125hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, right_gain_125hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if right_gain_200hz_config := config.get(CONF_RIGHT_EQ_GAIN_200HZ):
    #     n = await number.new_number(
    #         right_gain_200hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, right_gain_200hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if right_gain_315hz_config := config.get(CONF_RIGHT_EQ_GAIN_315HZ):
    #     n = await number.new_number(
    #         right_gain_315hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, right_gain_315hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if right_gain_500hz_config := config.get(CONF_RIGHT_EQ_GAIN_500HZ):
    #     n = await number.new_number(
    #         right_gain_500hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, right_gain_500hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if right_gain_800hz_config := config.get(CONF_RIGHT_EQ_GAIN_800HZ):
    #     n = await number.new_number(
    #         right_gain_800hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, right_gain_800hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if right_gain_1250hz_config := config.get(CONF_RIGHT_EQ_GAIN_1250HZ):
    #     n = await number.new_number(
    #         right_gain_1250hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, right_gain_1250hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if right_gain_2000hz_config := config.get(CONF_RIGHT_EQ_GAIN_2000HZ):
    #     n = await number.new_number(
    #         right_gain_2000hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, right_gain_2000hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if right_gain_3150hz_config := config.get(CONF_RIGHT_EQ_GAIN_3150HZ):
    #     n = await number.new_number(
    #         right_gain_3150hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, right_gain_3150hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if right_gain_5000hz_config := config.get(CONF_RIGHT_EQ_GAIN_5000HZ):
    #     n = await number.new_number(
    #         right_gain_5000hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, right_gain_5000hz_config)
    #     await cg.register_parented(n, tas58xx_component)


    # if right_gain_8000hz_config := config.get(CONF_RIGHT_EQ_GAIN_8000HZ):
    #     n = await number.new_number(
    #         right_gain_8000hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, right_gain_8000hz_config)
    #     await cg.register_parented(n, tas58xx_component)

    # if right_gain_16000hz_config := config.get(CONF_RIGHT_EQ_GAIN_16000HZ):
    #     n = await number.new_number(
    #         right_gain_16000hz_config, min_value=-15, max_value=15, step=1
    #     )
    #     await cg.register_component(n, right_gain_16000hz_config)
    #     await cg.register_parented(n, tas58xx_component)


