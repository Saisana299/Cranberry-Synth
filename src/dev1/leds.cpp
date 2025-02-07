#include "dev1/leds.hpp"

void Leds::init() {
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    RED_PORTSET = RED_BITMASK;
    GREEN_PORTSET = GREEN_BITMASK;
    BLUE_PORTSET = BLUE_BITMASK;
}

void Leds::process() {
    // LEDは共通アノード型
    auto& led_midi = State::led_midi;
    auto& led_audio = State::led_audio;

    if(led_midi != before_led_midi) {
        before_led_midi = led_midi;
        if(!led_midi) BLUE_PORTSET = BLUE_BITMASK;
        else BLUE_PORTCLR = BLUE_BITMASK;
    }

    if(led_audio != before_led_audio) {
        before_led_audio = led_audio;
        if(!led_audio) GREEN_PORTSET = GREEN_BITMASK;
        else GREEN_PORTCLR = GREEN_BITMASK;
    }
}