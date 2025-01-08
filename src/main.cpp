#include <Arduino.h>

#include <handlers/audio.h>
#include <handlers/midi.h>
#include <display/gfx.h>

AudioHandler audio_hdl;
MIDIHandler  midi_hdl;

GFX_SSD1351 display;

void setup() {
    audio_hdl.init();
    midi_hdl.init();

    display.init();
}

void loop() {
    audio_hdl.process();
    midi_hdl.process();
}