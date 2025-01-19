#ifndef SYNTH_H
#define SYNTH_H

#define MAX_NOTES 8

#include "handlers/audio.h"
#include "handlers/midi.h"
#include "utils/debug.h"
#include "utils/wavetable.h"

struct ActiveSynthNote {
    uint8_t order;
    uint8_t note;
    uint8_t velocity;
    uint8_t channel;
    float phase;
    float delta;
};

class Synth {
private:
    static inline Synth* instance = nullptr;
    ActiveSynthNote active_notes[MAX_NOTES];
    void init();
    void generate();
public:
    Synth() {
        init();
    }
    static inline Synth* getInstance() {
        return instance;
    };
    void update();
    void addNote(uint8_t note, uint8_t velocity, uint8_t channel);
    void removeNote(uint8_t note, uint8_t channel);
};

#endif