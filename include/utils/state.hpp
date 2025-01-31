#pragma once

constexpr uint8_t MODE_SYNTH = 0;

class State {
public:
    static inline bool led_state = false;
    static inline bool led_state_prev = false;
    static inline uint8_t mode_state = MODE_SYNTH;
};