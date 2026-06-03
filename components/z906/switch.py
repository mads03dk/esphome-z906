import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch

from . import CONF_Z906_ID, Z906Component, z906_ns

CONF_TYPE = "type"

Z906MuteSwitch = z906_ns.class_("Z906MuteSwitch", switch.Switch, cg.Component)
Z906InputsBlockedSwitch = z906_ns.class_(
    "Z906InputsBlockedSwitch", switch.Switch, cg.Component
)

TYPE_MUTE = "mute"
TYPE_INPUTS_BLOCKED = "inputs_blocked"

TYPE_CLASSES = {
    TYPE_MUTE: Z906MuteSwitch,
    TYPE_INPUTS_BLOCKED: Z906InputsBlockedSwitch,
}


def _set_id_class(config):
    config = config.copy()
    config["id"].type = TYPE_CLASSES[config[CONF_TYPE]]
    return config


CONFIG_SCHEMA = cv.All(
    switch.switch_schema(switch.Switch, default_restore_mode="DISABLED").extend(
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
    await switch.register_switch(var, config)
    parent = await cg.get_variable(config[CONF_Z906_ID])
    cg.add(var.set_parent(parent))

    setter_map = {
        TYPE_MUTE: "set_mute_switch",
        TYPE_INPUTS_BLOCKED: "set_inputs_blocked_switch",
    }
    cg.add(getattr(parent, setter_map[config[CONF_TYPE]])(var))
