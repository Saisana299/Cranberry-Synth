#pragma once

// #define MAX_NOTES 8
#define MAX_NOTES 32
#define MAX_OPERATORS 4

#include "handlers/audio.h"
#include "modules/envelope.h"
#include "modules/oscillator.h"
#include "utils/state.h"
#include "utils/debug.h"
#include "utils/math.h"

class Synth {
private:
    struct SynthNote {
        uint8_t order;
        uint8_t note;
        uint8_t velocity;
        uint8_t channel;
    };
    SynthNote notes[MAX_NOTES];

    struct OperatorState {
        Oscillator::Memory osc_mems[MAX_NOTES];
        Envelope::Memory env_mems[MAX_NOTES];
    };
    OperatorState ope_states[MAX_OPERATORS];

    enum class OpMode {
        Carrier, Modulator
    };

    struct Operator { //todo Operator ON/OFF機能
        OpMode mode = OpMode::Modulator;
        Oscillator osc = Oscillator();
        Envelope env = Envelope();
    };
    Operator operators[MAX_OPERATORS];

    static inline Synth* instance = nullptr;
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