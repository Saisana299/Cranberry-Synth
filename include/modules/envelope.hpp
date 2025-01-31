#pragma once

#include "handlers/audio.hpp"
#include "utils/math.hpp"

class Envelope {
private:
    uint32_t attack_samples = static_cast<uint32_t>(0.005f * SAMPLE_RATE);
    uint32_t decay_samples = static_cast<uint32_t>(0.01f * SAMPLE_RATE);
    uint32_t release_samples = static_cast<uint32_t>(0.01f * SAMPLE_RATE);
    float sustain_level = 1.0f;

public:

    enum class State {
        Attack, Decay, Sustain, Release
    };

    struct Memory {
        State state = State::Attack;
        uint32_t elapsed = 0;
        float current_level = 0.0f;
        float prev_level = 0.0f;
    };

    void reset(Memory& mem);
    void release(Memory& mem);
    void update(Memory& mem, uint32_t dt = 1);
    float currentLevel(Memory& mem);
    bool isFinished(Memory& mem);

    void setAttack(float attack_ms);
    void setDecay(float decay_ms);
    void setRelease(float release_ms);
    void setSustain(float sustain_level);
};