import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)

from . import CONF_Z906_ID, Z906Component, z906_ns

CONF_TYPE = "type"

Z906TemperatureSensor = z906_ns.class_(
    "Z906TemperatureSensor", sensor.Sensor, cg.Component
)
Z906VersionSensor = z906_ns.class_("Z906VersionSensor", sensor.Sensor, cg.Component)

TYPE_TEMPERATURE = "temperature"
TYPE_VERSION = "version"

CONFIG_SCHEMA = cv.typed_schema(
    {
        TYPE_TEMPERATURE: sensor.sensor_schema(
            Z906TemperatureSensor,
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        )
        .extend(
            {
                cv.GenerateID(CONF_Z906_ID): cv.use_id(Z906Component),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
        TYPE_VERSION: sensor.sensor_schema(
            Z906VersionSensor,
            accuracy_decimals=0,
        )
        .extend(
            {
                cv.GenerateID(CONF_Z906_ID): cv.use_id(Z906Component),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
    },
    key=CONF_TYPE,
)


async def to_code(config):
    type_ = config[CONF_TYPE]
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_Z906_ID])

    setter_map = {
        TYPE_TEMPERATURE: "set_temperature_sensor",
        TYPE_VERSION: "set_version_sensor",
    }
    cg.add(getattr(parent, setter_map[type_])(var))
