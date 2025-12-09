#include "handlers/physical.hpp"

volatile std::atomic<int32_t> PhysicalHandler::encoder_raw_value = 0;
volatile std::atomic<uint8_t> PhysicalHandler::encoder_last_encoded = 0;

void PhysicalHandler::updateEncoderISR() {
    // 高速ピン読み込み
    uint32_t gpio_state = GPIO6_PSR;
    uint8_t MSB = (gpio_state & ENC_A_MASK) ? 1 : 0;
    uint8_t LSB = (gpio_state & ENC_B_MASK) ? 1 : 0;

    uint8_t encoded = (MSB << 1) | LSB;
    uint8_t last_encoded = encoder_last_encoded.load(std::memory_order_relaxed);
    uint8_t sum = (last_encoded << 2) | encoded;

    switch(sum) {
        case 0b1101: case 0b0100: case 0b0010: case 0b1011:
            encoder_raw_value.fetch_add(1, std::memory_order_relaxed);
            break;
        case 0b1110: case 0b0111: case 0b0001: case 0b1000:
            encoder_raw_value.fetch_sub(1, std::memory_order_relaxed);
            break;
        default:
            break;
    }

    encoder_last_encoded.store(encoded, std::memory_order_relaxed);
}

void PhysicalHandler::init() {
    // ENC_AB
    pinMode(ENC_A_PIN, INPUT_PULLUP);
    pinMode(ENC_B_PIN, INPUT_PULLUP);

    uint32_t gpio_state = GPIO6_PSR;
    uint8_t MSB = (gpio_state & ENC_A_MASK) ? 1 : 0;
    uint8_t LSB = (gpio_state & ENC_B_MASK) ? 1 : 0;
    encoder_last_encoded.store((MSB << 1) | LSB, std::memory_order_relaxed);

    attachInterrupt(digitalPinToInterrupt(ENC_A_PIN), updateEncoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_B_PIN), updateEncoderISR, CHANGE);

    // BTN
    for (size_t i = 0; i < 7; ++i) {
        const auto& cfg = BUTTON_CONFIGS[i];
        pinMode(cfg.pin, cfg.active_high ? INPUT_PULLDOWN : INPUT_PULLUP);
    }
}

void PhysicalHandler::process_button(size_t btn_idx, uint32_t now) {
    const auto& cfg = BUTTON_CONFIGS[btn_idx];
    auto& state = button_states[btn_idx];

    // 高速ピン読み込み
    uint32_t gpio_state = GPIO6_PSR | GPIO7_PSR | GPIO9_PSR;
    bool is_active = ((gpio_state & cfg.pin_mask) != 0) == cfg.active_high;

    if(is_active) {
        // ボタンが押されている
        if(!state.is_pressed) {
            // 押下開始
            state.is_pressed = true;
            state.press_start_time = now;
            state.long_triggered = false;
        } else if(!state.long_triggered && (now - state.press_start_time > TIME_LONG_PRESS)) {
            // 長押し判定
            state_.setBtnState(cfg.id_long);
            state.long_triggered = true;
        }
    } else {
        // ボタンが押されていない
        if(state.is_pressed) {
            // チャタリング対策
            if(now - state.press_start_time > TIME_DEBOUNCE) {
                if(!state.long_triggered) {
                    // 短押し判定
                    state_.setBtnState(cfg.id_short);
                }
            }
            state.is_pressed = false;
        }
    }
}

void PhysicalHandler::process() {
    uint32_t now = millis();

    // エンコーダーのデバウンス処理（メインループで実行）
    if(now - last_encoder_debounce_time > TIME_ENCODER_DEBOUNCE) {
        int32_t delta = encoder_raw_value.exchange(0, std::memory_order_acq_rel);

        if(delta != 0) {
            //state_.setEncoderDelta(delta);
        }

        last_encoder_debounce_time = now;
    }

    // ボタン処理
    for(size_t i = 0; i < 7; ++i) {
        process_button(i, now);
    }
}