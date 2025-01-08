#include <midi_handler.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

void MIDIHandler::init() {
    MIDI.begin(MIDI_CHANNEL_OMNI);
    Serial.begin(57600);
}

void MIDIHandler::process() {
    int note, velocity, channel, d1, d2;
    if(MIDI.read()) {
        byte type = MIDI.getType();
        switch(type) {
            case midi::NoteOn:
                note = MIDI.getData1();
                velocity = MIDI.getData2();
                channel = MIDI.getChannel();
                if (velocity > 0) {
                    Serial.println(String("Note On:  ch=") + channel + ", note=" + note + ", velocity=" + velocity);
                } else {
                    Serial.println(String("Note Off: ch=") + channel + ", note=" + note);
                }
                break;
            case midi::NoteOff:
                note = MIDI.getData1();
                velocity = MIDI.getData2();
                channel = MIDI.getChannel();
                Serial.println(String("Note Off: ch=") + channel + ", note=" + note + ", velocity=" + velocity);
                break;
            default:
                d1 = MIDI.getData1();
                d2 = MIDI.getData2();
                Serial.println(String("Message, type=") + type + ", data = " + d1 + " " + d2);
        }
    }
}