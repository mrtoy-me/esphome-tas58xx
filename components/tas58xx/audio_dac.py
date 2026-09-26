import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.core import CORE, ID
from esphome.components import i2c, i2s_audio #, speaker
from esphome.components.audio_dac import AudioDac
from esphome import pins

from esphome.const import (
    CONF_ADDRESS,
    CONF_ENABLE_PIN,
    CONF_ID,
)

CODEOWNERS = ["@mrtoy-me"]
DEPENDENCIES = ["i2c", "i2s_audio"]

# constant exclusively used in audio_dac schema
CONF_DAC_MODE = "dac_mode"
CONF_MIXER_MODE = "mixer_mode"
CONF_ANALOG_GAIN = "analog_gain"
CONF_MODULATION = "modulation"
CONF_TAS58XX_DAC = "tas58xx_dac"
CONF_VOLUME_MIN = "volume_min"
CONF_VOLUME_MAX = "volume_max"
CONF_TAS58XX_ID = "tas58xx_id"
CONF_I2S_AUDIO_ID = "i2s_audio_id"
CONF_I2S_DOUT_PIN = "i2s_dout_pin"

# dac names
TAS5805M_DAC = "TAS5805M"
TAS5825M_DAC = "TAS5825M"

# i2c addresses of dac models
TAS5805M_I2C_ADDR = 0x2D
TAS5825M_I2C_ADDR = 0x4C
PLACEHOLDER_I2C_ADDR = 0x00

tas58xx_ns = cg.esphome_ns.namespace("tas58xx")
Tas58xxComponent = tas58xx_ns.class_("Tas58xxComponent", AudioDac, cg.PollingComponent, i2c.I2CDevice, i2s_audio.I2SAudioOut)

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

# validated audio_dac config
def validate_config(config):
    if config[CONF_DAC_MODE] == "PBTL" and (config[CONF_MIXER_MODE] == "STEREO" or config[CONF_MIXER_MODE] == "STEREO_INVERSE"):
        raise cv.Invalid("dac_mode: PBTL must have mixer_mode: MONO or RIGHT or LEFT")
    if (config[CONF_VOLUME_MAX] - config[CONF_VOLUME_MIN]) < 9:
        raise cv.Invalid("volume_max must at least 9db greater than volume_min")
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Tas58xxComponent),
            cv.GenerateID(CONF_I2S_AUDIO_ID): cv.use_id(i2s_audio.I2SAudioComponent),
            cv.Required(CONF_I2S_DOUT_PIN): pins.internal_gpio_output_pin_number,
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
            cv.Optional(CONF_MIXER_MODE, default="STEREO"): cv.enum(
                        INPUT_MIXER_MODES, upper=True
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
    .extend(i2c.i2c_device_schema(PLACEHOLDER_I2C_ADDR))
    .add_extra(validate_config),
    cv.only_on_esp32,
)

async def to_code(config):
    tas58xx_dac = config.get(CONF_TAS58XX_DAC)

    if config[CONF_ADDRESS] == PLACEHOLDER_I2C_ADDR:
        if tas58xx_dac == TAS5805M_DAC:
            config[CONF_ADDRESS] = TAS5805M_I2C_ADDR
        else:
            config[CONF_ADDRESS] = TAS5825M_I2C_ADDR

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    enable = await cg.gpio_pin_expression(config[CONF_ENABLE_PIN])

    i2s_parent = await cg.get_variable(config[CONF_I2S_AUDIO_ID])
    cg.add(var.set_parent(i2s_parent))
    cg.add(var.set_dout_pin(config[CONF_I2S_DOUT_PIN]))


    cg.add(var.set_enable_pin(enable))
    cg.add(var.config_analog_gain(config[CONF_ANALOG_GAIN]))
    cg.add(var.config_dac_mode(config[CONF_DAC_MODE]))
    cg.add(var.config_input_mixer_mode(config[CONF_MIXER_MODE]))
    cg.add(var.config_volume_max(config[CONF_VOLUME_MAX]))
    cg.add(var.config_volume_min(config[CONF_VOLUME_MIN]))

    if tas58xx_dac == TAS5805M_DAC:
        cg.add_define("USE_TAS5805M_DAC")
    else:
        cg.add_define("USE_TAS5825M_DAC")
