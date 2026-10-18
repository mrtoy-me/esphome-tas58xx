import logging

import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
import esphome.final_validate as fv

from esphome.const import (
    CONF_FREQUENCY,
    DEVICE_CLASS_SOUND_PRESSURE,
    ENTITY_CATEGORY_CONFIG,
    UNIT_DECIBEL,
)

_LOGGER = logging.getLogger(__name__)

CONF_CHANNEL_VOLUME_LEFT = "channel_volume_left"
CONF_CHANNEL_VOLUME_RIGHT = "channel_volume_right"
CONF_GAIN_MIN = "gain_min"
CONF_GAIN_MAX = "gain_max"

ICON_VOLUME_SOURCE = "mdi:volume-source"

from ..audio_dac import (
    CHANNEL_ENUMS,
    CONF_DAC_MODE,
    CONF_EQ_FILTER,
    CONF_LEFT_BANDS,
    CONF_Q_FACTOR,
    CONF_RIGHT_BANDS,
    CONF_TAS58XX_ID,
    DAC_MODE_BTL,
    EQ_BAND_DEFAULT_Q,
    EQ_BAND_FILTERS,
    EQ_BAND_MAX_GAIN,
    EQ_BAND_MAX_GAIN_LIMITED,
    EQ_BAND_MAX_Q,
    EQ_BAND_MIN_GAIN,
    EQ_BAND_MIN_Q,
    EQ_FIXED_Q_FILTERS,
    EQ_GAIN_LIMITED_FILTERS,
    EQ_SHELF_Q,
    LEFT,
    RIGHT,
    Tas58xxComponent,
    eq_frequency,
    get_audio_dac_config,
    get_band_biquad,
    tas58xx_ns,
)

ChannelVolumeLeft = tas58xx_ns.class_("ChannelVolumeLeft", number.Number, cg.Component)
ChannelVolumeRight = tas58xx_ns.class_("ChannelVolumeRight", number.Number, cg.Component)
EqGainNumber = tas58xx_ns.class_("EqGainNumber", number.Number, cg.Component)

BAND_KEYS = {LEFT: CONF_LEFT_BANDS, RIGHT: CONF_RIGHT_BANDS}


def _final_validate(config):
    full_conf = fv.full_config.get()

    this_number_id = config[CONF_TAS58XX_ID]

    # the audio_dac config declaring the ID used by this number
    # EQ checks (eq_mode select, bands with presets, biquad count) are done by the audio_dac final validation
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
            if config.get(CONF_RIGHT_BANDS):
                _LOGGER.warning("right_bands are not used when dac_mode is PBTL - they can be removed from YAML configuration")

    return config

FINAL_VALIDATE_SCHEMA = _final_validate


# the default Q depends on the filter; low_pass and high_pass have a fixed Q, so a q_factor there is ignored
def _validate_eq_band(config):
    eq_filter = config[CONF_EQ_FILTER]
    if eq_filter in EQ_FIXED_Q_FILTERS:
        if CONF_Q_FACTOR in config:
            _LOGGER.warning("q_factor is not used by eq_filter %s (fixed Q %s) - it can be removed from band '%s'",
                            eq_filter, EQ_SHELF_Q, config.get("name", ""))
        config[CONF_Q_FACTOR] = EQ_SHELF_Q
    elif CONF_Q_FACTOR not in config:
        config[CONF_Q_FACTOR] = EQ_BAND_DEFAULT_Q[eq_filter]
    return config


def _eq_band_schema():
    return cv.All(
        number.number_schema(
            EqGainNumber,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA)
        .extend(
            {
                cv.Optional(CONF_EQ_FILTER, default="equalizer"): cv.one_of(*EQ_BAND_FILTERS, lower=True),
                cv.Required(CONF_FREQUENCY): eq_frequency,
                cv.Optional(CONF_Q_FACTOR): cv.float_range(min=EQ_BAND_MIN_Q, max=EQ_BAND_MAX_Q),
                # limits for this band, checked against the filter's full range in _resolve_gain_limits
                cv.Optional(CONF_GAIN_MIN): cv.All(cv.decibel, cv.int_range(EQ_BAND_MIN_GAIN, EQ_BAND_MAX_GAIN)),
                cv.Optional(CONF_GAIN_MAX): cv.All(cv.decibel, cv.int_range(EQ_BAND_MIN_GAIN, EQ_BAND_MAX_GAIN)),
            }
        ),
        _validate_eq_band,
    )

