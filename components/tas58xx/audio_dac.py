import logging
from collections import namedtuple

import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.core import CORE
from esphome.components import i2c
from esphome.components.audio_dac import AudioDac
from esphome import pins

from esphome.const import (
    CONF_ADDRESS,
    CONF_ENABLE_PIN,
    CONF_FREQUENCY,
    CONF_ID,
    CONF_NUMBER,
    CONF_PLATFORM,
)

_LOGGER = logging.getLogger(__name__)

from esphome.components.const import CONF_VOLUME_MAX, CONF_VOLUME_MIN

from . import DOMAIN

#MULTI_CONF = True
CODEOWNERS = ["@mrtoy-me"]
DEPENDENCIES = ["i2c"]

# yaml configuration constants
CONF_ANALOG_GAIN = "analog_gain"
CONF_DAC_MODE = "dac_mode"
CONF_EQ_MODE = "eq_mode"
CONF_MODULATION = "modulation"
CONF_TAS58XX_DAC = "tas58xx_dac"
CONF_IGNORE_FAULT = "ignore_fault"
CONF_MIXER_MODE = "mixer_mode"
CONF_REFRESH_EQ = "refresh_eq"
CONF_TAS58XX_ID = "tas58xx_id"

# used for looking through the full configuration to derive eq configuration
PLATFORM_TAS58XX = "tas58xx"
SELECT_COMPONENT = "select"

CONF_EQ_PRESET_LEFT_CHANNEL = "eq_preset_left_channel"
CONF_EQ_PRESET_RIGHT_CHANNEL = "eq_preset_right_channel"

# EQ bands (number platform lists, one gain number each) and fixed filters (audio_dac lists, no gain)
CONF_LEFT_BANDS = "left_bands"
CONF_RIGHT_BANDS = "right_bands"
CONF_LEFT_FIXED_FILTERS = "left_fixed_filters"
CONF_RIGHT_FIXED_FILTERS = "right_fixed_filters"
CONF_EQ_FILTER = "eq_filter"
CONF_Q_FACTOR = "q_factor"

# the 15 biquads of each channel
NUMBER_EQ_BIQUADS = 15
EQ_PRESET_BIQUADS = 3  # an EQ preset uses biquads 1 to 3

EQ_MIN_FREQUENCY = 20
EQ_MAX_FREQUENCY = 16000

# default Q of the band filters:
# equalizer and peaking: 2.144908, the constant Q of a 2/3-octave graphic EQ, Q = sqrt(2^N) / (2^N - 1), N = 2/3
# (the ISO 266 2/3-octave centres 25, 40, 63, ... 16000 Hz make a standard 15 band graphic EQ)
# shelves: 0.707, a shelf with no overshoot
# low_pass and high_pass are 2nd order Butterworth with a fixed Q of 0.707
EQ_GRAPHIC_Q = 2.144908
EQ_SHELF_Q = 0.707
EQ_BAND_MIN_Q = 0.1
EQ_BAND_MAX_Q = 20.0
# notch and band_pass: Q above 0.5 keeps the lower edge (frequency - frequency / (2Q)) above 0 Hz,
# Q at most 30 as 5.27 rounding detunes very narrow low frequency filters (a 20 Hz notch is -49 dB at Q 30)
EQ_FIXED_MIN_Q = 0.5
EQ_FIXED_MAX_Q = 30.0

DAC_MODE_BTL = "BTL"

# C++ EqMode values, also the index of the EQ Mode select option text
EQ_ON = 1
EQ_LEFT_ON = 2
EQ_RIGHT_ON = 3
EQ_BIAMP_ON = 4
EQ_MODE_OPTIONS = ("EQ Off", "EQ On Right=Left", "EQ Left On", "EQ Right On", "EQ BIAMP On")

# dac names
TAS5805M_DAC = "TAS5805M"
TAS5825M_DAC = "TAS5825M"

# i2c addresses of dac models
DUMMY_I2C_ADDR = 0x00
TAS5805M_I2C_ADDR = 0x2D
TAS5825M_I2C_ADDR = 0x4C

tas58xx_ns = cg.esphome_ns.namespace("tas58xx")
Tas58xxComponent = tas58xx_ns.class_("Tas58xxComponent", AudioDac, cg.PollingComponent, i2c.I2CDevice)

# refresh_eq is no longer used, it is accepted so existing YAML still validates
EQ_REFRESH_MODES = {
     "AUTO"  : "AUTO",
     "MANUAL": "MANUAL",
}

