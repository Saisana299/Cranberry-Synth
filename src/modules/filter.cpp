#include "modules/filter.h"

// 2次IIR(Biquad)フィルタ
void Filter::setLowPass(float cutoff, float resonance) {
    //todo 条件式
    float omega = 2.0f * M_PI * cutoff / SAMPLE_RATE;
    float alpha = sin(omega) / (2.0f * resonance);

    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos(omega);
    float a2 = 1.0f - alpha;
    float b0 = (1.0f - cos(omega)) / 2.0f;
    float b1 = 1.0f - cos(omega);
    float b2 = (1.0f - cos(omega)) / 2.0f;

    lpf_coefs_L.f0 = lpf_coefs_R.f0 = b0 / a0;
    lpf_coefs_L.f1 = lpf_coefs_R.f1 = b1 / a0;
    lpf_coefs_L.f2 = lpf_coefs_R.f2 = b2 / a0;
    lpf_coefs_L.f3 = lpf_coefs_R.f3 = a1 / a0;
    lpf_coefs_L.f4 = lpf_coefs_R.f4 = a2 / a0;
}

// 2次IIR(Biquad)フィルタ
void Filter::setHighPass(float cutoff, float resonance) {
    //todo 条件式
    float omega = 2.0f * M_PI * cutoff / SAMPLE_RATE;
    float alpha = sin(omega) / (2.0f * resonance);

    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos(omega);
    float a2 = 1.0f - alpha;
    float b0 = (1.0f + cos(omega)) / 2.0f;
    float b1 = -(1.0f + cos(omega));
    float b2 = (1.0f + cos(omega)) / 2.0f;

    hpf_coefs_L.f0 = hpf_coefs_R.f0 = b0 / a0;
    hpf_coefs_L.f1 = hpf_coefs_R.f1 = b1 / a0;
    hpf_coefs_L.f2 = hpf_coefs_R.f2 = b2 / a0;
    hpf_coefs_L.f3 = hpf_coefs_R.f3 = a1 / a0;
    hpf_coefs_L.f4 = hpf_coefs_R.f4 = a2 / a0;
}

int16_t Filter::processLpf(int16_t in, bool isR) {
    Filter::Coefs &coefs = (isR ? lpf_coefs_R : lpf_coefs_L);

    float out = (
        (coefs.f0 * in) + (coefs.f1 * coefs.in1) + (coefs.f2 * coefs.in2)
        - (coefs.f3 * coefs.out1) - (coefs.f4 * coefs.out2)
    );
    coefs.in2 = coefs.in1;
    coefs.in1 = in;
    coefs.out2 = coefs.out1;
    coefs.out1 = out;

    float mixed = (1.0f - lpf_mix) * in + lpf_mix * out;
    mixed = std::clamp(mixed, -32768.0f, 32767.0f);
    return static_cast<int16_t>(mixed);
}

int16_t Filter::processHpf(int16_t in, bool isR) {
    Filter::Coefs &coefs = (isR ? hpf_coefs_R : hpf_coefs_L);

    float out = (
        (coefs.f0 * in) + (coefs.f1 * coefs.in1) + (coefs.f2 * coefs.in2)
        - (coefs.f3 * coefs.out1) - (coefs.f4 * coefs.out2)
    );
    coefs.in2 = coefs.in1;
    coefs.in1 = in;
    coefs.out2 = coefs.out1;
    coefs.out1 = out;

    float mixed = (1.0f - hpf_mix) * in + hpf_mix * out;
    mixed = std::clamp(mixed, -32768.0f, 32767.0f);
    return static_cast<int16_t>(mixed);
}