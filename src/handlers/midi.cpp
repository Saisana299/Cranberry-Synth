#include <handlers/midi.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial6, MIDI);

void MIDIHandler::init(AudioHandler &audio) {
    audio_hdl = audio;
    MIDI.begin(MIDI_CHANNEL_OMNI);
    Serial1.begin(115200);
    Serial1.println("MIDI Handler initialized\n");
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
                    Serial1.println(String("Note On:  ch=") + channel + ", note=" + note + ", velocity=" + velocity);
                } else {
                    Serial1.println(String("Note Off: ch=") + channel + ", note=" + note);
                }
                break;
            case midi::NoteOff:
                note = MIDI.getData1();
                velocity = MIDI.getData2();
                channel = MIDI.getChannel();
                Serial1.println(String("Note Off: ch=") + channel + ", note=" + note + ", velocity=" + velocity);
                break;
            default:
                d1 = MIDI.getData1();
                d2 = MIDI.getData2();
                Serial1.println(String("Message, type=") + type + ", data = " + d1 + " " + d2);
        }
    }
}