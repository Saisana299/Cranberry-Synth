#include "display/leds.hpp"

void Leds::init() {
    pinMode(MIDI_LED_PIN, OUTPUT);
    pinMode(AUDIO_LED_PIN, OUTPUT);
    pinMode(LED3_PIN, OUTPUT);
    pinMode(LED4_PIN, OUTPUT);
}

void Leds::process() {
    auto& led_midi = State::led_midi;
    auto& led_audio = State::led_audio;

    // MIDIインジケーターの状態切替
    if(led_midi != before_led_midi) {
        before_led_midi = led_midi;
        if(led_midi) MIDI_LED_PORTSET = MIDI_LED_BITMASK;
        else MIDI_LED_PORTCLR = MIDI_LED_BITMASK;
    }

    // 音声出力インジケーターの状態切替
    if(led_audio != before_led_audio) {
        before_led_audio = led_audio;
        if(led_audio) AUDIO_LED_PORTSET = AUDIO_LED_BITMASK;
        else AUDIO_LED_PORTCLR = AUDIO_LED_BITMASK;
    }
}