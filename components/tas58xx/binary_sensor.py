import esphome.codegen as cg
import esphome.final_validate as fv
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_AUDIO_DAC,
    CONF_ID,
    CONF_PLATFORM,
    DEVICE_CLASS_PROBLEM,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

CONF_LEFT_CHANNEL_DC_FAULT = "left_channel_dc_fault"
CONF_RIGHT_CHANNEL_DC_FAULT = "right_channel_dc_fault"
CONF_LEFT_CHANNEL_OVER_CURRENT_FAULT = "left_channel_over_current_fault"
CONF_RIGHT_CHANNEL_OVER_CURRENT_FAULT = "right_channel_over_current_fault"
CONF_OTP_CRC_CHECK_ERROR = "otp_crc_check_error"
CONF_BQ_WRITE_FAILED = "bq_write_failed"
CONF_EEPROM_LOAD_ERROR = "eeprom_load_error"
CONF_PVDD_OVER_VOLTAGE_FAULT = "pcdd_over_voltage_fault"
CONF_PVDD_UNDER_VOLTAGE_FAULT = "pcdd_under_voltage_fault"
CONF_RIGHT_CHANNEL_CBC_CURRENT_FAULT = "right_channel_cbc_current_fault"
CONF_LEFT_CHANNEL_CBC_CURRENT_FAULT = "left_channel_cbc_current_fault"
CONF_OVER_TEMP_SHUTDOWN_FAULT = "over_temp_shutdown_fault"
CONF_LEFT_CHANNEL_CBC_CURRENT_WARNING = "left_channel_cbc_current_warning"
CONF_RIGHT_CHANNEL_CBC_CURRENT_WARNING = "right_channel_cbc_current_warning"
CONF_OVER_TEMP_146C_WARNING = "over_temp_146c_warning"
CONF_OVER_TEMP_134C_WARNING = "over_temp_134c_warning"
CONF_OVER_TEMP_122C_WARNING = "over_temp_122c_warning"
CONF_OVER_TEMP_112C_WARNING = "over_temp_112c_warning"

PLATFORM_TAS58XX = "tas58xx"
CONF_TAS58XX_DAC = "tas58xx_dac"
TAS5805M_DAC = "TAS5805M"

from .audio_dac import CONF_TAS58XX_ID, Tas58xxComponent

def _final_validate(config):
    full_conf = fv.full_config.get()
    binary_sensor_id = config[CONF_TAS58XX_ID]
    dac_confs = full_conf.get(CONF_AUDIO_DAC, [])
    for dac_conf in dac_confs:
        if dac_conf.get(CONF_PLATFORM) == PLATFORM_TAS58XX:
          if dac_conf.get(CONF_ID) == binary_sensor_id:
            if dac_conf.get(CONF_TAS58XX_DAC) == TAS5805M_DAC:
              tas5805_dac_has_tas5825_option = (
                CONF_EEPROM_LOAD_ERROR in config or
                CONF_RIGHT_CHANNEL_CBC_CURRENT_FAULT in config or
                CONF_LEFT_CHANNEL_CBC_CURRENT_FAULT in config or
                CONF_LEFT_CHANNEL_CBC_CURRENT_WARNING in config or
                CONF_RIGHT_CHANNEL_CBC_CURRENT_WARNING in config or
                CONF_OVER_TEMP_146C_WARNING in config or
                CONF_OVER_TEMP_122C_WARNING in config or
                CONF_OVER_TEMP_112C_WARNING in config
              )
              if tas5805_dac_has_tas5825_option:
                  raise cv.Invalid(
                    "TAS5805 DAC has one or more TAS5825 binary sensors - remove any TAS5825 only binary sensors from YAML "
                  )
            break
    return config

FINAL_VALIDATE_SCHEMA = _final_validate

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_TAS58XX_ID): cv.use_id(Tas58xxComponent),

    cv.Optional(CONF_LEFT_CHANNEL_DC_FAULT): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_RIGHT_CHANNEL_DC_FAULT): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_LEFT_CHANNEL_OVER_CURRENT_FAULT): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_RIGHT_CHANNEL_OVER_CURRENT_FAULT): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_OTP_CRC_CHECK_ERROR): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_BQ_WRITE_FAILED): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_EEPROM_LOAD_ERROR): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_PVDD_OVER_VOLTAGE_FAULT): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_PVDD_UNDER_VOLTAGE_FAULT): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_RIGHT_CHANNEL_CBC_CURRENT_FAULT): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_LEFT_CHANNEL_CBC_CURRENT_FAULT): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_OVER_TEMP_SHUTDOWN_FAULT): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_LEFT_CHANNEL_CBC_CURRENT_WARNING): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_RIGHT_CHANNEL_CBC_CURRENT_WARNING): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_OVER_TEMP_146C_WARNING): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_OVER_TEMP_134C_WARNING): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_OVER_TEMP_122C_WARNING): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_OVER_TEMP_112C_WARNING): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
}

async def to_code(config):
    cg.add_define("USE_TAS58XX_BINARY_SENSOR")
    tas58xx_component = await cg.get_variable(config[CONF_TAS58XX_ID])

    if has_fault_config := config.get(CONF_LEFT_CHANNEL_DC_FAULT):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_left_channel_dc_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_RIGHT_CHANNEL_DC_FAULT):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_right_channel_dc_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_LEFT_CHANNEL_OVER_CURRENT_FAULT):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_left_channel_over_current_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_RIGHT_CHANNEL_OVER_CURRENT_FAULT):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_right_channel_over_current_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_OTP_CRC_CHECK_ERROR):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_otp_crc_check_error_binary_sensor(sens))

    if has_fault_config := config.get(CONF_BQ_WRITE_FAILED):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_bq_write_failed_binary_sensor(sens))

    if has_fault_config := config.get(CONF_EEPROM_LOAD_ERROR):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_eeprom_load_error_binary_sensor(sens))

    if has_fault_config := config.get(CONF_PVDD_OVER_VOLTAGE_FAULT):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_pvdd_over_voltage_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_PVDD_UNDER_VOLTAGE_FAULT):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_pvdd_under_voltage_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_RIGHT_CHANNEL_CBC_CURRENT_FAULT):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_right_channel_cbc_current_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_LEFT_CHANNEL_CBC_CURRENT_FAULT):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_left_channel_cbc_current_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_OVER_TEMP_SHUTDOWN_FAULT):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_over_temperature_shutdown_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_LEFT_CHANNEL_CBC_CURRENT_WARNING):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_left_channel_cbc_current_warning_binary_sensor(sens))

    if has_fault_config := config.get(CONF_RIGHT_CHANNEL_CBC_CURRENT_WARNING):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_right_channel_cbc_current_warning_binary_sensor(sens))

    if has_fault_config := config.get(CONF_OVER_TEMP_146C_WARNING):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_over_temperature_146c_warning_binary_sensor(sens))

    if has_fault_config := config.get(CONF_OVER_TEMP_134C_WARNING):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_over_temperature_134c_warning_binary_sensor(sens))

    if has_fault_config := config.get(CONF_OVER_TEMP_122C_WARNING):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_over_temperature_122c_warning_binary_sensor(sens))

    if has_fault_config := config.get(CONF_OVER_TEMP_112C_WARNING):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas58xx_component.set_over_temperature_112c_warning_binary_sensor(sens))