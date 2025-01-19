#include "modules/synth.h"

void Synth::init() {
    instance = this;
}

void Synth::generate() {
    if(samples_ready) return;
    auto& wavetable = Wavetable::square;
    const size_t WAVETABLE_SIZE = sizeof(wavetable) / sizeof(wavetable[0]);
    auto& phase = active_notes[0].phase;
    auto& delta = active_notes[0].delta;
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
    if(active_notes[0].order == 0) return;
    generate();
}

void Synth::addNote(uint8_t note, uint8_t velocity, uint8_t channel) {
    active_notes[0].order = 1;
    active_notes[0].note = note;
    active_notes[0].velocity = velocity;
    active_notes[0].channel = channel;
    active_notes[0].phase = 0.0;
    active_notes[0].delta = AudioMath::noteToFrequency(note) / SAMPLE_RATE;
}

void Synth::removeNote(uint8_t note, uint8_t channel) {
    active_notes[0].order = 0;
    active_notes[0].note = 0;
    active_notes[0].velocity = 0;
    active_notes[0].channel = 0;
    active_notes[0].phase = 0.0;
    active_notes[0].delta = 0.0;
}