TasDac = tas58xx_ns.enum("TasDac")
TAS_DACS = {
    "TAS5805M" : TasDac.TAS5805M,
    "TAS5825M" : TasDac.TAS5825M,
}

DacMode = tas58xx_ns.enum("DacMode")
DAC_MODES = {
    "BTL"  : DacMode.BTL,
    "PBTL" : DacMode.PBTL,
}

ModulationScheme = tas58xx_ns.enum("ModulationScheme")
MODULATION_SCHEMES = {
    "BD_MODE"   : ModulationScheme.MODE_BD,
    "1SPW_MODE" : ModulationScheme.MODE_1SPW,
}

ExcludeIgnoreMode = tas58xx_ns.enum("ExcludeIgnoreModes")
EXCLUDE_IGNORE_MODES = {
     "NONE"        : ExcludeIgnoreMode.NONE,
     "CLOCK_FAULT" : ExcludeIgnoreMode.CLOCK_FAULT,
}

InputMixerMode = tas58xx_ns.enum("InputMixerMode")
INPUT_MIXER_MODES = {
    "STEREO"         : InputMixerMode.STEREO,
    "STEREO_INVERSE" : InputMixerMode.STEREO_INVERSE,
    "MONO"           : InputMixerMode.MONO,
    "RIGHT"          : InputMixerMode.RIGHT,
    "LEFT"           : InputMixerMode.LEFT,
}

FilterType = tas58xx_ns.enum("FilterType")
# filters with a gain input, used by the number platform EQ bands
EQ_BAND_FILTERS = {
    "equalizer": FilterType.FILTER_TYPE_EQUALIZER,
    "peaking": FilterType.FILTER_TYPE_PEAKING,
    "low_shelf": FilterType.FILTER_TYPE_LOW_SHELF,
    "high_shelf": FilterType.FILTER_TYPE_HIGH_SHELF,
    "low_pass": FilterType.FILTER_TYPE_LOW_PASS,
    "high_pass": FilterType.FILTER_TYPE_HIGH_PASS,
}
EQ_BAND_DEFAULT_Q = {
    "equalizer": EQ_GRAPHIC_Q,
    "peaking": EQ_GRAPHIC_Q,
    "low_shelf": EQ_SHELF_Q,
    "high_shelf": EQ_SHELF_Q,
}
EQ_FIXED_Q_FILTERS = ("low_pass", "high_pass")
# filters with no gain input, used by the audio_dac fixed filters
EQ_FIXED_FILTERS = {
    "notch": FilterType.FILTER_TYPE_NOTCH,
    "band_pass": FilterType.FILTER_TYPE_BAND_PASS,
}

# biquad order: filters in series give the same response in any order, only the level between biquads changes,
# so cuts come before filters that may boost (stable sort, YAML order is kept within a group)
EQ_FILTER_ORDER = {
    "high_pass": 0,
    "low_pass": 1, "notch": 1, "band_pass": 1,
    "low_shelf": 2, "high_shelf": 2, "equalizer": 2, "peaking": 2,
}


def eq_frequency(value):
    """A whole number of Hz from 20 Hz to 16 kHz, eg 80Hz or 1.25kHz."""
    value = cv.frequency(value)
    if value != round(value):
        raise cv.Invalid(f"frequency must be a whole number of Hz, got {value:g} Hz")
    value = int(round(value))
    if not EQ_MIN_FREQUENCY <= value <= EQ_MAX_FREQUENCY:
        raise cv.Invalid(f"frequency must be from {EQ_MIN_FREQUENCY} Hz to {EQ_MAX_FREQUENCY} Hz, got {value} Hz")
    return value


def _validate_fixed_q(value):
    value = cv.positive_float(value)
    if not EQ_FIXED_MIN_Q < value <= EQ_FIXED_MAX_Q:
        raise cv.Invalid(f"q_factor must be above {EQ_FIXED_MIN_Q} and at most {EQ_FIXED_MAX_Q:g}, got {value:g}")
    return value


FIXED_FILTER_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_EQ_FILTER): cv.one_of(*EQ_FIXED_FILTERS, lower=True),
        cv.Required(CONF_FREQUENCY): eq_frequency,
        cv.Required(CONF_Q_FACTOR): _validate_fixed_q,
    }
)

