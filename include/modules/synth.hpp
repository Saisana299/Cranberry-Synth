#pragma once

#include "handlers/audio.hpp"
#include "modules/envelope.hpp"
#include "modules/oscillator.hpp"
#include "modules/delay.hpp"
#include "modules/filter.hpp"
#include "utils/state.hpp"
#include "utils/debug.hpp"
#include "utils/math.hpp"

// #define MAX_NOTES 8
constexpr uint8_t MAX_NOTES = 32;
constexpr uint8_t MAX_OPERATORS = 4;

class Synth {
private:
    struct SynthNote {
        uint8_t order = 0;
        uint8_t note = 255;
        uint8_t velocity = 0;
        uint8_t channel = 0;
    };
    SynthNote notes[MAX_NOTES] = {};

    struct OperatorState {
        Oscillator::Memory osc_mems[MAX_NOTES];
        Envelope::Memory env_mems[MAX_NOTES];
    };
    OperatorState ope_states[MAX_OPERATORS] = {};

    enum class OpMode {
        Carrier, Modulator
    };

    struct Operator { //todo Operator ON/OFF機能
        OpMode mode = OpMode::Modulator;
        Oscillator osc = Oscillator{};
        Envelope env = Envelope{};
    };
    Operator operators[MAX_OPERATORS] = {};

    Delay delay = Delay{};
    uint32_t delay_remain = 0;
    bool delay_enabled = false;

    Filter filter = Filter{};
    bool lpf_enabled = false;
    bool hpf_enabled = false;

    static inline Synth* instance = nullptr;
    uint8_t order_max = 0;
    uint8_t last_index = 0;
    float amp_level = 1.0f;
    float adjust_level = 1.0f / MAX_NOTES;

    void init();
    void generate();
    void updateOrder(uint8_t removed);
    void resetNote(uint8_t index);

public:
    Synth() {
        instance = this;
        init();
    }
    static inline Synth* getInstance() {
        return instance;
    };
    void update();
    void noteOn(uint8_t note, uint8_t velocity, uint8_t channel);
    void noteOff(uint8_t note, uint8_t channel);
};