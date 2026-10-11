import logging

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
CONF_MODULATION = "modulation"
CONF_TAS58XX_DAC = "tas58xx_dac"
CONF_IGNORE_FAULT = "ignore_fault"
CONF_MIXER_MODE = "mixer_mode"
CONF_REFRESH_EQ = "refresh_eq"
CONF_TAS58XX_ID = "tas58xx_id"

# used for looking through the full configuration to derive eq configuration
PLATFORM_TAS58XX = "tas58xx"
SELECT_COMPONENT = "select"

EQ_PRESET_LEFT_CHANNEL = "eq_preset_left_channel"

# eq gain number keys, eg left_eq_gain_20Hz
EQ_BAND_FREQUENCIES = ("20", "31.5", "50", "80", "125", "200", "315", "500", "800",
                       "1250", "2000", "3150", "5000", "8000", "16000")
CONF_LEFT_EQ_GAINS = tuple(f"left_eq_gain_{freq}Hz" for freq in EQ_BAND_FREQUENCIES)
CONF_RIGHT_EQ_GAINS = tuple(f"right_eq_gain_{freq}Hz" for freq in EQ_BAND_FREQUENCIES)

# eq mode enum and select index values
EQ_OFF = 0
EQ_15BAND = 1
EQ_BIAMP = 2
EQ_PRESETS = 3

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
            cv.Optional(CONF_DAC_MODE, default="BTL"): cv.enum(
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

def has_eq_gains(number_conf, conf_eq_gains):
    return number_conf is not None and any(key in number_conf for key in conf_eq_gains)

# derive the eq mode from the tas58xx numbers and selects of this audio_dac
# saved for audio_dac to_code and select to_code (EQ Mode select options)
def _final_validate(config):
    full_conf = fv.full_config.get()
    audio_dac_id = config[CONF_ID]
    number_conf = find_matching_config(full_conf, audio_dac_id, CONF_NUMBER)
    select_conf = find_matching_config(full_conf, audio_dac_id, SELECT_COMPONENT)

    eq_mode = EQ_OFF
    if has_eq_gains(number_conf, CONF_RIGHT_EQ_GAINS):
        eq_mode = EQ_BIAMP
    elif has_eq_gains(number_conf, CONF_LEFT_EQ_GAINS):
        eq_mode = EQ_15BAND
    elif select_conf is not None and EQ_PRESET_LEFT_CHANNEL in select_conf:
        eq_mode = EQ_PRESETS

    CORE.data.setdefault(DOMAIN, {})[str(audio_dac_id)] = eq_mode
    return config

FINAL_VALIDATE_SCHEMA = _final_validate

def get_eq_mode(audio_dac_id):
    return CORE.data[DOMAIN][str(audio_dac_id)]

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
    cg.add(var.config_eq_mode(get_eq_mode(config[CONF_ID])))

    if tas58xx_dac == TAS5805M_DAC:
        cg.add_define("USE_TAS5805M_DAC")
    else:
        cg.add_define("USE_TAS5825M_DAC")
