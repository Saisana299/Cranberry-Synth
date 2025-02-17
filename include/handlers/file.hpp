#pragma once

#include <MD_MIDIFile.h>

#include "modules/synth.hpp"

class FileHandler {
private:
    MD_MIDIFile SMF = {};

    static inline void midiCallbackStatic(midi_event *pev);

    static inline FileHandler* instance = nullptr;

    void init();
    void midiCallback(midi_event *pev);

public:
    FileHandler() {
        instance = this;
        init();
    }
    void process();
    static void play();
    static void stop();
};