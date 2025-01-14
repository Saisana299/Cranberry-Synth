#ifndef MIDI_HANDLER_H
#define MIDI_HANDLER_H

#include <Audio.h>
#include <MIDI.h>

extern MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial>> MIDI;

class MIDIHandler {
private:
    void init();
public:
    MIDIHandler() {
        init();
    }
    void process();
};

#endif