#ifndef SYNTH_H
#define SYNTH_H

#define MAX_NOTES 8

#include "handlers/audio.h"
#include "handlers/midi.h"
#include "utils/debug.h"
#include "utils/wavetable.h"

extern bool on;
extern float phas;
extern float delta;

struct SynthNote {
    uint8_t order;
    uint8_t note;
    uint8_t velocity;
    uint8_t channel;
    float phase;
    float delta;
};

class Synth {
private:
    SynthNote notes[MAX_NOTES];
    void generate();
    int16_t triangle(float phase);
public:
    void update();
};

#endif