import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button

from . import CONF_Z906_ID, Z906Component, z906_ns

CONF_TYPE = "type"

Z906SaveEepromButton = z906_ns.class_(
    "Z906SaveEepromButton", button.Button, cg.Component
)
Z906ResetPowerUpTimeButton = z906_ns.class_(
    "Z906ResetPowerUpTimeButton", button.Button, cg.Component
)

TYPE_SAVE_EEPROM = "save_eeprom"
TYPE_RESET_POWER_UP_TIME = "reset_power_up_time"

TYPE_CLASSES = {
    TYPE_SAVE_EEPROM: Z906SaveEepromButton,
    TYPE_RESET_POWER_UP_TIME: Z906ResetPowerUpTimeButton,
}


def _set_id_class(config):
    config = config.copy()
    config["id"].type = TYPE_CLASSES[config[CONF_TYPE]]
    return config


CONFIG_SCHEMA = cv.All(
    button.button_schema(button.Button).extend(
        {
            cv.GenerateID(CONF_Z906_ID): cv.use_id(Z906Component),
            cv.Required(CONF_TYPE): cv.enum({k: k for k in TYPE_CLASSES}, lower=True),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _set_id_class,
)


async def to_code(config):
    var = cg.new_Pvariable(config["id"])
    await cg.register_component(var, config)
    await button.register_button(var, config)
    parent = await cg.get_variable(config[CONF_Z906_ID])
    cg.add(var.set_parent(parent))
