#pragma once

#include <Arduino.h>

constexpr uint8_t BUTTON_PIN = 30;
constexpr uint8_t A_PIN = 31;
constexpr uint8_t B_PIN = 32;

class Switches {
private:
    static volatile bool buttonStateFlag;
    static volatile long encoderValue;
    static volatile uint8_t lastEncoded;

    void init();

    static void buttonISR();
    static void updateEncoder();

public:
    Switches() {
        init();
    }
    void process();
};