#pragma once

#include "handlers/audio.h"
#include "utils/math.h"
#include "utils/wavetable.h"

class Oscillator {
private:
    //wavetable
    void init();

public:
    struct Memory {
        float phase;
        float delta;
    };

    Oscillator() {
        init();
    }

    void setFrequency(Memory& mem, uint8_t note);
    void setFrequency(Memory& mem, float freq);
    void resetPhase(Memory& mem);
    int16_t getSample(Memory& mem);
    void update(Memory& mem);
};