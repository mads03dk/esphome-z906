from esphome import pins
import esphome.codegen as cg
from esphome.components import (
    binary_sensor as binary_sensor_component,
    button as button_component,
    number as number_component,
    select as select_component,
    sensor as sensor_component,
    switch as switch_component,
    uart,
)
from esphome.components.adc import sensor as adc_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_INTERNAL,
    CONF_MAX_VALUE,
    CONF_MIN_VALUE,
    CONF_MODE,
    CONF_NAME,
    CONF_PIN,
    CONF_STEP,
    CONF_THRESHOLD,
    CONF_UPDATE_INTERVAL,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_POWER,
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)

CODEOWNERS = []
DEPENDENCIES = ["uart"]
AUTO_LOAD = [
    "adc",
    "binary_sensor",
    "button",
    "number",
    "select",
    "sensor",
    "switch",
    "voltage_sampler",
]
MULTI_CONF = True

z906_ns = cg.esphome_ns.namespace("z906")
Z906Component = z906_ns.class_("Z906Component", cg.PollingComponent, uart.UARTDevice)

CONF_Z906_ID = "z906_id"
CONF_VOLUME = "volume"
CONF_REAR = "rear"
CONF_CENTER = "center"
CONF_SUBWOOFER = "subwoofer"
CONF_INPUT = "input"
CONF_EFFECT = "effect"
CONF_MUTE = "mute"
CONF_INPUTS_BLOCKED = "inputs_blocked"
CONF_TEMPERATURE = "temperature"
CONF_VERSION = "version"
CONF_SAVE_EEPROM = "save_eeprom"
CONF_RESET_POWER_UP_TIMER = "reset_power_up_timer"
CONF_POWER = "power"
CONF_LED_VOLTAGE = "led_voltage"
CONF_POWERED_ON = "powered_on"
CONF_PULSE_DURATION = "pulse_duration"
CONF_REFRESH_DELAY = "refresh_delay"

from .button import Z906ResetPowerUpTimeButton, Z906SaveEepromButton
from .number import Z906CenterNumber, Z906RearNumber, Z906SubNumber, Z906VolumeNumber
from .select import Z906EffectSelect, Z906InputSelect
from .sensor import Z906TemperatureSensor, Z906VersionSensor
from .switch import Z906InputsBlockedSwitch, Z906MuteSwitch

INPUT_OPTIONS = ["Input 1", "Input 2", "Input 3", "Input 4", "Input 5", "AUX"]
EFFECT_OPTIONS = ["3D", "4.1", "2.1", "Off"]
Z906PowerSwitch = z906_ns.class_(
    "Z906PowerSwitch", switch_component.Switch, cg.Component
)


def _default_entity(name, **kwargs):
    return {CONF_NAME: name, **kwargs}


def _with_default_name(name):
    def validator(config):
        if CONF_NAME not in config and CONF_ID not in config:
            config = {**config, CONF_NAME: name}
        return config

    return validator


def _number_schema(class_):
    return number_component.number_schema(class_).extend(
        {
            cv.Optional(CONF_MIN_VALUE, default=0.0): cv.float_,
            cv.Optional(CONF_MAX_VALUE, default=40.0): cv.float_,
            cv.Optional(CONF_STEP, default=1.0): cv.positive_float,
        }
    )


