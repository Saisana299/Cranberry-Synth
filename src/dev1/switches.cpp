#include "dev1/switches.hpp"

volatile bool Switches::buttonStateFlag = false;
volatile long Switches::encoderValue = 0;
volatile uint8_t Switches::lastEncoded = 0;

void Switches::buttonISR() {
    buttonStateFlag = (digitalRead(BUTTON_PIN) == LOW);
}

void Switches::updateEncoder() {
    uint8_t MSB = digitalRead(A_PIN);
    uint8_t LSB = digitalRead(B_PIN);
    uint8_t encoded = (MSB << 1) | LSB;
    uint8_t sum = (lastEncoded << 2) | encoded;

    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
        encoderValue++;
    }
    if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
        encoderValue--;
    }

    lastEncoded = encoded;
}

void Switches::init() {
    pinMode(BUTTON_PIN, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), Switches::buttonISR, CHANGE);

    pinMode(A_PIN, INPUT_PULLUP);
    pinMode(B_PIN, INPUT_PULLUP);

    lastEncoded = (digitalRead(A_PIN) << 1) | digitalRead(B_PIN);

    attachInterrupt(digitalPinToInterrupt(A_PIN), Switches::updateEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(B_PIN), Switches::updateEncoder, CHANGE);
}

void Switches::process() {
    static long lastReportedValue = 0;
    long value;

    noInterrupts();
    value = encoderValue;
    interrupts();

    if (value != lastReportedValue) {
        lastReportedValue = value;
    }

    if(buttonStateFlag) {
        FileHandler::stop();
    }
}