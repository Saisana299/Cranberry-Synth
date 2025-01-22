#pragma once

#include "handlers/audio.h"
#include "utils/math.h"
#include "utils/wavetable.h"

class Oscillator {
private:
    float phase;
    float delta;
    float frequency;

    void init();

public:
    Oscillator() {
        init();
    }

    void setFrequency(uint8_t note);
    void setFrequency(float freq);
    void resetPhase();
    int16_t getSample();
    void update();
};