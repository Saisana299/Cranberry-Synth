#include <Arduino.h>

#include <audio_handler.h>
#include <midi_handler.h>

AudioHandler audio;
MIDIHandler midih;

void setup() {
    audio.init();
    midih.init();
}

void loop() {
    audio.process();
    midih.process();
}