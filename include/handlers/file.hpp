#pragma once

#include <MD_MIDIFile.h>

#include "modules/synth.hpp"
#include "utils/state.hpp"

class FileHandler {
private:
    MD_MIDIFile SMF = {};

    static inline void midiCallbackStatic(midi_event *pev);

    static inline FileHandler* instance = nullptr;

    State& state_;

    void init();
    void midiCallback(midi_event *pev);

public:
    FileHandler(State& state) : state_(state) {
        instance = this;
        init();
    }
    void process();
    static void play(const char* path);
    static void stop();
};