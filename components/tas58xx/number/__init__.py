import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
import esphome.final_validate as fv

from esphome.const import (
    CONF_PLATFORM,
    DEVICE_CLASS_SOUND_PRESSURE,
    ENTITY_CATEGORY_CONFIG,
    UNIT_DECIBEL,
)

SELECT_COMPONENT = "select"
PLATFORM_TAS58XX = "tas58xx"
EQ_MODE = "eq_mode"
EQ_PRESET_LEFT_CHANNEL = "eq_preset_left_channel"

CONF_CHANNEL_VOLUME_LEFT = "channel_volume_left"
CONF_CHANNEL_VOLUME_RIGHT = "channel_volume_right"
CONF_LEFT_EQ_GAIN_20HZ = "left_eq_gain_20Hz"
CONF_LEFT_EQ_GAIN_31P5HZ = "left_eq_gain_31.5Hz"
CONF_LEFT_EQ_GAIN_50HZ = "left_eq_gain_50Hz"
CONF_LEFT_EQ_GAIN_80HZ = "left_eq_gain_80Hz"
CONF_LEFT_EQ_GAIN_125HZ = "left_eq_gain_125Hz"
CONF_LEFT_EQ_GAIN_200HZ = "left_eq_gain_200Hz"
CONF_LEFT_EQ_GAIN_315HZ = "left_eq_gain_315Hz"
CONF_LEFT_EQ_GAIN_500HZ = "left_eq_gain_500Hz"
CONF_LEFT_EQ_GAIN_800HZ = "left_eq_gain_800Hz"
CONF_LEFT_EQ_GAIN_1250HZ = "left_eq_gain_1250Hz"
CONF_LEFT_EQ_GAIN_2000HZ = "left_eq_gain_2000Hz"
CONF_LEFT_EQ_GAIN_3150HZ = "left_eq_gain_3150Hz"
CONF_LEFT_EQ_GAIN_5000HZ = "left_eq_gain_5000Hz"
CONF_LEFT_EQ_GAIN_8000HZ = "left_eq_gain_8000Hz"
CONF_LEFT_EQ_GAIN_16000HZ = "left_eq_gain_16000Hz"

CONF_RIGHT_EQ_GAIN_20HZ = "right_eq_gain_20Hz"
CONF_RIGHT_EQ_GAIN_31P5HZ = "right_eq_gain_31.5Hz"
CONF_RIGHT_EQ_GAIN_50HZ = "right_eq_gain_50Hz"
CONF_RIGHT_EQ_GAIN_80HZ = "right_eq_gain_80Hz"
CONF_RIGHT_EQ_GAIN_125HZ = "right_eq_gain_125Hz"
CONF_RIGHT_EQ_GAIN_200HZ = "right_eq_gain_200Hz"
CONF_RIGHT_EQ_GAIN_315HZ = "right_eq_gain_315Hz"
CONF_RIGHT_EQ_GAIN_500HZ = "right_eq_gain_500Hz"
CONF_RIGHT_EQ_GAIN_800HZ = "right_eq_gain_800Hz"
CONF_RIGHT_EQ_GAIN_1250HZ = "right_eq_gain_1250Hz"
CONF_RIGHT_EQ_GAIN_2000HZ = "right_eq_gain_2000Hz"
CONF_RIGHT_EQ_GAIN_3150HZ = "right_eq_gain_3150Hz"
CONF_RIGHT_EQ_GAIN_5000HZ = "right_eq_gain_5000Hz"
CONF_RIGHT_EQ_GAIN_8000HZ = "right_eq_gain_8000Hz"
CONF_RIGHT_EQ_GAIN_16000HZ = "right_eq_gain_16000Hz"

ICON_VOLUME_SOURCE = "mdi:volume-source"

from ..audio_dac import CONF_TAS58XX_ID, Tas58xxComponent, tas58xx_ns

ChannelVolumeLeft = tas58xx_ns.class_("ChannelVolumeLeft", number.Number, cg.Component)
ChannelVolumeRight = tas58xx_ns.class_("ChannelVolumeRight", number.Number, cg.Component)

