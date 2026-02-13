#pragma once

#include <Arduino.h>

#include "utils/state.hpp"

constexpr uint8_t MIDI_LED_PIN = 6;
constexpr uint8_t AUDIO_LED_PIN = 9;
constexpr uint8_t PWR_LED_PIN = 30;
constexpr uint8_t ST_LED_PIN = 31;

struct LedConfig {
    uint8_t pin;
    volatile uint32_t* port_set;
    volatile uint32_t* port_clr;
    uint32_t bitmask;
};

class Leds {
private:
    State& state_;

    uint32_t audio_led_off_timer = 0;

    struct {
        bool midi = false;
        bool audio = false;
    } led_state;

    static constexpr LedConfig LED_CONFIGS[4] = {
        {MIDI_LED_PIN, &CORE_PIN6_PORTSET, &CORE_PIN6_PORTCLEAR, static_cast<uint32_t>(CORE_PIN6_BITMASK)},
        {AUDIO_LED_PIN, &CORE_PIN9_PORTSET, &CORE_PIN9_PORTCLEAR, static_cast<uint32_t>(CORE_PIN9_BITMASK)},
        {PWR_LED_PIN, &CORE_PIN30_PORTSET, &CORE_PIN30_PORTCLEAR, static_cast<uint32_t>(CORE_PIN30_BITMASK)},
        {ST_LED_PIN, &CORE_PIN31_PORTSET, &CORE_PIN31_PORTCLEAR, static_cast<uint32_t>(CORE_PIN31_BITMASK)},
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