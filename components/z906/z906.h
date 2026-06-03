#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "esphome/components/button/button.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {

class GPIOPin;

namespace adc {
class ADCSensor;
}  // namespace adc

namespace binary_sensor {
class BinarySensor;
}  // namespace binary_sensor

namespace z906 {

class Z906Component;

class Z906VolumeNumber : public number::Number, public Component {
 public:
  void set_parent(Z906Component *parent) { parent_ = parent; }

 protected:
  void control(float value) override;
  Z906Component *parent_{nullptr};
};

class Z906RearNumber : public number::Number, public Component {
 public:
  void set_parent(Z906Component *parent) { parent_ = parent; }

 protected:
  void control(float value) override;
  Z906Component *parent_{nullptr};
};

class Z906CenterNumber : public number::Number, public Component {
 public:
  void set_parent(Z906Component *parent) { parent_ = parent; }

 protected:
  void control(float value) override;
  Z906Component *parent_{nullptr};
};

class Z906SubNumber : public number::Number, public Component {
 public:
  void set_parent(Z906Component *parent) { parent_ = parent; }

 protected:
  void control(float value) override;
  Z906Component *parent_{nullptr};
};

class Z906InputSelect : public select::Select, public Component {
 public:
  void set_parent(Z906Component *parent) { parent_ = parent; }

 protected:
  void control(size_t index) override;
  Z906Component *parent_{nullptr};
};

class Z906EffectSelect : public select::Select, public Component {
 public:
  void set_parent(Z906Component *parent) { parent_ = parent; }

 protected:
  void control(size_t index) override;
  Z906Component *parent_{nullptr};
};

class Z906MuteSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(Z906Component *parent) { parent_ = parent; }
  bool assumed_state() override;

 protected:
  void write_state(bool state) override;
  Z906Component *parent_{nullptr};
};

class Z906InputsBlockedSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(Z906Component *parent) { parent_ = parent; }
  bool assumed_state() override;

 protected:
  void write_state(bool state) override;
  Z906Component *parent_{nullptr};
};

class Z906PowerSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(Z906Component *parent) { parent_ = parent; }
  bool assumed_state() override;

 protected:
  void write_state(bool state) override;
  Z906Component *parent_{nullptr};
};

class Z906SaveEepromButton : public button::Button, public Component {
 public:
  void set_parent(Z906Component *parent) { parent_ = parent; }

 protected:
  void press_action() override;
  Z906Component *parent_{nullptr};
};

class Z906ResetPowerUpTimeButton : public button::Button, public Component {
 public:
  void set_parent(Z906Component *parent) { parent_ = parent; }

 protected:
  void press_action() override;
  Z906Component *parent_{nullptr};
};

class Z906TemperatureSensor : public sensor::Sensor, public Component {
 public:
  void set_parent(Z906Component *parent) { parent_ = parent; }

 protected:
  Z906Component *parent_{nullptr};
};

class Z906VersionSensor : public sensor::Sensor, public Component {
 public:
  void set_parent(Z906Component *parent) { parent_ = parent; }

 protected:
  Z906Component *parent_{nullptr};
};

