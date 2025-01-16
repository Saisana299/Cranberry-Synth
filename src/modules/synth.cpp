#include "modules/synth.h"

bool on = false;
float phase = 0;
float delta = 440.0 / SAMPLE_RATE;

void Synth::generate() {
    if(!on || samples_ready) return;
    for(size_t i = 0; i < BUFFER_SIZE; i++) {
        samples_L[i] = triangle(phase);
        samples_R[i] = triangle(phase);
        phase += delta;
        if (phase >= 1.0) phase -= 1.0;
    }
    samples_ready = true;
}

void Synth::update() {
    generate();
}

int16_t Synth::triangle(float phase) {
    return static_cast<int16_t>((1.0 - fabs(2.0 * phase - 1.0)) * 32767);
}