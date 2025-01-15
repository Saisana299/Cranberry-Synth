#include "handlers/midi.h"

#include "utils/debug.h"

Queue<NoteQueue> note_on_queue = Queue<NoteQueue>(NOTE_QUEUE_SIZE);
Queue<NoteQueue> note_off_queue = Queue<NoteQueue>(NOTE_QUEUE_SIZE);

void MIDIHandler::pushNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
    if (is_enable_queue) {
        if (note_on_queue.count() == NOTE_QUEUE_SIZE) {
            note_on_queue.pop();
        }
        note_on_queue.push(NoteQueue{note, velocity, channel});
    }
}

void MIDIHandler::pushNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
    if (is_enable_queue) {
        if (note_off_queue.count() == NOTE_QUEUE_SIZE) {
            note_off_queue.pop();
        }
        note_off_queue.push(NoteQueue{note, velocity, channel});
    }
}

void MIDIHandler::queueMode(bool enable) {
    if(!enable) {
        note_off_queue.clear();
        note_on_queue.clear();
    }
    is_enable_queue = enable;
}

void MIDIHandler::init() {
    is_enable_queue = false;
    MIDI.begin(MIDI_CHANNEL_OMNI);
}

void MIDIHandler::process() {
    uint8_t note, velocity, channel, d1, d2;
    if(MIDI.read()) {
        byte type = MIDI.getType();
        switch(type) {
            case midi::NoteOn:{
                note = MIDI.getData1();
                velocity = MIDI.getData2();
                channel = MIDI.getChannel();
                if (velocity > 0 && note <= 127) {
                    pushNoteOn(note, velocity, channel);
                    Debug::println(String("Note On:  ch=") + channel + ", note=" + note + ", velocity=" + velocity);
                } else if(note <= 127) {
                    pushNoteOff(note, velocity, channel);
                    Debug::println(String("Note Off: ch=") + channel + ", note=" + note);
                }
                break;
            }

            case midi::NoteOff:{
                note = MIDI.getData1();
                velocity = MIDI.getData2();
                channel = MIDI.getChannel();
                if (note <= 127) {
                    pushNoteOff(note, velocity, channel);
                    Debug::println(String("Note Off: ch=") + channel + ", note=" + note);
                }
                break;
            }

            case midi::PitchBend:{
                break;
            }

            case midi::ControlChange:{
                d1 = MIDI.getData1();
                d2 = MIDI.getData2();
                Debug::println(String("Control Change:") + d1 + " " + d2);
                break;
            }

            default:{
                break;
            }
        }
    }
}