LeftEqGain20hz = tas58xx_ns.class_("LeftEqGain20hz", number.Number, cg.Component)
LeftEqGain31p5hz = tas58xx_ns.class_("LeftEqGain31p5hz", number.Number, cg.Component)
LeftEqGain50hz = tas58xx_ns.class_("LeftEqGain50hz", number.Number, cg.Component)
LeftEqGain80hz = tas58xx_ns.class_("LeftEqGain80hz", number.Number, cg.Component)
LeftEqGain125hz = tas58xx_ns.class_("LeftEqGain125hz", number.Number, cg.Component)
LeftEqGain200hz = tas58xx_ns.class_("LeftEqGain200hz", number.Number, cg.Component)
LeftEqGain315hz = tas58xx_ns.class_("LeftEqGain315hz", number.Number, cg.Component)
LeftEqGain500hz = tas58xx_ns.class_("LeftEqGain500hz", number.Number, cg.Component)
LeftEqGain800hz = tas58xx_ns.class_("LeftEqGain800hz", number.Number, cg.Component)
LeftEqGain1250hz = tas58xx_ns.class_("LeftEqGain1250hz", number.Number, cg.Component)
LeftEqGain2000hz = tas58xx_ns.class_("LeftEqGain2000hz", number.Number, cg.Component)
LeftEqGain3150hz = tas58xx_ns.class_("LeftEqGain3150hz", number.Number, cg.Component)
LeftEqGain5000hz = tas58xx_ns.class_("LeftEqGain5000hz", number.Number, cg.Component)
LeftEqGain8000hz = tas58xx_ns.class_("LeftEqGain8000hz", number.Number, cg.Component)
LeftEqGain16000hz = tas58xx_ns.class_("LeftEqGain16000hz", number.Number, cg.Component)

RightEqGain20hz = tas58xx_ns.class_("RightEqGain20hz", number.Number, cg.Component)
RightEqGain31p5hz = tas58xx_ns.class_("RightEqGain31p5hz", number.Number, cg.Component)
RightEqGain50hz = tas58xx_ns.class_("RightEqGain50hz", number.Number, cg.Component)
RightEqGain80hz = tas58xx_ns.class_("RightEqGain80hz", number.Number, cg.Component)
RightEqGain125hz = tas58xx_ns.class_("RightEqGain125hz", number.Number, cg.Component)
RightEqGain200hz = tas58xx_ns.class_("RightEqGain200hz", number.Number, cg.Component)
RightEqGain315hz = tas58xx_ns.class_("RightEqGain315hz", number.Number, cg.Component)
RightEqGain500hz = tas58xx_ns.class_("RightEqGain500hz", number.Number, cg.Component)
RightEqGain800hz = tas58xx_ns.class_("RightEqGain800hz", number.Number, cg.Component)
RightEqGain1250hz = tas58xx_ns.class_("RightEqGain1250hz", number.Number, cg.Component)
RightEqGain2000hz = tas58xx_ns.class_("RightEqGain2000hz", number.Number, cg.Component)
RightEqGain3150hz = tas58xx_ns.class_("RightEqGain3150hz", number.Number, cg.Component)
RightEqGain5000hz = tas58xx_ns.class_("RightEqGain5000hz", number.Number, cg.Component)
RightEqGain8000hz = tas58xx_ns.class_("RightEqGain8000hz", number.Number, cg.Component)
RightEqGain16000hz = tas58xx_ns.class_("RightEqGain16000hz", number.Number, cg.Component)

