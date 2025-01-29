#pragma once

#include "handlers/audio.h"
#include "utils/math.h"

class Envelope {
public:

    enum class State {
        Attack, Decay, Sustain, Release
    };

    struct Memory {
        State state;
        uint32_t elapsed;
        float current_level;
        float prev_level;
    };

    void reset(Memory& mem);
    void release(Memory& mem);
    void update(Memory& mem, uint32_t dt = 1);
    float currentLevel(Memory& mem);
    bool isFinished(Memory& mem);

private:
    // uint32_t attack_samples = 0.005f * SAMPLE_RATE;
    // uint32_t decay_samples = 0.01f * SAMPLE_RATE;
    // uint32_t release_samples = 0.01f * SAMPLE_RATE;
    // float sustain_level = 1.0f;
    uint32_t attack_samples = 0.005f * SAMPLE_RATE;
    uint32_t decay_samples = 1.5f * SAMPLE_RATE;
    uint32_t release_samples = 0.8f * SAMPLE_RATE;
    float sustain_level = 0.0f;
};