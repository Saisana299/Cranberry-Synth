#pragma once

#include <Arduino.h>

#include "utils/state.hpp"

constexpr uint8_t MIDI_LED_PIN = 34;
constexpr uint8_t AUDIO_LED_PIN = 29;
constexpr uint8_t LED3_PIN = 28;
constexpr uint8_t LED4_PIN = 35;

#define MIDI_LED_PORTSET CORE_PIN34_PORTSET
#define MIDI_LED_PORTCLR CORE_PIN34_PORTCLEAR
#define MIDI_LED_BITMASK CORE_PIN34_BITMASK

#define AUDIO_LED_PORTSET CORE_PIN29_PORTSET
#define AUDIO_LED_PORTCLR CORE_PIN29_PORTCLEAR
#define AUDIO_LED_BITMASK CORE_PIN29_BITMASK

class Leds {
private:
    State& state_;
    bool before_led_midi = false;
    bool before_led_audio = false;

public:
    Leds(State& state) : state_(state) {}
    void init();
    void process();
};