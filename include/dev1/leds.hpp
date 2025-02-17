#pragma once

#include <Arduino.h>

#include "utils/state.hpp"

#define MIDI_LED_PORTSET CORE_PIN34_PORTSET
#define MIDI_LED_PORTCLR CORE_PIN34_PORTCLEAR
#define AUDIO_LED_PORTSET CORE_PIN35_PORTSET
#define AUDIO_LED_PORTCLR CORE_PIN35_PORTCLEAR

#define MIDI_LED_BITMASK CORE_PIN34_BITMASK
#define AUDIO_LED_BITMASK CORE_PIN35_BITMASK

constexpr uint8_t MIDI_LED_PIN = 34;
constexpr uint8_t AUDIO_LED_PIN = 35;

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