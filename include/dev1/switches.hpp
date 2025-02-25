#pragma once

#include <Arduino.h>

#include "handlers/file.hpp"

constexpr uint8_t BUTTON_PIN = 16;
constexpr uint8_t A_PIN = 15;
constexpr uint8_t B_PIN = 17;

class Switches {
private:
    static volatile bool buttonStateFlag;
    static volatile long encoderValue;
    static volatile uint8_t lastEncoded;

    void init();

    static void buttonISR();
    static void updateEncoder();

    int8_t playing = 0;

public:
    Switches() {
        init();
    }
    void process();
};