#include "modules/synth.h"

float phase = 0;
float delta = 440.0 / SAMPLE_RATE;

void Synth::init() {
    instance = this;
}

Synth* Synth::getInstance() {
    return instance;
}

void Synth::generate() {
    if(samples_ready) return;
    auto& wavetable = Wavetable::square;
    const size_t WAVETABLE_SIZE = sizeof(wavetable) / sizeof(wavetable[0]);
    for(size_t i = 0; i < BUFFER_SIZE; i++) {
        size_t index = static_cast<size_t>(phase * WAVETABLE_SIZE) % WAVETABLE_SIZE;
        samples_L[i] = wavetable[index];
        samples_R[i] = wavetable[index];
        phase += delta;
        if (phase >= 1.0) phase -= 1.0;
    }
    samples_ready = true;
}

void Synth::update() {
    generate();
}

void Synth::addNote(uint8_t note, uint8_t velocity, uint8_t channel) {
}

void Synth::removeNote(uint8_t note, uint8_t channel) {
}