ANALOG_GAINS = [-15.5, -15, -14.5, -14, -13.5, -13, -12.5, -12, -11.5, -11, -10.5, -10, -9.5, -9, -8.5, -8,
                 -7.5,  -7,  -6.5,  -6,  -5.5,  -5,  -4.5,  -4,  -3.5,  -3,  -2.5,  -2, -1.5, -1, -0.5,  0]

def validate_config(config):
    if CONF_REFRESH_EQ in config:
        _LOGGER.warning("refresh_eq is no longer used and can be removed")
    if config[CONF_DAC_MODE] == "PBTL" and (config[CONF_MIXER_MODE] == "STEREO" or config[CONF_MIXER_MODE] == "STEREO_INVERSE"):
        raise cv.Invalid("dac_mode: PBTL must have mixer_mode: MONO or RIGHT or LEFT")
    if (config[CONF_VOLUME_MAX] - config[CONF_VOLUME_MIN]) < 9:
        raise cv.Invalid("volume_max must at least 9db greater than volume_min")
    return config

CONFIG_SCHEMA = cv.All(
    # also accept the keys used by ESPHome's own tas58xx, so the same YAML works with both
    cv.rename_key("model", CONF_TAS58XX_DAC),
    cv.rename_key("volume_min_db", CONF_VOLUME_MIN),
    cv.rename_key("volume_max_db", CONF_VOLUME_MAX),
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Tas58xxComponent),
            cv.Required(CONF_ENABLE_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_TAS58XX_DAC, default=TAS5805M_DAC): cv.enum(
                        TAS_DACS, upper=True
            ),
            cv.Optional(CONF_ANALOG_GAIN, default=-15.5): cv.All(
                        cv.decibel, cv.one_of(*ANALOG_GAINS)
            ),
            cv.Optional(CONF_DAC_MODE, default=DAC_MODE_BTL): cv.enum(
                        DAC_MODES, upper=True
            ),
            cv.Optional(CONF_MODULATION, default="BD_MODE"): cv.enum(
                        MODULATION_SCHEMES, upper=True
            ),
            cv.Optional(CONF_IGNORE_FAULT, default="CLOCK_FAULT"): cv.enum(
                        EXCLUDE_IGNORE_MODES, upper=True
            ),
            cv.Optional(CONF_MIXER_MODE, default="STEREO"): cv.enum(
                        INPUT_MIXER_MODES, upper=True
            ),
            cv.Optional(CONF_REFRESH_EQ): cv.enum(
                        EQ_REFRESH_MODES, upper=True
            ),
            cv.Optional(CONF_VOLUME_MAX, default=24): cv.All(
                        cv.decibel, cv.int_range(-103, 24)
            ),
            cv.Optional(CONF_VOLUME_MIN, default=-103): cv.All(
                        cv.decibel, cv.int_range(-103, 24)
            ),
            cv.Optional(CONF_LEFT_FIXED_FILTERS): cv.ensure_list(FIXED_FILTER_SCHEMA),
            cv.Optional(CONF_RIGHT_FIXED_FILTERS): cv.ensure_list(FIXED_FILTER_SCHEMA),
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(i2c.i2c_device_schema(DUMMY_I2C_ADDR))
    .add_extra(validate_config),
    cv.only_on_esp32,
    # on_audio_started() is used to write DSP settings once the I2S clock is running
    cv.require_esphome_version(2026, 10, 0),
)

# tas58xx platform config (eg number or select) of the given component for the audio_dac id, or None
def find_matching_config(full_conf, audio_dac_id, component):
    for conf in full_conf.get(component, []):
        if conf.get(CONF_PLATFORM) == PLATFORM_TAS58XX and conf.get(CONF_TAS58XX_ID) == audio_dac_id:
            return conf
    return None

# audio_dac config declaring the given id
def get_audio_dac_config(full_conf, audio_dac_id):
    return full_conf.get_config_for_path(full_conf.get_path_for_id(audio_dac_id)[:-1])

Channels = tas58xx_ns.enum("Channels")
LEFT, RIGHT = "left", "right"
CHANNEL_ENUMS = {LEFT: Channels.LEFT_CHANNEL, RIGHT: Channels.RIGHT_CHANNEL}
CHANNEL_KEYS = {
    LEFT: (CONF_LEFT_BANDS, CONF_LEFT_FIXED_FILTERS, CONF_EQ_PRESET_LEFT_CHANNEL),
    RIGHT: (CONF_RIGHT_BANDS, CONF_RIGHT_FIXED_FILTERS, CONF_EQ_PRESET_RIGHT_CHANNEL),
}

