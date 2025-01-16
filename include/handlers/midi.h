#ifndef MIDI_HANDLER_H
#define MIDI_HANDLER_H

#include <Audio.h>
#include <MIDI.h>

#include "modules/synth.h"
#include "utils/debug.h"

struct NoteQueue {
    uint8_t note;
    uint8_t velocity;
    uint8_t channel;
};
#define NOTE_QUEUE_SIZE 16

class MIDIHandler {
private:
    MIDI_NAMESPACE::SerialMIDI<HardwareSerial> serialMIDI;
    MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial>> MIDI;
    bool is_enable_queue;
    void init();
    void pushNoteOn(uint8_t note, uint8_t velocity, uint8_t channel);
    void pushNoteOff(uint8_t note, uint8_t velocity, uint8_t channel);
public:
    MIDIHandler(): serialMIDI(Serial1), MIDI(serialMIDI) {
        init();
    }
    void process();
    void queueMode(bool enable);
    void queueReset(bool enable = false);
    bool queueStatus();
};

#endif