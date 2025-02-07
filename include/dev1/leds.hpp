#pragma once

#include <Arduino.h>

#include "utils/state.hpp"

#define RED_PORTSET CORE_PIN7_PORTSET
#define RED_PORTCLR CORE_PIN7_PORTCLEAR
#define GREEN_PORTSET CORE_PIN8_PORTSET
#define GREEN_PORTCLR CORE_PIN8_PORTCLEAR
#define BLUE_PORTSET CORE_PIN9_PORTSET
#define BLUE_PORTCLR CORE_PIN9_PORTCLEAR

#define RED_BITMASK CORE_PIN7_BITMASK
#define GREEN_BITMASK CORE_PIN8_BITMASK
#define BLUE_BITMASK CORE_PIN9_BITMASK

constexpr uint8_t RED_PIN = 7;
constexpr uint8_t GREEN_PIN = 8;
constexpr uint8_t BLUE_PIN = 9;

class Leds {
private:
    bool before_led_midi = false;
    bool before_led_audio = false;

    void init();

public:
    Leds() {
        init();
    }
    void process();
};