def validate_eq_gain_numbers(config):
    have_at_least_one_left_gain = (CONF_LEFT_EQ_GAIN_20HZ in config or
                                    CONF_LEFT_EQ_GAIN_31P5HZ in config or CONF_LEFT_EQ_GAIN_50HZ in config or
                                    CONF_LEFT_EQ_GAIN_80HZ in config or CONF_LEFT_EQ_GAIN_125HZ in config or
                                    CONF_LEFT_EQ_GAIN_200HZ in config or CONF_LEFT_EQ_GAIN_315HZ in config or
                                    CONF_LEFT_EQ_GAIN_500HZ in config or CONF_LEFT_EQ_GAIN_800HZ in config or
                                    CONF_LEFT_EQ_GAIN_1250HZ in config or CONF_LEFT_EQ_GAIN_2000HZ in config or
                                    CONF_LEFT_EQ_GAIN_3150HZ in config or CONF_LEFT_EQ_GAIN_5000HZ in config or
                                    CONF_LEFT_EQ_GAIN_8000HZ in config or CONF_LEFT_EQ_GAIN_16000HZ in config)

    have_all_left_gains = (CONF_LEFT_EQ_GAIN_20HZ in config and
                            CONF_LEFT_EQ_GAIN_31P5HZ in config and CONF_LEFT_EQ_GAIN_50HZ in config and
                            CONF_LEFT_EQ_GAIN_80HZ in config and CONF_LEFT_EQ_GAIN_125HZ in config and
                            CONF_LEFT_EQ_GAIN_200HZ in config and CONF_LEFT_EQ_GAIN_315HZ in config and
                            CONF_LEFT_EQ_GAIN_500HZ in config and CONF_LEFT_EQ_GAIN_800HZ in config and
                            CONF_LEFT_EQ_GAIN_1250HZ in config and CONF_LEFT_EQ_GAIN_2000HZ in config and
                            CONF_LEFT_EQ_GAIN_3150HZ in config and CONF_LEFT_EQ_GAIN_5000HZ in config and
                            CONF_LEFT_EQ_GAIN_8000HZ in config and CONF_LEFT_EQ_GAIN_16000HZ in config)

    have_at_least_one_right_gain = (CONF_RIGHT_EQ_GAIN_20HZ in config or
                                    CONF_RIGHT_EQ_GAIN_31P5HZ in config or CONF_RIGHT_EQ_GAIN_50HZ in config or
                                    CONF_RIGHT_EQ_GAIN_80HZ in config or CONF_RIGHT_EQ_GAIN_125HZ in config or
                                    CONF_RIGHT_EQ_GAIN_200HZ in config or CONF_RIGHT_EQ_GAIN_315HZ in config or
                                    CONF_RIGHT_EQ_GAIN_500HZ in config or CONF_RIGHT_EQ_GAIN_800HZ in config or
                                    CONF_RIGHT_EQ_GAIN_1250HZ in config or CONF_RIGHT_EQ_GAIN_2000HZ in config or
                                    CONF_RIGHT_EQ_GAIN_3150HZ in config or CONF_RIGHT_EQ_GAIN_5000HZ in config or
                                    CONF_RIGHT_EQ_GAIN_8000HZ in config or CONF_RIGHT_EQ_GAIN_16000HZ in config)


    have_all_right_gains = (CONF_RIGHT_EQ_GAIN_20HZ in config and
                            CONF_RIGHT_EQ_GAIN_31P5HZ in config and CONF_RIGHT_EQ_GAIN_50HZ in config and
                            CONF_RIGHT_EQ_GAIN_80HZ in config and CONF_RIGHT_EQ_GAIN_125HZ in config and
                            CONF_RIGHT_EQ_GAIN_200HZ in config and CONF_RIGHT_EQ_GAIN_315HZ in config and
                            CONF_RIGHT_EQ_GAIN_500HZ in config and CONF_RIGHT_EQ_GAIN_800HZ in config and
                            CONF_RIGHT_EQ_GAIN_1250HZ in config and CONF_RIGHT_EQ_GAIN_2000HZ in config and
                            CONF_RIGHT_EQ_GAIN_3150HZ in config and CONF_RIGHT_EQ_GAIN_5000HZ in config and
                            CONF_RIGHT_EQ_GAIN_8000HZ in config and CONF_RIGHT_EQ_GAIN_16000HZ in config)
    if (have_at_least_one_left_gain):
       if (not have_all_left_gains):
            raise cv.Invalid("All 15 Left EQ Gain numbers must be configured")

    if (have_at_least_one_right_gain):
        if (not have_all_right_gains):
            raise cv.Invalid("All 15 Right EQ Gain numbers must be configured")

    if (have_all_right_gains and not have_all_left_gains):
        raise cv.Invalid("Right EQ Gain Numbers must have all 15 Left EQ Gain numbers also configured")

    have_channel_volume_left = CONF_CHANNEL_VOLUME_LEFT in config
    have_channel_volume_right = CONF_CHANNEL_VOLUME_RIGHT in config

    if (have_channel_volume_left and not have_channel_volume_right):
        raise cv.Invalid("channel_volume_right must be configured with channel_volume_left - Add channel_volume_right to YAML configuration")

    if (have_channel_volume_right and not have_channel_volume_left):
        raise cv.Invalid("channel_volume_left must be configured with channel_volume_right - Add channel_volume_left to YAML configuration")

    return config

