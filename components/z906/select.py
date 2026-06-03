import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select

from . import CONF_Z906_ID, Z906Component, z906_ns

CONF_TYPE = "type"

Z906InputSelect = z906_ns.class_("Z906InputSelect", select.Select, cg.Component)
Z906EffectSelect = z906_ns.class_("Z906EffectSelect", select.Select, cg.Component)

TYPE_INPUT = "input"
TYPE_EFFECT = "effect"

TYPE_CONFIG = {
    TYPE_INPUT: (
        Z906InputSelect,
        ["Input 1", "Input 2", "Input 3", "Input 4", "Input 5", "AUX"],
    ),
    TYPE_EFFECT: (Z906EffectSelect, ["3D", "4.1", "2.1", "Off"]),
}


def _set_id_class(config):
    config = config.copy()
    config["id"].type = TYPE_CONFIG[config[CONF_TYPE]][0]
    return config


CONFIG_SCHEMA = cv.All(
    select.select_schema(select.Select).extend(
        {
            cv.GenerateID(CONF_Z906_ID): cv.use_id(Z906Component),
            cv.Required(CONF_TYPE): cv.enum({k: k for k in TYPE_CONFIG}, lower=True),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _set_id_class,
)


async def to_code(config):
    type_ = config[CONF_TYPE]
    options = TYPE_CONFIG[type_][1]
    var = cg.new_Pvariable(config["id"])
    await cg.register_component(var, config)
    await select.register_select(var, config, options=options)
    parent = await cg.get_variable(config[CONF_Z906_ID])
    cg.add(var.set_parent(parent))

    setter_map = {
        TYPE_INPUT: "set_input_select",
        TYPE_EFFECT: "set_effect_select",
    }
    cg.add(getattr(parent, setter_map[type_])(var))
