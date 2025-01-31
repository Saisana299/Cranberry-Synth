#pragma once

#include "handlers/audio.hpp"

class Filter {
private:
    float lpf_mix = 1.0f;
    float hpf_mix = 1.0f;

    struct Coefs {
        float f0 = 0.0f, f1 = 0.0f, f2 = 0.0f, f3 = 0.0f, f4 = 0.0f;
        float in1 = 0.0f, in2 = 0.0f;
        float out1 = 0.0f, out2 = 0.0f;
    };
    Coefs lpf_coefs_L = {}, lpf_coefs_R = {};
    Coefs hpf_coefs_L = {}, hpf_coefs_R = {};

public:
    void setLowPass(float cutoff = 2000.0f, float resonance = 1.0f/sqrt(2.0f));
    void setHighPass(float cutoff = 500.0f, float resonance = 1.0f/sqrt(2.0f));
    int16_t processLpf(int16_t in, bool isR);
    int16_t processHpf(int16_t in, bool isR);
};