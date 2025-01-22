#ifndef SYNTH_H
#define SYNTH_H

#define MAX_NOTES 8
#define NOTE_BUFFER_SIZE 16

#include "handlers/audio.h"
#include "handlers/midi.h"
#include "utils/debug.h"
#include "utils/envelope.h"
#include "utils/wavetable.h"
#include "utils/state.h"

struct ActiveSynthNote {
    uint8_t order;
    uint8_t note;
    uint8_t velocity;
    uint8_t channel;
    float phase;
    float delta;
    Envelope amp_env;
};

class Synth {
private:
    static inline Synth* instance = nullptr;
    ActiveSynthNote active_notes[MAX_NOTES];
    ADSRConfig amp_adsr;
    uint8_t order_max = 0;
    uint8_t last_index = 0;
    void init();
    void generate();
    void updateOrder(uint8_t removed);
    void resetNote(uint8_t index);

public:
    Synth() {
        init();
    }
    static inline Synth* getInstance() {
        return instance;
    };
    void update();
    void noteOn(uint8_t note, uint8_t velocity, uint8_t channel);
    void noteOff(uint8_t note, uint8_t channel);
};

#endif