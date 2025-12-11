#pragma once

#include <MD_MIDIFile.h>

#include "modules/synth.hpp"
#include "utils/state.hpp"

class MIDIPlayer {
private:
    MD_MIDIFile SMF = {};

    static inline void midiCallbackStatic(midi_event *pev);

    static inline MIDIPlayer* instance = nullptr;

    bool is_initialized = false;

    State& state_;
    Synth& synth_;

    void init();
    void midiCallback(midi_event *pev);

public:
    MIDIPlayer(State& state) : state_(state), synth_(Synth::getInstance()) {
        if (instance == nullptr) {
            instance = this;
        }
        init();
    }
    ~MIDIPlayer() {
        if (instance == this) instance = nullptr;
    }

    void process();
    static void play(const char* path);
    static void stop();
};