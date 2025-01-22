#pragma once

#include "utils/math.h"

struct ADSRConfig {
    uint32_t attack_samples;
    uint32_t decay_samples;
    float    sustain_level;
    uint32_t release_samples;
};

class Envelope {
public:
    enum class EnvState {
        Attack, Decay, Sustain, Release
    };

    void reset();
    void release();
    void update(const ADSRConfig& adsr, uint32_t dt);
    float currentLevel();
    bool isFinished();

private:
    EnvState state = EnvState::Attack;
    uint32_t elapsed = 0;
    float current_level = 0;
    float prev_level = 0;
};