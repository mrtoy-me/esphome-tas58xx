import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import DEVICE_CLASS_SWITCH

CONF_ENABLE_DAC = "enable_dac"
# CONF_ENABLE_EQ = "enable_eq"

from ..audio_dac import CONF_TAS58XX_ID, Tas58xxComponent, tas58xx_ns

EnableDacSwitch = tas58xx_ns.class_("EnableDacSwitch", switch.Switch, cg.Component)
# EnableEqSwitch = tas58xx_ns.class_("EnableEqSwitch", switch.Switch , cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_TAS58XX_ID): cv.use_id(Tas58xxComponent),

        cv.Optional(CONF_ENABLE_DAC): switch.switch_schema(
            EnableDacSwitch,
            device_class=DEVICE_CLASS_SWITCH,
        )
        .extend(cv.COMPONENT_SCHEMA),

        # cv.Optional(CONF_ENABLE_EQ): switch.switch_schema(
        #     EnableEqSwitch,
        #     device_class=DEVICE_CLASS_SWITCH,
        # )
        # .extend(cv.COMPONENT_SCHEMA),
    }
)

async def to_code(config):
  tas58xx_component = await cg.get_variable(config[CONF_TAS58XX_ID])
  if enable_dac_config := config.get(CONF_ENABLE_DAC):
    s = await switch.new_switch(enable_dac_config)
    await cg.register_component(s, enable_dac_config)
    await cg.register_parented(s, tas58xx_component)

#   if enable_eq_config := config.get(CONF_ENABLE_EQ):
#     s = await switch.new_switch(enable_eq_config)
#     await cg.register_component(s, enable_eq_config)
#     await cg.register_parented(s, tas58xx_component)

