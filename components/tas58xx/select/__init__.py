import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_CONFIG

from ..audio_dac import CONF_TAS58XX_ID, Tas58xxComponent, tas58xx_ns

EqModeSelect = tas58xx_ns.class_("EqModeSelect", select.Select, cg.Component)

CONF_EQ_MODE = "eq_mode"

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_TAS58XX_ID): cv.use_id(Tas58xxComponent),
    cv.Optional(CONF_EQ_MODE): select.select_schema(
        EqModeSelect,
        entity_category=ENTITY_CATEGORY_CONFIG,
    ),
}

async def to_code(config):
    tas58xx_component = await cg.get_variable(config[CONF_TAS58XX_ID])
    if eq_mode_config := config.get(CONF_EQ_MODE):
        s = await select.new_select(
            eq_mode_config,
            options=[],
        )
        await cg.register_component(s, eq_mode_config)
        await cg.register_parented(s, tas58xx_component)