def _filter_max_gain(eq_filter):
    return EQ_BAND_MAX_GAIN_LIMITED if eq_filter in EQ_GAIN_LIMITED_FILTERS else EQ_BAND_MAX_GAIN


# sets gain_min / gain_max of every band: the band's own value, else the platform value, else the filter's full range
def _resolve_gain_limits(config):
    platform_min = config.get(CONF_GAIN_MIN)
    platform_max = config.get(CONF_GAIN_MAX)
    capped_names = []
    for key in BAND_KEYS.values():
        for band_config in config.get(key, []):
            name = band_config.get("name", "")
            full_max = _filter_max_gain(band_config[CONF_EQ_FILTER])
            for conf in (CONF_GAIN_MIN, CONF_GAIN_MAX):
                value = band_config.get(conf)
                if value is not None and not EQ_BAND_MIN_GAIN <= value <= full_max:
                    raise cv.Invalid(f"{conf} of band '{name}' must be from {EQ_BAND_MIN_GAIN} dB to {full_max} dB "
                                     f"for eq_filter {band_config[CONF_EQ_FILTER]}, got {value} dB")

            gain_min = band_config.get(CONF_GAIN_MIN, platform_min if platform_min is not None else EQ_BAND_MIN_GAIN)
            if CONF_GAIN_MAX in band_config:
                gain_max = band_config[CONF_GAIN_MAX]
            elif platform_max is not None and platform_max > full_max:
                gain_max = full_max
                capped_names.append(f"'{name}'")
            else:
                gain_max = platform_max if platform_max is not None else full_max

            if gain_min >= gain_max:
                raise cv.Invalid(f"gain_min ({gain_min} dB) must be less than gain_max ({gain_max} dB) "
                                 f"for band '{name}'")
            band_config[CONF_GAIN_MIN] = gain_min
            band_config[CONF_GAIN_MAX] = gain_max

    if capped_names:
        _LOGGER.warning("gain_max %s dB is above +%s dB, the limit for high_shelf / high_pass bands - "
                        "+%s dB is used as max for %s", platform_max, EQ_BAND_MAX_GAIN_LIMITED,
                        EQ_BAND_MAX_GAIN_LIMITED, ", ".join(capped_names))
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
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

        # EQ bands: one gain number per entry, in the biquad allocated by audio_dac.py
        cv.Optional(CONF_LEFT_BANDS): cv.ensure_list(_eq_band_schema()),
        cv.Optional(CONF_RIGHT_BANDS): cv.ensure_list(_eq_band_schema()),

        # slider range limits for every band of this block (not the channel volumes); a band can override them
        cv.Optional(CONF_GAIN_MIN): cv.All(cv.decibel, cv.int_range(EQ_BAND_MIN_GAIN, EQ_BAND_MAX_GAIN)),
        cv.Optional(CONF_GAIN_MAX): cv.All(cv.decibel, cv.int_range(EQ_BAND_MIN_GAIN, EQ_BAND_MAX_GAIN)),
    }
    ),
    _resolve_gain_limits,
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

    # left bands, then right bands; the filter of each band is added to the hub by audio_dac to_code
    for channel, key in BAND_KEYS.items():
        for list_index, band_config in enumerate(config.get(key, [])):
            cg.add_define("USE_TAS58XX_EQ_GAINS")
            biquad_index = get_band_biquad(config[CONF_TAS58XX_ID], channel, list_index)
            n = await number.new_number(
                band_config, CHANNEL_ENUMS[channel], biquad_index,
                min_value=band_config[CONF_GAIN_MIN], max_value=band_config[CONF_GAIN_MAX], step=1
            )
            await cg.register_component(n, band_config)
            await cg.register_parented(n, tas58xx_component)