def _final_validate(config):
    have_defined_tas58xx_select_eq_preset = False
    have_defined_tas58xx_select_eq_mode = False

    have_defined_tas58xx_number_eq_gains = (CONF_LEFT_EQ_GAIN_20HZ in config or CONF_RIGHT_EQ_GAIN_20HZ in config)

    full_conf = fv.full_config.get()
    select_confs = full_conf.get(SELECT_COMPONENT, [])

    for select_conf in select_confs:
        if select_conf.get(CONF_PLATFORM) == PLATFORM_TAS58XX:
           if EQ_MODE in select_conf:
               have_defined_tas58xx_select_eq_mode = True
               break

    for select_conf in select_confs:
        if select_conf.get(CONF_PLATFORM) == PLATFORM_TAS58XX:
           if EQ_PRESET_LEFT_CHANNEL in select_conf:
               have_defined_tas58xx_select_eq_preset = True
               break

    if (have_defined_tas58xx_number_eq_gains and not have_defined_tas58xx_select_eq_mode):
        raise cv.Invalid("Select eq_mode must be configured with left_eq_gains - add Select eq_mode to YAML configuration")

    if (have_defined_tas58xx_number_eq_gains and have_defined_tas58xx_select_eq_preset):
        raise cv.Invalid("left_eq_gains and right_eq_gains are not allowed with Select eq_presets - remove one set of those configurations")
    return config

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

        cv.Optional(CONF_LEFT_EQ_GAIN_20HZ): number.number_schema(
            LeftEqGain20hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_LEFT_EQ_GAIN_31P5HZ): number.number_schema(
            LeftEqGain31p5hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_LEFT_EQ_GAIN_50HZ): number.number_schema(
            LeftEqGain50hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_LEFT_EQ_GAIN_80HZ): number.number_schema(
            LeftEqGain80hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_LEFT_EQ_GAIN_125HZ): number.number_schema(
            LeftEqGain125hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_LEFT_EQ_GAIN_200HZ): number.number_schema(
            LeftEqGain200hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_LEFT_EQ_GAIN_315HZ): number.number_schema(
            LeftEqGain315hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_LEFT_EQ_GAIN_500HZ): number.number_schema(
            LeftEqGain500hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_LEFT_EQ_GAIN_800HZ): number.number_schema(
            LeftEqGain800hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_LEFT_EQ_GAIN_1250HZ): number.number_schema(
            LeftEqGain1250hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_LEFT_EQ_GAIN_2000HZ): number.number_schema(
            LeftEqGain2000hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_LEFT_EQ_GAIN_3150HZ): number.number_schema(
            LeftEqGain3150hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_LEFT_EQ_GAIN_5000HZ): number.number_schema(
            LeftEqGain5000hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_LEFT_EQ_GAIN_8000HZ): number.number_schema(
            LeftEqGain8000hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_LEFT_EQ_GAIN_16000HZ): number.number_schema(
            LeftEqGain16000hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_RIGHT_EQ_GAIN_20HZ): number.number_schema(
            RightEqGain20hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_RIGHT_EQ_GAIN_31P5HZ): number.number_schema(
            RightEqGain31p5hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_RIGHT_EQ_GAIN_50HZ): number.number_schema(
            RightEqGain50hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_RIGHT_EQ_GAIN_80HZ): number.number_schema(
            RightEqGain80hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_RIGHT_EQ_GAIN_125HZ): number.number_schema(
            RightEqGain125hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_RIGHT_EQ_GAIN_200HZ): number.number_schema(
            RightEqGain200hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_RIGHT_EQ_GAIN_315HZ): number.number_schema(
            RightEqGain315hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_RIGHT_EQ_GAIN_500HZ): number.number_schema(
            RightEqGain500hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_RIGHT_EQ_GAIN_800HZ): number.number_schema(
            RightEqGain800hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_RIGHT_EQ_GAIN_1250HZ): number.number_schema(
            RightEqGain1250hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_RIGHT_EQ_GAIN_2000HZ): number.number_schema(
            RightEqGain2000hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_RIGHT_EQ_GAIN_3150HZ): number.number_schema(
            RightEqGain3150hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_RIGHT_EQ_GAIN_5000HZ): number.number_schema(
            RightEqGain5000hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_RIGHT_EQ_GAIN_8000HZ): number.number_schema(
            RightEqGain8000hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_RIGHT_EQ_GAIN_16000HZ): number.number_schema(
            RightEqGain16000hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),
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

    if left_gain_20hz_config := config.get(CONF_LEFT_EQ_GAIN_20HZ):
        cg.add_define("USE_TAS58XX_EQ_GAINS")
        n = await number.new_number(
           left_gain_20hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, left_gain_20hz_config)
        await cg.register_parented(n, tas58xx_component)

    if left_gain_31p5hz_config := config.get(CONF_LEFT_EQ_GAIN_31P5HZ):
        n = await number.new_number(
            left_gain_31p5hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, left_gain_31p5hz_config)
        await cg.register_parented(n, tas58xx_component)

    if left_gain_50hz_config := config.get(CONF_LEFT_EQ_GAIN_50HZ):
        n = await number.new_number(
            left_gain_50hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, left_gain_50hz_config)
        await cg.register_parented(n, tas58xx_component)

    if left_gain_80hz_config := config.get(CONF_LEFT_EQ_GAIN_80HZ):
        n = await number.new_number(
            left_gain_80hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, left_gain_80hz_config)
        await cg.register_parented(n, tas58xx_component)

    if left_gain_125hz_config := config.get(CONF_LEFT_EQ_GAIN_125HZ):
        n = await number.new_number(
            left_gain_125hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, left_gain_125hz_config)
        await cg.register_parented(n, tas58xx_component)

    if left_gain_200hz_config := config.get(CONF_LEFT_EQ_GAIN_200HZ):
        n = await number.new_number(
            left_gain_200hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, left_gain_200hz_config)
        await cg.register_parented(n, tas58xx_component)

    if left_gain_315hz_config := config.get(CONF_LEFT_EQ_GAIN_315HZ):
        n = await number.new_number(
            left_gain_315hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, left_gain_315hz_config)
        await cg.register_parented(n, tas58xx_component)

    if left_gain_500hz_config := config.get(CONF_LEFT_EQ_GAIN_500HZ):
        n = await number.new_number(
            left_gain_500hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, left_gain_500hz_config)
        await cg.register_parented(n, tas58xx_component)

    if left_gain_800hz_config := config.get(CONF_LEFT_EQ_GAIN_800HZ):
        n = await number.new_number(
            left_gain_800hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, left_gain_800hz_config)
        await cg.register_parented(n, tas58xx_component)

    if left_gain_1250hz_config := config.get(CONF_LEFT_EQ_GAIN_1250HZ):
        n = await number.new_number(
            left_gain_1250hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, left_gain_1250hz_config)
        await cg.register_parented(n, tas58xx_component)

    if left_gain_2000hz_config := config.get(CONF_LEFT_EQ_GAIN_2000HZ):
        n = await number.new_number(
            left_gain_2000hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, left_gain_2000hz_config)
        await cg.register_parented(n, tas58xx_component)

    if left_gain_3150hz_config := config.get(CONF_LEFT_EQ_GAIN_3150HZ):
        n = await number.new_number(
            left_gain_3150hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, left_gain_3150hz_config)
        await cg.register_parented(n, tas58xx_component)

    if left_gain_5000hz_config := config.get(CONF_LEFT_EQ_GAIN_5000HZ):
        n = await number.new_number(
            left_gain_5000hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, left_gain_5000hz_config)
        await cg.register_parented(n, tas58xx_component)


    if left_gain_8000hz_config := config.get(CONF_LEFT_EQ_GAIN_8000HZ):
        n = await number.new_number(
            left_gain_8000hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, left_gain_8000hz_config)
        await cg.register_parented(n, tas58xx_component)

    if left_gain_16000hz_config := config.get(CONF_LEFT_EQ_GAIN_16000HZ):
        n = await number.new_number(
            left_gain_16000hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, left_gain_16000hz_config)
        await cg.register_parented(n, tas58xx_component)

    # Right Eq Gain - EQ BiAMP
    if right_gain_20hz_config := config.get(CONF_RIGHT_EQ_GAIN_20HZ):
        cg.add_define("USE_TAS58XX_EQ_BIAMP")
        n = await number.new_number(
           right_gain_20hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, right_gain_20hz_config)
        await cg.register_parented(n, tas58xx_component)

    if right_gain_31p5hz_config := config.get(CONF_RIGHT_EQ_GAIN_31P5HZ):

        n = await number.new_number(
            right_gain_31p5hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, right_gain_31p5hz_config)
        await cg.register_parented(n, tas58xx_component)

    if right_gain_50hz_config := config.get(CONF_RIGHT_EQ_GAIN_50HZ):
        n = await number.new_number(
            right_gain_50hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, right_gain_50hz_config)
        await cg.register_parented(n, tas58xx_component)

    if right_gain_80hz_config := config.get(CONF_RIGHT_EQ_GAIN_80HZ):
        n = await number.new_number(
            right_gain_80hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, right_gain_80hz_config)
        await cg.register_parented(n, tas58xx_component)

    if right_gain_125hz_config := config.get(CONF_RIGHT_EQ_GAIN_125HZ):
        n = await number.new_number(
            right_gain_125hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, right_gain_125hz_config)
        await cg.register_parented(n, tas58xx_component)

    if right_gain_200hz_config := config.get(CONF_RIGHT_EQ_GAIN_200HZ):
        n = await number.new_number(
            right_gain_200hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, right_gain_200hz_config)
        await cg.register_parented(n, tas58xx_component)

    if right_gain_315hz_config := config.get(CONF_RIGHT_EQ_GAIN_315HZ):
        n = await number.new_number(
            right_gain_315hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, right_gain_315hz_config)
        await cg.register_parented(n, tas58xx_component)

    if right_gain_500hz_config := config.get(CONF_RIGHT_EQ_GAIN_500HZ):
        n = await number.new_number(
            right_gain_500hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, right_gain_500hz_config)
        await cg.register_parented(n, tas58xx_component)

    if right_gain_800hz_config := config.get(CONF_RIGHT_EQ_GAIN_800HZ):
        n = await number.new_number(
            right_gain_800hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, right_gain_800hz_config)
        await cg.register_parented(n, tas58xx_component)

    if right_gain_1250hz_config := config.get(CONF_RIGHT_EQ_GAIN_1250HZ):
        n = await number.new_number(
            right_gain_1250hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, right_gain_1250hz_config)
        await cg.register_parented(n, tas58xx_component)

    if right_gain_2000hz_config := config.get(CONF_RIGHT_EQ_GAIN_2000HZ):
        n = await number.new_number(
            right_gain_2000hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, right_gain_2000hz_config)
        await cg.register_parented(n, tas58xx_component)

    if right_gain_3150hz_config := config.get(CONF_RIGHT_EQ_GAIN_3150HZ):
        n = await number.new_number(
            right_gain_3150hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, right_gain_3150hz_config)
        await cg.register_parented(n, tas58xx_component)

    if right_gain_5000hz_config := config.get(CONF_RIGHT_EQ_GAIN_5000HZ):
        n = await number.new_number(
            right_gain_5000hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, right_gain_5000hz_config)
        await cg.register_parented(n, tas58xx_component)


    if right_gain_8000hz_config := config.get(CONF_RIGHT_EQ_GAIN_8000HZ):
        n = await number.new_number(
            right_gain_8000hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, right_gain_8000hz_config)
        await cg.register_parented(n, tas58xx_component)

    if right_gain_16000hz_config := config.get(CONF_RIGHT_EQ_GAIN_16000HZ):
        n = await number.new_number(
            right_gain_16000hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, right_gain_16000hz_config)
        await cg.register_parented(n, tas58xx_component)
