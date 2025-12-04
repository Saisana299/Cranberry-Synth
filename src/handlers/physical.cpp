#include "handlers/physical.hpp"

volatile int32_t PhysicalHandler::encoder_value = 0;
volatile uint8_t PhysicalHandler::last_encoded = 0;

void PhysicalHandler::updateEncoderISR() {
    uint8_t MSB = digitalRead(ENC_A_PIN);
    uint8_t LSB = digitalRead(ENC_B_PIN);

    uint8_t encoded = (MSB << 1) | LSB;
    uint8_t sum = (last_encoded << 2) | encoded;

    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
        encoder_value++;
    }
    if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
        encoder_value--;
    }

    last_encoded = encoded;
}

void PhysicalHandler::init() {
    // ENC_AB
    pinMode(ENC_A_PIN, INPUT_PULLUP);
    pinMode(ENC_B_PIN, INPUT_PULLUP);

    uint8_t MSB = digitalRead(ENC_A_PIN);
    uint8_t LSB = digitalRead(ENC_B_PIN);
    last_encoded = (MSB << 1) | LSB;

    attachInterrupt(digitalPinToInterrupt(ENC_A_PIN), updateEncoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_B_PIN), updateEncoderISR, CHANGE);

    // BTN
    for (auto &btn : buttons) {
        pinMode(btn.pin, btn.pin == SW_ENC_PIN ? INPUT_PULLDOWN : INPUT_PULLUP);
    }
}

void PhysicalHandler::process() {
    if(encoder_value != 0) {
        noInterrupts();
        int32_t delta = encoder_value;
        encoder_value = 0;
        interrupts();

        if(delta > 0) {
            // 時計回り
        } else if(delta < 0) {
            // 反時計回り
        }
    }

    uint32_t now = millis();

    for(auto &btn: buttons) {
        bool is_active = false;
        if(btn.pin == SW_ENC_PIN) {
            is_active = (digitalRead(btn.pin) == HIGH);
        } else {
            is_active = (digitalRead(btn.pin) == LOW);
        }

        if(is_active) {
            // ボタンを押している
            if(!btn.is_pressed) {
                // 押下開始
                btn.is_pressed = true;
                btn.press_start_time = now;
                btn.long_triggered = false;
            } else {
                // 押下中
                if(!btn.long_triggered && (now - btn.press_start_time > TIME_LONG_PRESS)) {
                    state_.setBtnState(btn.id_long);
                    btn.long_triggered = true;
                }
            }
        }

        else {
            // ボタンが離されている
            if(btn.is_pressed) {
                // チャタリング防止
                if(now - btn.press_start_time > TIME_DEBOUNCE) {
                    if(!btn.long_triggered) {
                        state_.setBtnState(btn.id_short);
                    }
                }
                btn.is_pressed = false;
            }
        }
    }
}