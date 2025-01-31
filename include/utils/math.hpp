#pragma once

#include <Arduino.h>

class AudioMath {
public:
    static inline float noteToFrequency(uint8_t note) {
        return 440.0f * pow(2.0f, (note - 69) / 12.0f);
    }

    static inline float velocityToAmplitude(uint8_t velocity) {
        return velocity / 127.0f;
    }

    static inline float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    static inline uint8_t bitPadding32(size_t size) {
        uint8_t shift = 0;
        while (size > 1) {
            size >>= 1;
            shift++;
        }
        return 32 - shift;
    }
};