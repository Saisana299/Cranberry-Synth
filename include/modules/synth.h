#pragma once

// #define MAX_NOTES 8
#define MAX_NOTES 32

#include "handlers/audio.h"
#include "modules/envelope.h"
#include "modules/oscillator.h"
#include "utils/state.h"
#include "utils/debug.h"

class Synth {
private:
    struct SynthNote {
        uint8_t order;
        uint8_t note;
        uint8_t velocity;
        uint8_t channel;
        Oscillator::Memory osc_mem;
        Envelope::Memory env_mem;
    };
    struct Operator {
        Oscillator osc;
        Envelope env;
    };
    static inline Synth* instance = nullptr;
    SynthNote notes[MAX_NOTES];
    Operator operators[6];
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