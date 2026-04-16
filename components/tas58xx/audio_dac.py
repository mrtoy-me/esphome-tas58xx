import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.core import CORE, ID
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
CONF_VOLUME_MIN = "volume_min"
CONF_VOLUME_MAX = "volume_max"
CONF_TAS58XX_ID = "tas58xx_id"
CONF_REDEFINE_EQ_FREQ = "redefine_eq_freq"

# used for looking through CORE.config to derive eq configuration
PLATFORM_TAS58XX = "tas58xx"
SELECT_COMPONENT = "select"

EQ_PRESET_LEFT_CHANNEL = "eq_preset_left_channel"
LEFT_EQ_GAIN_20HZ = "left_eq_gain_20Hz"
RIGHT_EQ_GAIN_20HZ = "right_eq_gain_20Hz"

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

EqRefreshMode = tas58xx_ns.enum("EqRefreshMode")
EQ_REFRESH_MODES = {
     "AUTO"  : EqRefreshMode.AUTO,
     "MANUAL": EqRefreshMode.MANUAL,
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
    if config[CONF_DAC_MODE] == "PBTL" and (config[CONF_MIXER_MODE] == "STEREO" or config[CONF_MIXER_MODE] == "STEREO_INVERSE"):
        raise cv.Invalid("dac_mode: PBTL must have mixer_mode: MONO or RIGHT or LEFT")
    if (config[CONF_VOLUME_MAX] - config[CONF_VOLUME_MIN]) < 9:
        raise cv.Invalid("volume_max must at least 9db greater than volume_min")
    return config

def validate_eq_freq(value):
    if not cv.ensure_list(cv.int_range(0, 16000))(value):
        raise cv.Invalid("frequency are not in correct range")
    if len(value) != 15:
        raise cv.Invalid("You must specify 15 frequencies")
    return value

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
            cv.Optional(CONF_REFRESH_EQ, default="AUTO"): cv.enum(
                        EQ_REFRESH_MODES, upper=True
            ),
            cv.Optional(CONF_VOLUME_MAX, default=24): cv.All(
                        cv.decibel, cv.int_range(-103, 24)
            ),
            cv.Optional(CONF_VOLUME_MIN, default=-103): cv.All(
                        cv.decibel, cv.int_range(-103, 24)
            ),
            cv.Optional(CONF_REDEFINE_EQ_FREQ): validate_eq_freq
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(i2c.i2c_device_schema(DUMMY_I2C_ADDR))
    .add_extra(validate_config),
    cv.only_on_esp32,
)

def get_configured_number_eq_gains(config):
    audio_dac_id = config.get(CONF_ID)
    all_numbers = CORE.config.get(CONF_NUMBER, [])
    for num in all_numbers:
        if num.get(CONF_PLATFORM) == PLATFORM_TAS58XX:
            if num.get(CONF_TAS58XX_ID) == audio_dac_id:
                return LEFT_EQ_GAIN_20HZ in num, RIGHT_EQ_GAIN_20HZ in num
    return False, False

def select_eq_presets_configured(config):
    audio_dac_id = config.get(CONF_ID)
    all_select = CORE.config.get(SELECT_COMPONENT, [])
    for select in all_select:
        if select.get(CONF_PLATFORM) == PLATFORM_TAS58XX:
            if select.get(CONF_TAS58XX_ID) == audio_dac_id:
                return EQ_PRESET_LEFT_CHANNEL in select
    return False

async def to_code(config):
    derived_eq_mode_configuration = EQ_OFF
    number_left_eq_gain_configured, number_right_eq_gain_configured = get_configured_number_eq_gains(config)
    if number_right_eq_gain_configured:
        derived_eq_mode_configuration  = EQ_BIAMP
    else:
        if number_left_eq_gain_configured:
            derived_eq_mode_configuration = EQ_15BAND
        else:
            if select_eq_presets_configured(config):
                derived_eq_mode_configuration = EQ_PRESETS

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
    cg.add(var.config_refresh_eq(config[CONF_REFRESH_EQ]))
    cg.add(var.config_volume_max(config[CONF_VOLUME_MAX]))
    cg.add(var.config_volume_min(config[CONF_VOLUME_MIN]))
    cg.add(var.config_eq_mode(derived_eq_mode_configuration))
    freq_data = config[CONF_REDEFINE_EQ_FREQ]
    freq_var_id = ID(
        f"eq_freq_config_{config[CONF_ID]}", is_declaration=True, type=cg.uint16
    )
    freq_var = cg.static_const_array(
        freq_var_id, cg.ArrayInitializer(*freq_data)
    )
    cg.add(var.add_eq_freq(freq_var, len(freq_data)))

    if tas58xx_dac == TAS5805M_DAC:
        cg.add_define("USE_TAS5805M_DAC")
    else:
        cg.add_define("USE_TAS5825M_DAC")