POWER_SCHEMA = cv.All(
    _with_default_name("Power"),
    switch_component.switch_schema(
        Z906PowerSwitch,
        default_restore_mode="DISABLED",
    ).extend(
        {
            cv.Required(CONF_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_THRESHOLD, default=0.45): cv.float_,
            cv.Optional(
                CONF_PULSE_DURATION, default="200ms"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_REFRESH_DELAY, default="1s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_LED_VOLTAGE,
                default=_default_entity(
                    "LED Voltage",
                    **{
                        CONF_INTERNAL: True,
                        CONF_PIN: "A0",
                        CONF_UPDATE_INTERVAL: "3s",
                    },
                ),
            ): adc_sensor.CONFIG_SCHEMA,
            cv.Optional(
                CONF_POWERED_ON,
                default=_default_entity("Powered On", **{CONF_INTERNAL: True}),
            ): binary_sensor_component.binary_sensor_schema(
                binary_sensor_component.BinarySensor,
                device_class=DEVICE_CLASS_POWER,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    ),
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Z906Component),
            cv.Optional(
                CONF_VOLUME,
                default=_default_entity("Main Volume", **{CONF_MODE: "SLIDER"}),
            ): _number_schema(Z906VolumeNumber),
            cv.Optional(
                CONF_REAR,
                default=_default_entity("Rear Level", **{CONF_MODE: "SLIDER"}),
            ): _number_schema(Z906RearNumber),
            cv.Optional(
                CONF_CENTER,
                default=_default_entity("Center Level", **{CONF_MODE: "SLIDER"}),
            ): _number_schema(Z906CenterNumber),
            cv.Optional(
                CONF_SUBWOOFER,
                default=_default_entity("Subwoofer Level", **{CONF_MODE: "SLIDER"}),
            ): _number_schema(Z906SubNumber),
            cv.Optional(CONF_INPUT, default=_default_entity("Input")): select_component.select_schema(
                Z906InputSelect
            ),
            cv.Optional(CONF_EFFECT, default=_default_entity("Effect")): select_component.select_schema(
                Z906EffectSelect
            ),
            cv.Optional(CONF_MUTE, default=_default_entity("Mute")): switch_component.switch_schema(
                Z906MuteSwitch, default_restore_mode="DISABLED"
            ),
            cv.Optional(CONF_INPUTS_BLOCKED): cv.All(
                _with_default_name("Inputs Blocked"),
                switch_component.switch_schema(
                    Z906InputsBlockedSwitch,
                    default_restore_mode="DISABLED",
                    entity_category=ENTITY_CATEGORY_CONFIG,
                ),
            ),
            cv.Optional(
                CONF_TEMPERATURE,
                default=_default_entity("Temperature"),
            ): sensor_component.sensor_schema(
                Z906TemperatureSensor,
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_VERSION,
                default=_default_entity("Firmware Version"),
            ): sensor_component.sensor_schema(
                Z906VersionSensor,
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_SAVE_EEPROM): cv.All(
                _with_default_name("Save EEPROM"),
                button_component.button_schema(
                    Z906SaveEepromButton,
                    entity_category=ENTITY_CATEGORY_CONFIG,
                ),
            ),
            cv.Optional(CONF_RESET_POWER_UP_TIMER): cv.All(
                _with_default_name("Reset Power-Up Timer"),
                button_component.button_schema(
                    Z906ResetPowerUpTimeButton,
                    entity_category=ENTITY_CATEGORY_CONFIG,
                ),
            ),
            cv.Optional(CONF_POWER): POWER_SCHEMA,
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.polling_component_schema("1s"))
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "z906",
    baud_rate=57600,
    require_rx=True,
    require_tx=True,
    parity="ODD",
    stop_bits=1,
    data_bits=8,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    volume = await number_component.new_number(
        config[CONF_VOLUME],
        min_value=config[CONF_VOLUME][CONF_MIN_VALUE],
        max_value=config[CONF_VOLUME][CONF_MAX_VALUE],
        step=config[CONF_VOLUME][CONF_STEP],
    )
    cg.add(volume.set_parent(var))
    cg.add(var.set_volume_number(volume))

    rear = await number_component.new_number(
        config[CONF_REAR],
        min_value=config[CONF_REAR][CONF_MIN_VALUE],
        max_value=config[CONF_REAR][CONF_MAX_VALUE],
        step=config[CONF_REAR][CONF_STEP],
    )
    cg.add(rear.set_parent(var))
    cg.add(var.set_rear_number(rear))

    center = await number_component.new_number(
        config[CONF_CENTER],
        min_value=config[CONF_CENTER][CONF_MIN_VALUE],
        max_value=config[CONF_CENTER][CONF_MAX_VALUE],
        step=config[CONF_CENTER][CONF_STEP],
    )
    cg.add(center.set_parent(var))
    cg.add(var.set_center_number(center))

    subwoofer = await number_component.new_number(
        config[CONF_SUBWOOFER],
        min_value=config[CONF_SUBWOOFER][CONF_MIN_VALUE],
        max_value=config[CONF_SUBWOOFER][CONF_MAX_VALUE],
        step=config[CONF_SUBWOOFER][CONF_STEP],
    )
    cg.add(subwoofer.set_parent(var))
    cg.add(var.set_sub_number(subwoofer))

    input_select = await select_component.new_select(
        config[CONF_INPUT], options=INPUT_OPTIONS
    )
    cg.add(input_select.set_parent(var))
    cg.add(var.set_input_select(input_select))

    effect_select = await select_component.new_select(
        config[CONF_EFFECT], options=EFFECT_OPTIONS
    )
    cg.add(effect_select.set_parent(var))
    cg.add(var.set_effect_select(effect_select))

    mute_switch = await switch_component.new_switch(config[CONF_MUTE])
    cg.add(mute_switch.set_parent(var))
    cg.add(var.set_mute_switch(mute_switch))

    if (inputs_blocked_config := config.get(CONF_INPUTS_BLOCKED)) is not None:
        inputs_blocked_switch = await switch_component.new_switch(
            inputs_blocked_config
        )
        cg.add(inputs_blocked_switch.set_parent(var))
        cg.add(var.set_inputs_blocked_switch(inputs_blocked_switch))

    temperature_sensor = await sensor_component.new_sensor(config[CONF_TEMPERATURE])
    cg.add(temperature_sensor.set_parent(var))
    cg.add(var.set_temperature_sensor(temperature_sensor))

    version_sensor = await sensor_component.new_sensor(config[CONF_VERSION])
    cg.add(version_sensor.set_parent(var))
    cg.add(var.set_version_sensor(version_sensor))

    if (save_eeprom_config := config.get(CONF_SAVE_EEPROM)) is not None:
        save_eeprom_button = await button_component.new_button(save_eeprom_config)
        cg.add(save_eeprom_button.set_parent(var))

    if (reset_power_up_timer_config := config.get(CONF_RESET_POWER_UP_TIMER)) is not None:
        reset_power_up_timer_button = await button_component.new_button(
            reset_power_up_timer_config
        )
        cg.add(reset_power_up_timer_button.set_parent(var))

    if (power_config := config.get(CONF_POWER)) is not None:
        power_switch = await switch_component.new_switch(power_config)
        cg.add(power_switch.set_parent(var))
        cg.add(var.set_power_switch(power_switch))

        power_pin = await cg.gpio_pin_expression(power_config[CONF_PIN])
        cg.add(var.set_power_pin(power_pin))
        cg.add(var.set_power_threshold(power_config[CONF_THRESHOLD]))
        cg.add(
            var.set_power_pulse_duration(
                power_config[CONF_PULSE_DURATION].total_milliseconds
            )
        )
        cg.add(
            var.set_power_refresh_delay(
                power_config[CONF_REFRESH_DELAY].total_milliseconds
            )
        )

        await adc_sensor.to_code(power_config[CONF_LED_VOLTAGE])
        led_voltage_sensor = await cg.get_variable(
            power_config[CONF_LED_VOLTAGE][CONF_ID]
        )
        cg.add(var.set_led_voltage_sensor(led_voltage_sensor))

        powered_on_sensor = await binary_sensor_component.new_binary_sensor(
            power_config[CONF_POWERED_ON]
        )
        cg.add(var.set_powered_on_binary_sensor(powered_on_sensor))
