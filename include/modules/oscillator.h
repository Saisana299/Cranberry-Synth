#pragma once

#include "handlers/audio.h"
#include "utils/math.h"
#include "utils/wavetable.h"

class Oscillator {
private:
    int16_t* wavetable;
    size_t wavetable_size;
    void init();

public:
    struct Memory {
        float phase;
        float delta;
        float vel_vol;
    };

    Oscillator() {
        init();
    }

    void setFrequency(Memory& mem, uint8_t note);
    void setFrequency(Memory& mem, float freq);
    void setVolume(Memory& mem, uint8_t velocity);
    void setPhase(Memory& mem);
    int16_t getSample(Memory& mem);
    void update(Memory& mem);
    void reset(Memory& mem);
};