# what is configured for each channel: EQ bands (numbers), fixed filters (audio_dac) and/or an EQ preset select
EqChannels = namedtuple("EqChannels", "left_bands right_bands left_fixed right_fixed left_preset right_preset")

def get_eq_channels(number_conf, select_conf, audio_dac_conf):
    def present(conf, key):
        return conf is not None and bool(conf.get(key))
    return EqChannels(
        left_bands=present(number_conf, CONF_LEFT_BANDS),
        right_bands=present(number_conf, CONF_RIGHT_BANDS),
        left_fixed=present(audio_dac_conf, CONF_LEFT_FIXED_FILTERS),
        right_fixed=present(audio_dac_conf, CONF_RIGHT_FIXED_FILTERS),
        left_preset=present(select_conf, CONF_EQ_PRESET_LEFT_CHANNEL),
        right_preset=present(select_conf, CONF_EQ_PRESET_RIGHT_CHANNEL),
    )

def has_eq(eq_channels):
    return any(eq_channels)

# each channel has either EQ bands or an EQ preset, not both (fixed filters are allowed with either)
def validate_eq_channels(eq_channels):
    if eq_channels.left_bands and eq_channels.left_preset:
        raise cv.Invalid("Number left_bands are not allowed with Select eq_preset_left_channel - remove one of those configurations")
    if eq_channels.right_bands and eq_channels.right_preset:
        raise cv.Invalid("Number right_bands are not allowed with Select eq_preset_right_channel - remove one of those configurations")

# EQ modes that can be selected, in select option order after EQ Off
# EQ On Right=Left: the right channel uses the left channel biquads, so only the left bands and left fixed
# filters are used, for both channels (left bands or fixed filters only, a lone preset is not shared)
# EQ Left On, EQ Right On and EQ BIAMP On have separate filters for each channel; a channel that is not used is
# set flat: its bands and preset are set to all pass, its fixed filters stay
# PBTL has a single output from the left channel: the right channel is not used and nothing is shared, so the
# left channel is always EQ Left On
def derive_eq_modes(eq_channels, is_pbtl):
    left = eq_channels.left_bands or eq_channels.left_fixed or eq_channels.left_preset
    right = not is_pbtl and (eq_channels.right_bands or eq_channels.right_fixed or eq_channels.right_preset)
    if left and right:
        return (EQ_ON, EQ_LEFT_ON, EQ_RIGHT_ON, EQ_BIAMP_ON)
    if left:
        shareable = eq_channels.left_bands or eq_channels.left_fixed
        return (EQ_ON,) if shareable and not is_pbtl else (EQ_LEFT_ON,)
    if right:
        return (EQ_RIGHT_ON,)
    return ()

# One channel's biquads, sorted by filter group (cuts first) and packed from biquad 1, after a preset if there is
# one. Each entry: (fixed, list index, biquad index, filter, frequency, q_factor)
def allocate_eq_biquads(channel, bands, fixed_filters, has_preset):
    entries = [(False, i, conf) for i, conf in enumerate(bands)] + [(True, i, conf) for i, conf in enumerate(fixed_filters)]
    entries.sort(key=lambda entry: EQ_FILTER_ORDER[entry[2][CONF_EQ_FILTER]])  # stable, YAML order kept in a group
    first = EQ_PRESET_BIQUADS if has_preset else 0
    available = NUMBER_EQ_BIQUADS - first
    if len(entries) > available:
        raise cv.Invalid(
            f"{channel} channel has {len(bands)} EQ bands and {len(fixed_filters)} fixed filters, at most {available} "
            f"are allowed{' with an EQ preset' if has_preset else ''} - remove some of them"
        )
    return [
        (fixed, index, first + position, conf[CONF_EQ_FILTER], conf[CONF_FREQUENCY], conf[CONF_Q_FACTOR])
        for position, (fixed, index, conf) in enumerate(entries)
    ]

