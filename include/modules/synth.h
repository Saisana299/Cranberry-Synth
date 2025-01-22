#pragma once

#define MAX_NOTES 24

#include "handlers/audio.h"
#include "modules/envelope.h"
#include "modules/oscillator.h"
#include "utils/state.h"
#include "utils/debug.h"

struct ActiveSynthNote {
    uint8_t order;
    uint8_t note;
    uint8_t velocity;
    uint8_t channel;
    Oscillator osc;
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