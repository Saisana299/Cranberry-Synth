#include "display/leds.hpp"

void Leds::init() {
    pinMode(MIDI_LED_PIN, OUTPUT);
    pinMode(AUDIO_LED_PIN, OUTPUT);
    // pinMode(LED3_PIN, OUTPUT);
    // pinMode(LED4_PIN, OUTPUT);
}

void Leds::process() {
    // MIDIインジケーターの状態切替
    auto led_midi = state_.getLedMidi();
    if(led_midi != led_state.midi) {
        led_state.midi = led_midi;
        setLed(LED_CONFIGS[0], led_midi);
    }

    // 音声出力インジケーターの状態切替
    if (state_.getLedAudio()) {
        // LEDを点灯させる
        if (!led_state.audio) {
            led_state.audio = true;
            setLed(LED_CONFIGS[1], true);
        }
        audio_led_off_timer = millis() + 50;
        state_.setLedAudio(false);
    }
    else {
        // 点灯中かつ消灯予定時刻を過ぎていたら
        if (led_state.audio && millis() > audio_led_off_timer) {
            // LEDを消す
            led_state.audio = false;
            setLed(LED_CONFIGS[1], false);
        }
    }
}