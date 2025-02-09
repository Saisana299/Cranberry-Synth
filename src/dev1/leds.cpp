#include "dev1/leds.hpp"

void Leds::init() {
    pinMode(MIDI_LED_PIN, OUTPUT);
    pinMode(AUDIO_LED_PIN, OUTPUT);
}

void Leds::process() {
    // 使用したロータリーエンコーダーのLEDは共通アノード型
    auto& led_midi = State::led_midi;
    auto& led_audio = State::led_audio;

    if(led_midi != before_led_midi) {
        before_led_midi = led_midi;
        if(led_midi) MIDI_LED_PORTSET = MIDI_LED_BITMASK;
        else MIDI_LED_PORTCLR = MIDI_LED_BITMASK;
    }

    if(led_audio != before_led_audio) {
        before_led_audio = led_audio;
        if(led_audio) AUDIO_LED_PORTSET = AUDIO_LED_BITMASK;
        else AUDIO_LED_PORTCLR = AUDIO_LED_BITMASK;
    }
}