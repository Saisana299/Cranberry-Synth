#pragma once

#include <Audio.h>
#include <MIDI.h>

#include "modules/synth.hpp"
#include "utils/state.hpp"

class MIDIHandler {
private:
    MIDI_NAMESPACE::SerialMIDI<HardwareSerial> serialMIDI = Serial1;
    MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial>> MIDI = serialMIDI;

    static inline void handleNoteOnStatic(uint8_t ch, uint8_t note, uint8_t velocity);
    static inline void handleNoteOffStatic(uint8_t ch, uint8_t note, uint8_t velocity);

    static inline MIDIHandler* instance = nullptr;

    void init();
    void handleNoteOn(uint8_t ch, uint8_t note, uint8_t velocity);
    void handleNoteOff(uint8_t ch, uint8_t note, uint8_t velocity);

public:
    MIDIHandler() {
        instance = this;
        init();
    }
    void process();
};