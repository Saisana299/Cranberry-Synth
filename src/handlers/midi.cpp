#include "handlers/midi.h"

MIDI_CREATE_INSTANCE(HardwareSerial, Serial6, MIDI);

void MIDIHandler::init() {
    MIDI.begin(MIDI_CHANNEL_OMNI);
}

void MIDIHandler::process() {
    int note, velocity, channel, d1, d2;
    if(MIDI.read()) {
        byte type = MIDI.getType();
        switch(type) {
            case midi::NoteOn:{
                note = MIDI.getData1();
                velocity = MIDI.getData2();
                channel = MIDI.getChannel();
                if (velocity > 0) {
                    // NOTE ON
                } else {
                    // NOTE OFF
                }
                break;
            }

            case midi::NoteOff:{
                note = MIDI.getData1();
                velocity = MIDI.getData2();
                channel = MIDI.getChannel();
                // NOTE OFF
                break;
            }

            case midi::PitchBend:{
                break;
            }

            case midi::ControlChange:{
                d1 = MIDI.getData1();
                d2 = MIDI.getData2();
                break;
            }

            default:{
                break;
            }
        }
    }
}