#ifndef AUDIO_MATH_H
#define AUDIO_MATH_H

class AudioMath {
public:
    static inline float noteToFrequency(uint8_t note) {
        return 440.0 * pow(2.0, (note - 69) / 12.0);
    }
};

#endif