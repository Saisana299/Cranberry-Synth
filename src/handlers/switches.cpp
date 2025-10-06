#include "handlers/switches.hpp"

volatile uint8_t Switches::lastEncoded = 0;

void Switches::updateEncoder() {
    uint8_t MSB = digitalRead(A_PIN);
    uint8_t LSB = digitalRead(B_PIN);
    uint8_t encoded = (MSB << 1) | LSB;
    uint8_t sum = (lastEncoded << 2) | encoded;

    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
        //buttonState = ECR_CHANGE;
    }
    if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
        //buttonState = ECL_CHANGE;
    }

    lastEncoded = encoded;
}

void Switches::init() {
    // ENC_AB
    pinMode(A_PIN, INPUT_PULLUP);
    pinMode(B_PIN, INPUT_PULLUP);

    lastEncoded = (digitalRead(A_PIN) << 1) | digitalRead(B_PIN);

    attachInterrupt(digitalPinToInterrupt(A_PIN), Switches::updateEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(B_PIN), Switches::updateEncoder, CHANGE);

    // BTN
    for (auto &button : buttons) {
        pinMode(button.pin, button.pin == ECB_PIN ? INPUT_PULLDOWN : INPUT_PULLUP);
    }
}

void Switches::process() {
    for(auto &button : buttons) {
        // ボタンが押されている時
        if(digitalRead(button.pin) == (button.pin == ECB_PIN ? HIGH : LOW)) {
            if(button.pushCount <= PUSH_SHORT) button.pushCount++;
            else if(button.pushCount == PUSH_SHORT+1) {
                state_.setBtnState(button.state);
                button.pushCount++;
            }
        }
        // ボタンを離している時
        else {
            if(button.pushCount >= PUSH_SHORT && intervalCount >= PUSH_LONG) {
                intervalCount = 0;
                state_.setBtnState(button.stateLong);
            }
            button.pushCount = 0;
        }
    }
    if(intervalCount <= PUSH_LONG) intervalCount++;
}