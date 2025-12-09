#include "display/leds.hpp"

void Leds::init() {
    pinMode(MIDI_LED_PIN, OUTPUT);
    pinMode(AUDIO_LED_PIN, OUTPUT);
    // pinMode(LED3_PIN, OUTPUT);
    // pinMode(LED4_PIN, OUTPUT);
}

void Leds::process() {
    auto led_midi = state_.getLedMidi();
    auto led_audio = state_.getLedAudio();

    // MIDIインジケーターの状態切替
    if(led_midi != led_state.midi) {
        led_state.midi = led_midi;
        setLed(LED_CONFIGS[0], led_midi);
    }

    // 音声出力インジケーターの状態切替
    if(led_audio != led_state.audio) {
        led_state.audio = led_audio;
        setLed(LED_CONFIGS[1], led_audio);
    }
}