# derive the EQ modes and allocate the biquads from the tas58xx numbers, selects and fixed filters of this
# audio_dac, saved for audio_dac to_code, number to_code and select to_code
def _final_validate(config):
    full_conf = fv.full_config.get()
    audio_dac_id = config[CONF_ID]
    number_conf = find_matching_config(full_conf, audio_dac_id, CONF_NUMBER)
    select_conf = find_matching_config(full_conf, audio_dac_id, SELECT_COMPONENT)

    is_pbtl = config[CONF_DAC_MODE] != DAC_MODE_BTL
    eq_channels = get_eq_channels(number_conf, select_conf, config)
    validate_eq_channels(eq_channels)

    if has_eq(eq_channels) and (select_conf is None or CONF_EQ_MODE not in select_conf):
        raise cv.Invalid("Select eq_mode is required with Number left_bands / right_bands, fixed filters or EQ "
                         "Preset selects - add Select eq_mode to YAML configuration")
    if is_pbtl and eq_channels.right_fixed:
        _LOGGER.warning("right_fixed_filters are not used when dac_mode is PBTL - they can be removed from YAML configuration")

    biquads = {}
    for channel, (bands_key, fixed_key, preset_key) in CHANNEL_KEYS.items():
        biquads[channel] = allocate_eq_biquads(
            channel.capitalize(),
            (number_conf or {}).get(bands_key, []),
            config.get(fixed_key, []),
            select_conf is not None and preset_key in select_conf,
        )

    CORE.data.setdefault(DOMAIN, {})[str(audio_dac_id)] = {
        "eq_modes": derive_eq_modes(eq_channels, is_pbtl),
        "biquads": biquads,
    }
    return config

FINAL_VALIDATE_SCHEMA = _final_validate

def get_eq_modes(audio_dac_id):
    return CORE.data[DOMAIN][str(audio_dac_id)]["eq_modes"]

# the biquad allocated to the band at list_index of left_bands or right_bands
def get_band_biquad(audio_dac_id, channel, list_index):
    for fixed, index, biquad_index, *_ in CORE.data[DOMAIN][str(audio_dac_id)]["biquads"][channel]:
        if not fixed and index == list_index:
            return biquad_index
    raise cv.Invalid(f"no biquad allocated to {channel} band {list_index + 1}")

async def to_code(config):
    tas58xx_dac = config.get(CONF_TAS58XX_DAC)

    # when the user has not defined an audio dac i2c address
    # CONF_ADDRESS == DUMMY_I2C_ADDR
    # and it needs to be correctly assigned based on the defined tas58xx_dac
    if config[CONF_ADDRESS] == DUMMY_I2C_ADDR:
        if tas58xx_dac == TAS5805M_DAC:
            config[CONF_ADDRESS] = TAS5805M_I2C_ADDR
        else:
            config[CONF_ADDRESS] = TAS5825M_I2C_ADDR

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    enable = await cg.gpio_pin_expression(config[CONF_ENABLE_PIN])
    cg.add(var.set_enable_pin(enable))
    cg.add(var.config_analog_gain(config[CONF_ANALOG_GAIN]))
    cg.add(var.config_dac_mode(config[CONF_DAC_MODE]))
    cg.add(var.config_modulation_scheme(config[CONF_MODULATION]))
    cg.add(var.config_ignore_fault_mode(config[CONF_IGNORE_FAULT]))
    cg.add(var.config_input_mixer_mode(config[CONF_MIXER_MODE]))
    cg.add(var.config_volume_max(config[CONF_VOLUME_MAX]))
    cg.add(var.config_volume_min(config[CONF_VOLUME_MIN]))
    cg.add(var.config_eq_modes(sum(1 << mode for mode in get_eq_modes(config[CONF_ID]))))

    # every EQ band and fixed filter in the biquad allocated to it
    biquads = CORE.data[DOMAIN][str(config[CONF_ID])]["biquads"]
    if any(biquads.values()):
        cg.add_define("USE_TAS58XX_EQ_GAINS")
    for channel, entries in biquads.items():
        for fixed, _, biquad_index, eq_filter, frequency, q_factor in entries:
            filter_type = EQ_FIXED_FILTERS[eq_filter] if fixed else EQ_BAND_FILTERS[eq_filter]
            # Q as a double literal (a Python float is generated as a C++ float, which loses precision)
            q_literal = cg.RawExpression(repr(float(q_factor)))
            cg.add(var.add_eq_biquad(CHANNEL_ENUMS[channel], biquad_index, filter_type, frequency, q_literal, fixed))

    if tas58xx_dac == TAS5805M_DAC:
        cg.add_define("USE_TAS5805M_DAC")
    else:
        cg.add_define("USE_TAS5825M_DAC")
