#include "esphome/components/adc/adc_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "z906.h"

namespace esphome {
namespace z906 {

static const char *const TAG = "z906";

namespace {

constexpr std::array<uint8_t, 6> INPUT_COMMANDS = {
  0x02,
  0x05,
  0x03,
  0x04,
  0x06,
  0x07,
};

constexpr std::array<uint8_t, 4> EFFECT_COMMANDS = {
  0x14,
  0x15,
  0x16,
  0x35,
};

constexpr std::array<uint8_t, 6> EFFECT_STATUS_INDICES = {
  0x0D,
  0x0B,
  0x0E,
  0x09,
  0x0A,
  0x0C,
};

uint8_t clamp_level(float value) {
  if (value <= 0.0f) {
    return 0;
  }
  if (value >= 255.0f) {
    return 255;
  }
  return static_cast<uint8_t>(std::lround(value));
}

}  // namespace

void Z906VolumeNumber::control(float value) {
  if (this->parent_ != nullptr) {
    this->parent_->write_volume(clamp_level(value));
  }
}

void Z906RearNumber::control(float value) {
  if (this->parent_ != nullptr) {
    this->parent_->write_rear(clamp_level(value));
  }
}

void Z906CenterNumber::control(float value) {
  if (this->parent_ != nullptr) {
    this->parent_->write_center(clamp_level(value));
  }
}

void Z906SubNumber::control(float value) {
  if (this->parent_ != nullptr) {
    this->parent_->write_sub(clamp_level(value));
  }
}

void Z906InputSelect::control(size_t index) {
  if (this->parent_ != nullptr) {
    this->parent_->set_input(index);
  }
}

void Z906EffectSelect::control(size_t index) {
  if (this->parent_ != nullptr) {
    this->parent_->set_effect(index);
  }
}

bool Z906MuteSwitch::assumed_state() {
  return this->parent_ == nullptr || !this->parent_->is_mute_state_known();
}

void Z906MuteSwitch::write_state(bool state) {
  if (this->parent_ != nullptr) {
    this->parent_->set_mute(state);
  }
}

bool Z906InputsBlockedSwitch::assumed_state() {
  return this->parent_ == nullptr || !this->parent_->is_inputs_blocked_state_known();
}

void Z906InputsBlockedSwitch::write_state(bool state) {
  if (this->parent_ != nullptr) {
    this->parent_->set_inputs_blocked(state);
  }
}

bool Z906PowerSwitch::assumed_state() {
  return this->parent_ == nullptr || !this->parent_->is_power_state_known();
}

void Z906PowerSwitch::write_state(bool state) {
  if (this->parent_ != nullptr) {
    this->parent_->set_power(state);
  }
}

void Z906SaveEepromButton::press_action() {
  if (this->parent_ != nullptr) {
    this->parent_->save_eeprom();
  }
}

void Z906ResetPowerUpTimeButton::press_action() {
  if (this->parent_ != nullptr) {
    this->parent_->reset_power_up_time();
  }
}

void Z906Component::setup() {
  this->status_refresh_pending_ = true;

  if (this->power_pin_ != nullptr) {
    this->power_pin_->setup();
    this->power_pin_->digital_write(false);
  }

  if (this->led_voltage_sensor_ != nullptr) {
    this->led_voltage_sensor_->add_on_state_callback(
        [this](float voltage) { this->handle_led_voltage_state_(voltage); });
    this->power_refresh_pending_ = true;
    this->power_refresh_due_ms_ = 0;
  }
}

void Z906Component::dump_config() {
  ESP_LOGCONFIG(TAG, "Z906 Component");
  this->check_uart_settings(57600, 1, uart::UART_CONFIG_PARITY_ODD, 8);
  if (this->power_pin_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Integrated power control enabled");
  }
}

void Z906Component::update() {
  if (this->power_pin_ != nullptr) {
    if (!this->power_state_known_) {
      this->power_refresh_pending_ = true;
      this->power_refresh_due_ms_ = 0;
      return;
    }
    if (!this->power_state_) {
      return;
    }
  }

  this->status_refresh_pending_ = true;
}

void Z906Component::loop() {
  const uint32_t now = millis();

  if (this->power_pulse_active_ && now - this->power_pulse_started_ms_ >= this->power_pulse_duration_ms_) {
    this->power_pin_->digital_write(false);
    this->power_pulse_active_ = false;
    this->power_refresh_pending_ = true;
    this->power_refresh_due_ms_ = now + this->power_refresh_delay_ms_;
  }

  if (this->power_refresh_pending_ && now >= this->power_refresh_due_ms_) {
    this->power_refresh_pending_ = false;
    if (this->led_voltage_sensor_ != nullptr) {
      this->led_voltage_sensor_->update();
    }
    this->status_refresh_pending_ = true;
  }

  if (this->pending_request_ != PendingRequest::NONE &&
      now - this->pending_request_started_ms_ > REQUEST_TIMEOUT_MS) {
    if (this->pending_request_ == PendingRequest::ACK) {
      ESP_LOGW(TAG, "Timed out waiting for a Z906 write acknowledgement");
    } else {
      ESP_LOGW(TAG, "Timed out waiting for a Z906 response");
    }
    this->pending_request_ = PendingRequest::NONE;
    this->frame_pos_ = 0;
    this->expected_frame_length_ = 0;
  }

  if (this->pending_request_ == PendingRequest::ACK) {
    if (this->available() >= ACK_TOTAL_LENGTH) {
      uint8_t ack[ACK_TOTAL_LENGTH];
      if (this->read_array(ack, sizeof(ack))) {
        this->pending_request_ = PendingRequest::NONE;
      }
    }
  } else if (this->pending_request_ == PendingRequest::TEMP) {
    if (this->available() >= TEMP_TOTAL_LENGTH) {
      std::array<uint8_t, TEMP_TOTAL_LENGTH> frame{};
      if (this->read_array(frame.data(), frame.size())) {
        this->handle_temp_frame_(frame);
      }
    }
  } else {
    while (this->available() > 0) {
      uint8_t byte;
      if (!this->read_byte(&byte)) {
        break;
      }
      this->process_uart_byte_(byte);
    }
  }

  if (this->pending_request_ != PendingRequest::NONE) {
    return;
  }

  if (this->power_pin_ != nullptr) {
    if (!this->power_state_known_) {
      return;
    }
    if (!this->power_state_) {
      return;
    }
  }

  if (this->pending_level_step_count_ > 0) {
    if (now - this->last_level_step_command_ms_ >= LEVEL_STEP_INTERVAL_MS) {
      this->write_byte(this->pending_level_step_command_);
      this->flush();
      this->last_level_step_command_ms_ = now;
      this->pending_level_step_count_--;
      if (this->pending_level_step_count_ == 0) {
        this->status_refresh_pending_ = true;
      }
    }
    return;
  }

  if (this->status_refresh_pending_ && now - this->last_status_request_ms_ >= REQUEST_GAP_MS) {
    this->request_status_();
    return;
  }

  if (this->temperature_sensor_ != nullptr &&
      now - this->last_temp_request_ms_ >= TEMP_POLL_INTERVAL_MS &&
      now - this->last_status_request_ms_ >= REQUEST_GAP_MS) {
    this->request_temperature_();
  }
}

void Z906Component::write_volume(uint8_t value) { this->write_level_(STATUS_MAIN_LEVEL, value); }

void Z906Component::write_rear(uint8_t value) { this->write_level_(STATUS_REAR_LEVEL, value); }

void Z906Component::write_center(uint8_t value) { this->write_level_(STATUS_CENTER_LEVEL, value); }

void Z906Component::write_sub(uint8_t value) { this->write_level_(STATUS_SUB_LEVEL, value); }

void Z906Component::set_power(bool state) {
  if (this->power_pin_ == nullptr) {
    ESP_LOGW(TAG, "Ignoring power request because no power control pin is configured");
    return;
  }

  if (this->power_state_known_ && this->power_state_ == state) {
    if (this->power_switch_ != nullptr) {
      this->power_switch_->publish_state(state);
    }
    return;
  }

  if (this->power_pulse_active_ || this->power_refresh_pending_) {
    ESP_LOGW(TAG, "Ignoring power request while another power change is still in progress");
    return;
  }

  if (!this->power_state_known_) {
    ESP_LOGW(TAG, "Power state is unknown; sending a toggle pulse and waiting for LED voltage feedback");
  }

  this->pending_power_target_known_ = true;
  this->pending_power_target_ = state;
  this->power_state_known_ = false;
  this->press_power_button_();
}

void Z906Component::set_input(size_t index) {
  if (!this->is_ready_for_serial_()) {
    ESP_LOGW(TAG, "Ignoring input change while the amplifier is powered off or the power state is unknown");
    return;
  }

  if (!this->input_index_valid_(index)) {
    ESP_LOGW(TAG, "Input index %u is out of range", static_cast<unsigned>(index));
    return;
  }

  if (this->have_status_) {
    this->status_[STATUS_CURRENT_INPUT] = static_cast<uint8_t>(index);
    this->publish_status_state_();
  } else if (this->input_select_ != nullptr) {
    this->input_select_->publish_state(index);
  }

  this->send_single_command_(INPUT_COMMANDS[index]);
}

void Z906Component::set_effect(size_t index) {
  if (!this->is_ready_for_serial_()) {
    ESP_LOGW(TAG, "Ignoring effect change while the amplifier is powered off or the power state is unknown");
    return;
  }

  if (index >= EFFECT_COMMANDS.size()) {
    ESP_LOGW(TAG, "Effect index %u is out of range", static_cast<unsigned>(index));
    return;
  }

  if (this->have_status_ && this->input_index_valid_(this->status_[STATUS_CURRENT_INPUT])) {
    this->status_[this->effect_status_index_(this->status_[STATUS_CURRENT_INPUT])] =
        this->effect_status_code_from_option_(index);
    this->publish_status_state_();
  } else if (this->effect_select_ != nullptr) {
    this->effect_select_->publish_state(index);
  }

  this->send_single_command_(EFFECT_COMMANDS[index]);
}

void Z906Component::set_mute(bool state) {
  if (!this->is_ready_for_serial_()) {
    ESP_LOGW(TAG, "Ignoring mute change while the amplifier is powered off or the power state is unknown");
    return;
  }

  this->publish_mute_state_(state);
  this->send_single_command_(state ? MUTE_ON : MUTE_OFF);
}

void Z906Component::set_inputs_blocked(bool state) {
  if (!this->is_ready_for_serial_()) {
    ESP_LOGW(TAG,
             "Ignoring input block change while the amplifier is powered off or the power state is unknown");
    return;
  }

  this->publish_inputs_blocked_state_(state);
  this->send_single_command_(state ? BLOCK_INPUTS : NO_BLOCK_INPUTS);
}

void Z906Component::save_eeprom() { this->send_single_command_(EEPROM_SAVE); }

void Z906Component::reset_power_up_time() { this->send_single_command_(RESET_PWR_UP_TIME); }

uint8_t Z906Component::compute_lrc_(const uint8_t *data, size_t length) const {
  uint8_t lrc = 0;
  for (size_t index = 1; index + 1 < length; index++) {
    lrc = static_cast<uint8_t>(lrc - data[index]);
  }
  return lrc;
}

void Z906Component::process_uart_byte_(uint8_t byte) {
  if (this->frame_pos_ == 0) {
    if (byte == EXP_STX) {
      this->frame_buffer_[0] = byte;
      this->frame_pos_ = 1;
      this->expected_frame_length_ = 0;
      return;
    }
    this->handle_event_byte_(byte);
    return;
  }

  if (this->frame_pos_ >= this->frame_buffer_.size()) {
    this->frame_pos_ = 0;
    this->expected_frame_length_ = 0;
  }

  this->frame_buffer_[this->frame_pos_++] = byte;

  if (this->frame_pos_ == 2) {
    if (this->frame_buffer_[STATUS_MODEL] == EXP_MODEL_STATUS) {
      this->expected_frame_length_ = STATUS_TOTAL_LENGTH;
    } else if (this->frame_buffer_[STATUS_MODEL] == EXP_STX) {
      this->frame_buffer_[0] = EXP_STX;
      this->frame_pos_ = 1;
      this->expected_frame_length_ = 0;
    } else {
      this->frame_pos_ = 0;
      this->expected_frame_length_ = 0;
    }
    return;
  }

  if (this->expected_frame_length_ != 0 && this->frame_pos_ == this->expected_frame_length_) {
    this->handle_status_frame_();
    this->frame_pos_ = 0;
    this->expected_frame_length_ = 0;
  }
}

void Z906Component::handle_status_frame_() {
  if (this->frame_buffer_[STATUS_STX] != EXP_STX ||
      this->frame_buffer_[STATUS_MODEL] != EXP_MODEL_STATUS) {
    ESP_LOGV(TAG, "Discarded malformed status frame");
    this->pending_request_ = PendingRequest::NONE;
    return;
  }

  const uint8_t computed_checksum = this->compute_lrc_(this->frame_buffer_.data(), STATUS_TOTAL_LENGTH);
  if (this->frame_buffer_[STATUS_CHECKSUM] != computed_checksum) {
    if (this->have_last_invalid_status_frame_ && this->last_invalid_status_frame_ == this->frame_buffer_) {
      this->repeated_invalid_status_frame_count_++;
    } else {
      this->last_invalid_status_frame_ = this->frame_buffer_;
      this->have_last_invalid_status_frame_ = true;
      this->repeated_invalid_status_frame_count_ = 1;
    }

    if (!this->ignore_status_checksum_ &&
        this->repeated_invalid_status_frame_count_ >= STABLE_INVALID_STATUS_THRESHOLD) {
      this->ignore_status_checksum_ = true;
      ESP_LOGW(TAG,
               "Accepting repeated Z906 status frames despite invalid checksum; this usually means the "
               "ESP8266 UART transport is corrupting the checksum byte");
    }

    if (this->ignore_status_checksum_) {
      this->status_ = this->frame_buffer_;
      this->status_[STATUS_CHECKSUM] = computed_checksum;
      this->have_status_ = true;
      this->pending_request_ = PendingRequest::NONE;
      this->consecutive_invalid_status_frames_ = 0;
      this->publish_status_state_();
      return;
    }

    this->consecutive_invalid_status_frames_++;
    const uint32_t now = millis();
    if (this->consecutive_invalid_status_frames_ == 1 ||
        now - this->last_invalid_status_log_ms_ >= INVALID_STATUS_LOG_INTERVAL_MS) {
      ESP_LOGW(TAG, "Discarded status frame with invalid checksum (count=%u): %s",
               this->consecutive_invalid_status_frames_,
               format_hex_pretty(this->frame_buffer_.data(), this->frame_buffer_.size()).c_str());
      this->last_invalid_status_log_ms_ = now;
    } else {
      ESP_LOGD(TAG, "Discarded status frame with invalid checksum: %s",
               format_hex_pretty(this->frame_buffer_.data(), this->frame_buffer_.size()).c_str());
    }
    this->pending_request_ = PendingRequest::NONE;
    return;
  }

  this->ignore_status_checksum_ = false;
  this->have_last_invalid_status_frame_ = false;
  this->repeated_invalid_status_frame_count_ = 0;
  if (this->consecutive_invalid_status_frames_ > 0) {
    ESP_LOGI(TAG, "Recovered Z906 status after %u invalid frame(s)",
             this->consecutive_invalid_status_frames_);
    this->consecutive_invalid_status_frames_ = 0;
  }

  this->status_ = this->frame_buffer_;
  this->have_status_ = true;
  this->pending_request_ = PendingRequest::NONE;
  this->publish_status_state_();
}

void Z906Component::handle_temp_frame_(const std::array<uint8_t, TEMP_TOTAL_LENGTH> &frame) {
  this->pending_request_ = PendingRequest::NONE;
  if (frame[2] != EXP_MODEL_TEMP) {
    ESP_LOGV(TAG, "Discarded malformed temperature frame");
    return;
  }

  if (this->temperature_sensor_ != nullptr) {
    this->temperature_sensor_->publish_state(frame[7]);
  }
}

void Z906Component::handle_event_byte_(uint8_t byte) {
  switch (byte) {
    case LEVEL_MAIN_UP:
      this->adjust_level_(STATUS_MAIN_LEVEL, 1);
      break;
    case LEVEL_MAIN_DOWN:
      this->adjust_level_(STATUS_MAIN_LEVEL, -1);
      break;
    case LEVEL_REAR_UP:
      this->adjust_level_(STATUS_REAR_LEVEL, 1);
      break;
    case LEVEL_REAR_DOWN:
      this->adjust_level_(STATUS_REAR_LEVEL, -1);
      break;
    case LEVEL_CENTER_UP:
      this->adjust_level_(STATUS_CENTER_LEVEL, 1);
      break;
    case LEVEL_CENTER_DOWN:
      this->adjust_level_(STATUS_CENTER_LEVEL, -1);
      break;
    case LEVEL_SUB_UP:
      this->adjust_level_(STATUS_SUB_LEVEL, 1);
      break;
    case LEVEL_SUB_DOWN:
      this->adjust_level_(STATUS_SUB_LEVEL, -1);
      break;
    case SELECT_INPUT_1:
      if (this->have_status_) {
        this->status_[STATUS_CURRENT_INPUT] = 0;
        this->publish_status_state_();
      }
      this->status_refresh_pending_ = true;
      break;
    case SELECT_INPUT_2:
      if (this->have_status_) {
        this->status_[STATUS_CURRENT_INPUT] = 1;
        this->publish_status_state_();
      }
      this->status_refresh_pending_ = true;
      break;
    case SELECT_INPUT_3:
      if (this->have_status_) {
        this->status_[STATUS_CURRENT_INPUT] = 2;
        this->publish_status_state_();
      }
      this->status_refresh_pending_ = true;
      break;
    case SELECT_INPUT_4:
      if (this->have_status_) {
        this->status_[STATUS_CURRENT_INPUT] = 3;
        this->publish_status_state_();
      }
      this->status_refresh_pending_ = true;
      break;
    case SELECT_INPUT_5:
      if (this->have_status_) {
        this->status_[STATUS_CURRENT_INPUT] = 4;
        this->publish_status_state_();
      }
      this->status_refresh_pending_ = true;
      break;
    case SELECT_INPUT_AUX:
      if (this->have_status_) {
        this->status_[STATUS_CURRENT_INPUT] = 5;
        this->publish_status_state_();
      }
      this->status_refresh_pending_ = true;
      break;
    case SELECT_EFFECT_3D:
      if (this->have_status_ && this->input_index_valid_(this->status_[STATUS_CURRENT_INPUT])) {
        this->status_[this->effect_status_index_(this->status_[STATUS_CURRENT_INPUT])] = 0x00;
        this->publish_status_state_();
      }
      this->status_refresh_pending_ = true;
      break;
    case SELECT_EFFECT_41:
      if (this->have_status_ && this->input_index_valid_(this->status_[STATUS_CURRENT_INPUT])) {
        this->status_[this->effect_status_index_(this->status_[STATUS_CURRENT_INPUT])] = 0x02;
        this->publish_status_state_();
      }
      this->status_refresh_pending_ = true;
      break;
    case SELECT_EFFECT_21:
      if (this->have_status_ && this->input_index_valid_(this->status_[STATUS_CURRENT_INPUT])) {
        this->status_[this->effect_status_index_(this->status_[STATUS_CURRENT_INPUT])] = 0x01;
        this->publish_status_state_();
      }
      this->status_refresh_pending_ = true;
      break;
    case SELECT_EFFECT_NO:
      if (this->have_status_ && this->input_index_valid_(this->status_[STATUS_CURRENT_INPUT])) {
        this->status_[this->effect_status_index_(this->status_[STATUS_CURRENT_INPUT])] = 0x03;
        this->publish_status_state_();
      }
      this->status_refresh_pending_ = true;
      break;
    case MUTE_ON:
      this->publish_mute_state_(true);
      break;
    case MUTE_OFF:
      this->publish_mute_state_(false);
      break;
    case BLOCK_INPUTS:
      this->publish_inputs_blocked_state_(true);
      break;
    case NO_BLOCK_INPUTS:
      this->publish_inputs_blocked_state_(false);
      break;
    case PWM_ON:
    case PWM_OFF:
    case EEPROM_SAVE:
    case RESET_PWR_UP_TIME:
      break;
    default:
      return;
  }
}

void Z906Component::publish_status_state_() {
  if (!this->have_status_) {
    return;
  }

  if (this->volume_number_ != nullptr) {
    this->volume_number_->publish_state(this->status_[STATUS_MAIN_LEVEL]);
  }
  if (this->rear_number_ != nullptr) {
    this->rear_number_->publish_state(this->status_[STATUS_REAR_LEVEL]);
  }
  if (this->center_number_ != nullptr) {
    this->center_number_->publish_state(this->status_[STATUS_CENTER_LEVEL]);
  }
  if (this->sub_number_ != nullptr) {
    this->sub_number_->publish_state(this->status_[STATUS_SUB_LEVEL]);
  }

  const uint8_t input_index = this->status_[STATUS_CURRENT_INPUT];
  if (this->input_select_ != nullptr && this->input_index_valid_(input_index)) {
    this->input_select_->publish_state(input_index);
  }

  if (this->effect_select_ != nullptr && this->input_index_valid_(input_index)) {
    const uint8_t effect_code = this->status_[this->effect_status_index_(input_index)];
    this->effect_select_->publish_state(this->effect_option_index_from_status_(effect_code));
  }

  if (this->version_sensor_ != nullptr) {
    const uint16_t version = this->status_[STATUS_VER_C] +
                             10 * this->status_[STATUS_VER_B] +
                             100 * this->status_[STATUS_VER_A];
    this->version_sensor_->publish_state(version);
  }
}

void Z906Component::publish_mute_state_(bool state) {
  this->mute_state_known_ = true;
  this->mute_state_ = state;
  if (this->mute_switch_ != nullptr) {
    this->mute_switch_->publish_state(state);
  }
}

void Z906Component::publish_inputs_blocked_state_(bool state) {
  this->inputs_blocked_state_known_ = true;
  this->inputs_blocked_state_ = state;
  if (this->inputs_blocked_switch_ != nullptr) {
    this->inputs_blocked_switch_->publish_state(state);
  }
}

void Z906Component::handle_led_voltage_state_(float voltage) {
  const bool powered_on = voltage < this->power_threshold_;

  this->power_state_known_ = true;
  this->power_state_ = powered_on;

  if (this->powered_on_binary_sensor_ != nullptr) {
    this->powered_on_binary_sensor_->publish_state(powered_on);
  }

  if (this->power_switch_ != nullptr) {
    this->power_switch_->publish_state(powered_on);
  }

  if (this->pending_power_target_known_) {
    if (this->pending_power_target_ != powered_on) {
      ESP_LOGW(TAG, "Power state after toggle does not match the requested target");
    }
    this->pending_power_target_known_ = false;
  }

  if (!powered_on) {
    this->have_status_ = false;
    this->mute_state_known_ = false;
    this->inputs_blocked_state_known_ = false;
    this->pending_request_ = PendingRequest::NONE;
    this->pending_level_step_count_ = 0;
    this->status_refresh_pending_ = false;
    this->frame_pos_ = 0;
    this->expected_frame_length_ = 0;
    return;
  }

  this->status_refresh_pending_ = true;
}

void Z906Component::request_status_() {
  this->write_byte(GET_STATUS);
  this->flush();
  this->status_refresh_pending_ = false;
  this->pending_request_ = PendingRequest::STATUS;
  this->pending_request_started_ms_ = millis();
  this->last_status_request_ms_ = this->pending_request_started_ms_;
  this->frame_pos_ = 0;
  this->expected_frame_length_ = 0;
}

void Z906Component::request_temperature_() {
  this->write_byte(GET_TEMP);
  this->flush();
  this->pending_request_ = PendingRequest::TEMP;
  this->pending_request_started_ms_ = millis();
  this->last_temp_request_ms_ = this->pending_request_started_ms_;
}

bool Z906Component::write_level_(uint8_t status_index, uint8_t value) {
  if (!this->is_ready_for_serial_()) {
    ESP_LOGW(TAG, "Ignoring level write while the amplifier is powered off or the power state is unknown");
    return false;
  }

  if (!this->have_status_) {
    ESP_LOGW(TAG, "Ignoring level write until the first status frame has been received");
    this->status_refresh_pending_ = true;
    return false;
  }

  uint8_t up_command;
  uint8_t down_command;
  if (!this->get_level_step_commands_(status_index, &up_command, &down_command)) {
    ESP_LOGW(TAG, "Unsupported level index %u", status_index);
    return false;
  }

  const int16_t current_value = this->status_[status_index];
  const int16_t target_value = value;
  const int16_t delta = target_value - current_value;
  if (delta == 0) {
    return true;
  }

  this->pending_level_status_index_ = status_index;
  this->pending_level_target_ = value;
  this->pending_level_step_command_ = delta > 0 ? up_command : down_command;
  this->pending_level_step_count_ = static_cast<uint16_t>(std::abs(delta));
  this->last_level_step_command_ms_ = 0;
  this->status_refresh_pending_ = false;
  return true;
}

void Z906Component::send_single_command_(uint8_t command) {
  if (!this->is_ready_for_serial_()) {
    ESP_LOGW(TAG, "Ignoring serial command while the amplifier is powered off or the power state is unknown");
    return;
  }

  this->write_byte(command);
  this->flush();
  this->status_refresh_pending_ = true;
}

void Z906Component::press_power_button_() {
  if (this->power_pin_ == nullptr) {
    return;
  }

  this->power_pin_->digital_write(true);
  this->power_pulse_active_ = true;
  this->power_pulse_started_ms_ = millis();
}

bool Z906Component::is_ready_for_serial_() const {
  return this->power_pin_ == nullptr || (this->power_state_known_ && this->power_state_);
}

bool Z906Component::get_level_step_commands_(uint8_t status_index, uint8_t *up_command,
                                             uint8_t *down_command) const {
  switch (status_index) {
    case STATUS_MAIN_LEVEL:
      *up_command = LEVEL_MAIN_UP;
      *down_command = LEVEL_MAIN_DOWN;
      return true;
    case STATUS_REAR_LEVEL:
      *up_command = LEVEL_REAR_UP;
      *down_command = LEVEL_REAR_DOWN;
      return true;
    case STATUS_CENTER_LEVEL:
      *up_command = LEVEL_CENTER_UP;
      *down_command = LEVEL_CENTER_DOWN;
      return true;
    case STATUS_SUB_LEVEL:
      *up_command = LEVEL_SUB_UP;
      *down_command = LEVEL_SUB_DOWN;
      return true;
    default:
      return false;
  }
}

void Z906Component::adjust_level_(uint8_t status_index, int8_t delta) {
  if (!this->have_status_) {
    this->status_refresh_pending_ = true;
    return;
  }

  int16_t value = this->status_[status_index];
  value += delta;
  if (value < 0) {
    value = 0;
  } else if (value > 255) {
    value = 255;
  }
  this->status_[status_index] = static_cast<uint8_t>(value);
  this->publish_status_state_();
  this->status_refresh_pending_ = true;
}

bool Z906Component::input_index_valid_(size_t index) const { return index < INPUT_COMMANDS.size(); }

uint8_t Z906Component::effect_status_index_(size_t input_index) const {
  if (input_index >= EFFECT_STATUS_INDICES.size()) {
    return EFFECT_STATUS_INDICES[0];
  }
  return EFFECT_STATUS_INDICES[input_index];
}

size_t Z906Component::effect_option_index_from_status_(uint8_t effect_code) const {
  switch (effect_code & 0x03) {
    case 0x00:
      return 0;
    case 0x02:
      return 1;
    case 0x01:
      return 2;
    default:
      return 3;
  }
}

uint8_t Z906Component::effect_status_code_from_option_(size_t option_index) const {
  switch (option_index) {
    case 0:
      return 0x00;
    case 1:
      return 0x02;
    case 2:
      return 0x01;
    default:
      return 0x03;
  }
}

}  // namespace z906
}  // namespace esphome
