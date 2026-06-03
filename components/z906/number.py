import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_MAX_VALUE, CONF_MIN_VALUE, CONF_STEP

from . import CONF_Z906_ID, Z906Component, z906_ns

CONF_TYPE = "type"

Z906VolumeNumber = z906_ns.class_("Z906VolumeNumber", number.Number, cg.Component)
Z906RearNumber = z906_ns.class_("Z906RearNumber", number.Number, cg.Component)
Z906CenterNumber = z906_ns.class_("Z906CenterNumber", number.Number, cg.Component)
Z906SubNumber = z906_ns.class_("Z906SubNumber", number.Number, cg.Component)

TYPE_VOLUME = "volume"
TYPE_REAR = "rear"
TYPE_CENTER = "center"
TYPE_SUB = "sub"

TYPE_CLASSES = {
    TYPE_VOLUME: Z906VolumeNumber,
    TYPE_REAR: Z906RearNumber,
    TYPE_CENTER: Z906CenterNumber,
    TYPE_SUB: Z906SubNumber,
}


def _set_id_class(config):
    config = config.copy()
    config["id"].type = TYPE_CLASSES[config[CONF_TYPE]]
    return config


CONFIG_SCHEMA = cv.All(
    number.number_schema(number.Number).extend(
        {
            cv.GenerateID(CONF_Z906_ID): cv.use_id(Z906Component),
            cv.Required(CONF_TYPE): cv.enum({k: k for k in TYPE_CLASSES}, lower=True),
            cv.Optional(CONF_MIN_VALUE): cv.float_,
            cv.Optional(CONF_MAX_VALUE): cv.float_,
            cv.Optional(CONF_STEP): cv.positive_float,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _set_id_class,
)


async def to_code(config):
    var = cg.new_Pvariable(config["id"])
    await cg.register_component(var, config)
    await number.register_number(
        var,
        config,
        min_value=config.get(CONF_MIN_VALUE, 0.0),
        max_value=config.get(CONF_MAX_VALUE, 40.0),
        step=config.get(CONF_STEP, 1.0),
    )
    parent = await cg.get_variable(config[CONF_Z906_ID])
    cg.add(var.set_parent(parent))

    setter_map = {
        TYPE_VOLUME: "set_volume_number",
        TYPE_REAR: "set_rear_number",
        TYPE_CENTER: "set_center_number",
        TYPE_SUB: "set_sub_number",
    }
    cg.add(getattr(parent, setter_map[config[CONF_TYPE]])(var))
