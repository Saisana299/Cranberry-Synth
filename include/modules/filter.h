#pragma once

#include "handlers/audio.h"

class Filter {
public:
    void setLowPass(float cutoff = 2000.0f, float resonance = 1.0f/sqrt(2.0f));
    void setHighPass(float cutoff = 500.0f, float resonance = 1.0f/sqrt(2.0f));
    int16_t processLpf(int16_t in, bool isR);
    int16_t processHpf(int16_t in, bool isR);

private:
    float lpf_mix = 1.0f;
    float hpf_mix = 1.0f;

    struct Coefs {
        float f0, f1, f2, f3, f4;
        float in1, in2;
        float out1, out2;
    };
    Coefs lpf_coefs_L = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    Coefs lpf_coefs_R = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    Coefs hpf_coefs_L = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    Coefs hpf_coefs_R = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
};