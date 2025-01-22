#ifndef STATE_H
#define STATE_H

#define MODE_SYNTH 0

class State {
public:
    static inline bool led_state = false;
    static inline bool led_state_prev = false;
    static inline uint8_t mode_state = MODE_SYNTH;
    static inline float amp_level = 1.0f;
};

#endif