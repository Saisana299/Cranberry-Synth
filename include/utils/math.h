#ifndef AUDIO_MATH_H
#define AUDIO_MATH_H

class AudioMath {
public:
    static inline float noteToFrequency(uint8_t note) {
        return 440.0f * pow(2.0f, (note - 69) / 12.0f);
    }

    static inline float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    static inline float randomFloat4(float min, float max) {
        return min + (random(0, 10000) / 10000.0) * (max - min);
    }
};

#endif