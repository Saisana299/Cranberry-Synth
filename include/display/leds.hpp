#pragma once

#include <Arduino.h>

#include "utils/state.hpp"

constexpr uint8_t MIDI_LED_PIN = 34;
constexpr uint8_t AUDIO_LED_PIN = 29;
// constexpr uint8_t LED3_PIN = 28;
// constexpr uint8_t LED4_PIN = 35;

struct LedConfig {
    uint8_t pin;
    volatile uint32_t* port_set;
    volatile uint32_t* port_clr;
    uint32_t bitmask;
};

class Leds {
private:
    State& state_;

    struct {
        bool midi = false;
        bool audio = false;
    } led_state;

    static constexpr LedConfig LED_CONFIGS[2] = {
        {MIDI_LED_PIN, &CORE_PIN34_PORTSET, &CORE_PIN34_PORTCLEAR, static_cast<uint32_t>(CORE_PIN34_BITMASK)},
        {AUDIO_LED_PIN, &CORE_PIN29_PORTSET, &CORE_PIN29_PORTCLEAR, static_cast<uint32_t>(CORE_PIN29_BITMASK)},
    };

    static inline void setLed(const LedConfig& config, bool on) {
        if(on) {
            *(config.port_set) = config.bitmask;
        } else {
            *(config.port_clr) = config.bitmask;
        }
    }

public:
    Leds(State& state) : state_(state) {}

    void init();
    void process();
};