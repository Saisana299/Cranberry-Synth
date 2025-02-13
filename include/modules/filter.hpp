#pragma once

#include "handlers/audio.hpp"

class Filter {
private:
    int16_t lpf_mix = 1 << 10;
    int16_t hpf_mix = 1 << 10;

    struct Coefs {
        int32_t f0   = 0, f1   = 0, f2 = 0, f3 = 0, f4 = 0;
        int32_t in1  = 0, in2  = 0;
        int32_t out1 = 0, out2 = 0;
    };
    Coefs lpf_coefs_L = {}, lpf_coefs_R = {};
    Coefs hpf_coefs_L = {}, hpf_coefs_R = {};

public:
    void setLowPass(float cutoff = 2000.0f, float resonance = 1.0f/sqrt(2.0f));
    void setHighPass(float cutoff = 500.0f, float resonance = 1.0f/sqrt(2.0f));
    int16_t processLpf(int16_t in, bool is_r);
    int16_t processHpf(int16_t in, bool is_r);
};