class Z906Component : public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void write_volume(uint8_t value);
  void write_rear(uint8_t value);
  void write_center(uint8_t value);
  void write_sub(uint8_t value);
  void set_input(size_t index);
  void set_effect(size_t index);
  void set_mute(bool state);
  void set_inputs_blocked(bool state);
  void save_eeprom();
  void reset_power_up_time();
  void set_power(bool state);

  bool is_mute_state_known() const { return mute_state_known_; }
  bool is_inputs_blocked_state_known() const { return inputs_blocked_state_known_; }
  bool is_power_state_known() const { return power_state_known_; }
  bool is_powered_on() const { return power_state_; }

  void set_volume_number(Z906VolumeNumber *number) { volume_number_ = number; }
  void set_rear_number(Z906RearNumber *number) { rear_number_ = number; }
  void set_center_number(Z906CenterNumber *number) { center_number_ = number; }
  void set_sub_number(Z906SubNumber *number) { sub_number_ = number; }
  void set_input_select(Z906InputSelect *select) { input_select_ = select; }
  void set_effect_select(Z906EffectSelect *select) { effect_select_ = select; }
  void set_mute_switch(Z906MuteSwitch *sw) { mute_switch_ = sw; }
  void set_inputs_blocked_switch(Z906InputsBlockedSwitch *sw) { inputs_blocked_switch_ = sw; }
  void set_temperature_sensor(Z906TemperatureSensor *sensor) { temperature_sensor_ = sensor; }
  void set_version_sensor(Z906VersionSensor *sensor) { version_sensor_ = sensor; }
  void set_power_switch(Z906PowerSwitch *sw) { power_switch_ = sw; }
  void set_led_voltage_sensor(adc::ADCSensor *sensor) { led_voltage_sensor_ = sensor; }
  void set_powered_on_binary_sensor(binary_sensor::BinarySensor *sensor) {
    powered_on_binary_sensor_ = sensor;
  }
  void set_power_pin(GPIOPin *pin) { power_pin_ = pin; }
  void set_power_threshold(float threshold) { power_threshold_ = threshold; }
  void set_power_pulse_duration(uint32_t duration_ms) { power_pulse_duration_ms_ = duration_ms; }
  void set_power_refresh_delay(uint32_t delay_ms) { power_refresh_delay_ms_ = delay_ms; }

 protected:
  static constexpr uint32_t REQUEST_TIMEOUT_MS = 1000;
  static constexpr uint32_t REQUEST_GAP_MS = 50;
  static constexpr uint32_t TEMP_POLL_INTERVAL_MS = 10000;
  static constexpr uint32_t INVALID_STATUS_LOG_INTERVAL_MS = 5000;
  static constexpr uint32_t LEVEL_STEP_INTERVAL_MS = 1;
  static constexpr uint8_t STABLE_INVALID_STATUS_THRESHOLD = 2;

  static constexpr size_t ACK_TOTAL_LENGTH = 0x05;
  static constexpr size_t STATUS_TOTAL_LENGTH = 0x17;
  static constexpr size_t TEMP_TOTAL_LENGTH = 0x0A;

  static constexpr uint8_t EXP_STX = 0xAA;
  static constexpr uint8_t EXP_MODEL_STATUS = 0x0A;
  static constexpr uint8_t EXP_MODEL_TEMP = 0x0C;

  static constexpr uint8_t SELECT_INPUT_1 = 0x02;
  static constexpr uint8_t SELECT_INPUT_2 = 0x05;
  static constexpr uint8_t SELECT_INPUT_3 = 0x03;
  static constexpr uint8_t SELECT_INPUT_4 = 0x04;
  static constexpr uint8_t SELECT_INPUT_5 = 0x06;
  static constexpr uint8_t SELECT_INPUT_AUX = 0x07;

  static constexpr uint8_t LEVEL_MAIN_UP = 0x08;
  static constexpr uint8_t LEVEL_MAIN_DOWN = 0x09;
  static constexpr uint8_t LEVEL_SUB_UP = 0x0A;
  static constexpr uint8_t LEVEL_SUB_DOWN = 0x0B;
  static constexpr uint8_t LEVEL_CENTER_UP = 0x0C;
  static constexpr uint8_t LEVEL_CENTER_DOWN = 0x0D;
  static constexpr uint8_t LEVEL_REAR_UP = 0x0E;
  static constexpr uint8_t LEVEL_REAR_DOWN = 0x0F;

  static constexpr uint8_t PWM_OFF = 0x10;
  static constexpr uint8_t PWM_ON = 0x11;

  static constexpr uint8_t SELECT_EFFECT_3D = 0x14;
  static constexpr uint8_t SELECT_EFFECT_41 = 0x15;
  static constexpr uint8_t SELECT_EFFECT_21 = 0x16;
  static constexpr uint8_t SELECT_EFFECT_NO = 0x35;

  static constexpr uint8_t EEPROM_SAVE = 0x36;
  static constexpr uint8_t MUTE_ON = 0x38;
  static constexpr uint8_t MUTE_OFF = 0x39;
  static constexpr uint8_t BLOCK_INPUTS = 0x22;
  static constexpr uint8_t RESET_PWR_UP_TIME = 0x30;
  static constexpr uint8_t NO_BLOCK_INPUTS = 0x33;

  static constexpr uint8_t GET_TEMP = 0x25;
  static constexpr uint8_t GET_STATUS = 0x34;

  static constexpr uint8_t STATUS_STX = 0x00;
  static constexpr uint8_t STATUS_MODEL = 0x01;
  static constexpr uint8_t STATUS_LENGTH = 0x02;
  static constexpr uint8_t STATUS_MAIN_LEVEL = 0x03;
  static constexpr uint8_t STATUS_REAR_LEVEL = 0x04;
  static constexpr uint8_t STATUS_CENTER_LEVEL = 0x05;
  static constexpr uint8_t STATUS_SUB_LEVEL = 0x06;
  static constexpr uint8_t STATUS_CURRENT_INPUT = 0x07;
  static constexpr uint8_t STATUS_FX_INPUT_4 = 0x09;
  static constexpr uint8_t STATUS_FX_INPUT_5 = 0x0A;
  static constexpr uint8_t STATUS_FX_INPUT_2 = 0x0B;
  static constexpr uint8_t STATUS_FX_INPUT_AUX = 0x0C;
  static constexpr uint8_t STATUS_FX_INPUT_1 = 0x0D;
  static constexpr uint8_t STATUS_FX_INPUT_3 = 0x0E;
  static constexpr uint8_t STATUS_VER_A = 0x11;
  static constexpr uint8_t STATUS_VER_B = 0x12;
  static constexpr uint8_t STATUS_VER_C = 0x13;
  static constexpr uint8_t STATUS_CHECKSUM = 0x16;

  enum class PendingRequest : uint8_t {
    NONE,
    ACK,
    STATUS,
    TEMP,
  };

  std::array<uint8_t, STATUS_TOTAL_LENGTH> status_{};
  std::array<uint8_t, STATUS_TOTAL_LENGTH> frame_buffer_{};
  std::array<uint8_t, STATUS_TOTAL_LENGTH> last_invalid_status_frame_{};
  size_t frame_pos_{0};
  size_t expected_frame_length_{0};
  bool have_status_{false};
  bool have_last_invalid_status_frame_{false};
  bool ignore_status_checksum_{false};
  bool mute_state_known_{false};
  bool mute_state_{false};
  bool inputs_blocked_state_known_{false};
  bool inputs_blocked_state_{false};
  bool power_state_known_{false};
  bool power_state_{false};
  bool power_pulse_active_{false};
  bool power_refresh_pending_{false};
  bool pending_power_target_known_{false};
  bool pending_power_target_{false};
  bool status_refresh_pending_{true};
  PendingRequest pending_request_{PendingRequest::NONE};
  uint32_t pending_request_started_ms_{0};
  uint32_t last_status_request_ms_{0};
  uint32_t last_temp_request_ms_{0};
  uint32_t last_invalid_status_log_ms_{0};
  uint32_t last_level_step_command_ms_{0};
  uint32_t power_pulse_started_ms_{0};
  uint32_t power_refresh_due_ms_{0};
  uint32_t power_pulse_duration_ms_{200};
  uint32_t power_refresh_delay_ms_{1000};
  uint8_t consecutive_invalid_status_frames_{0};
  uint8_t repeated_invalid_status_frame_count_{0};
  uint8_t pending_level_status_index_{0};
  uint8_t pending_level_target_{0};
  uint8_t pending_level_step_command_{0};
  uint16_t pending_level_step_count_{0};
  float power_threshold_{0.45f};

  Z906VolumeNumber *volume_number_{nullptr};
  Z906RearNumber *rear_number_{nullptr};
  Z906CenterNumber *center_number_{nullptr};
  Z906SubNumber *sub_number_{nullptr};
  Z906InputSelect *input_select_{nullptr};
  Z906EffectSelect *effect_select_{nullptr};
  Z906MuteSwitch *mute_switch_{nullptr};
  Z906InputsBlockedSwitch *inputs_blocked_switch_{nullptr};
  Z906TemperatureSensor *temperature_sensor_{nullptr};
  Z906VersionSensor *version_sensor_{nullptr};
  Z906PowerSwitch *power_switch_{nullptr};
  adc::ADCSensor *led_voltage_sensor_{nullptr};
  binary_sensor::BinarySensor *powered_on_binary_sensor_{nullptr};
  GPIOPin *power_pin_{nullptr};

  uint8_t compute_lrc_(const uint8_t *data, size_t length) const;
  void process_uart_byte_(uint8_t byte);
  void handle_status_frame_();
  void handle_temp_frame_(const std::array<uint8_t, TEMP_TOTAL_LENGTH> &frame);
  void handle_event_byte_(uint8_t byte);
  void publish_status_state_();
  void publish_mute_state_(bool state);
  void publish_inputs_blocked_state_(bool state);
  void request_status_();
  void request_temperature_();
  bool write_level_(uint8_t status_index, uint8_t value);
  void send_single_command_(uint8_t command);
  void handle_led_voltage_state_(float voltage);
  void press_power_button_();
  bool is_ready_for_serial_() const;
  bool get_level_step_commands_(uint8_t status_index, uint8_t *up_command, uint8_t *down_command) const;
  void adjust_level_(uint8_t status_index, int8_t delta);
  bool input_index_valid_(size_t index) const;
  uint8_t effect_status_index_(size_t input_index) const;
  size_t effect_option_index_from_status_(uint8_t effect_code) const;
  uint8_t effect_status_code_from_option_(size_t option_index) const;
};

}  // namespace z906
}  // namespace esphome
