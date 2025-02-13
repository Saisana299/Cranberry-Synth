#include "modules/filter.hpp"

// 2次IIR(Biquad)フィルタ
/**
 * @brief ローパスフィルター設定
 *
 * @param cutoff カットオフ周波数
 * @param resonance Q値
 */
void Filter::setLowPass(float cutoff, float resonance) {
    //todo 条件式
    float omega = 2.0f * M_PI * cutoff / static_cast<float>(SAMPLE_RATE);
    float alpha = sin(omega) / (2.0f * resonance);

    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos(omega);
    float a2 = 1.0f - alpha;
    float b0 = (1.0f - cos(omega)) / 2.0f;
    float b1 = 1.0f - cos(omega);
    float b2 = (1.0f - cos(omega)) / 2.0f;

    lpf_coefs_L.f0 = lpf_coefs_R.f0 = static_cast<int32_t>((b0 / a0) * 65536.0f);
    lpf_coefs_L.f1 = lpf_coefs_R.f1 = static_cast<int32_t>((b1 / a0) * 65536.0f);
    lpf_coefs_L.f2 = lpf_coefs_R.f2 = static_cast<int32_t>((b2 / a0) * 65536.0f);
    lpf_coefs_L.f3 = lpf_coefs_R.f3 = static_cast<int32_t>((a1 / a0) * 65536.0f);
    lpf_coefs_L.f4 = lpf_coefs_R.f4 = static_cast<int32_t>((a2 / a0) * 65536.0f);
}

// 2次IIR(Biquad)フィルタ
/**
 * @brief ハイパスフィルター設定
 *
 * @param cutoff カットオフ周波数
 * @param resonance Q値
 */
void Filter::setHighPass(float cutoff, float resonance) {
    //todo 条件式
    float omega = 2.0f * M_PI * cutoff / static_cast<float>(SAMPLE_RATE);
    float alpha = sin(omega) / (2.0f * resonance);

    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos(omega);
    float a2 = 1.0f - alpha;
    float b0 = (1.0f + cos(omega)) / 2.0f;
    float b1 = -(1.0f + cos(omega));
    float b2 = (1.0f + cos(omega)) / 2.0f;

    hpf_coefs_L.f0 = hpf_coefs_R.f0 = static_cast<int32_t>((b0 / a0) * 65536.0f);
    hpf_coefs_L.f1 = hpf_coefs_R.f1 = static_cast<int32_t>((b1 / a0) * 65536.0f);
    hpf_coefs_L.f2 = hpf_coefs_R.f2 = static_cast<int32_t>((b2 / a0) * 65536.0f);
    hpf_coefs_L.f3 = hpf_coefs_R.f3 = static_cast<int32_t>((a1 / a0) * 65536.0f);
    hpf_coefs_L.f4 = hpf_coefs_R.f4 = static_cast<int32_t>((a2 / a0) * 65536.0f);
}

/**
 * @brief ローパスフィルタ処理
 *
 * @param in サンプル
 * @param is_r 右チャンネルかどうか
 * @return int16_t 処理後のサンプル
 */
int16_t Filter::processLpf(int16_t in, bool is_r) {
    Filter::Coefs &coefs = (is_r ? lpf_coefs_R : lpf_coefs_L);

    int32_t out = (
        (coefs.f0 * in) + (coefs.f1 * coefs.in1) + (coefs.f2 * coefs.in2)
        - (coefs.f3 * coefs.out1) - (coefs.f4 * coefs.out2)
    ) >> 16;
    coefs.in2 = coefs.in1;
    coefs.in1 = in;
    coefs.out2 = coefs.out1;
    coefs.out1 = out;

    int32_t mixed = ((1024 - lpf_mix) * in + lpf_mix * out) >> 10;
    mixed = std::clamp<int32_t>(mixed, -32768, 32767);
    return static_cast<int16_t>(mixed);
}

/**
 * @brief ハイパスフィルタ処理
 *
 * @param in サンプル
 * @param is_r 右チャンネルかどうか
 * @return int16_t 処理後のサンプル
 */
int16_t Filter::processHpf(int16_t in, bool is_r) {
    Filter::Coefs &coefs = (is_r ? hpf_coefs_R : hpf_coefs_L);

    int32_t out = (
        (coefs.f0 * in) + (coefs.f1 * coefs.in1) + (coefs.f2 * coefs.in2)
        - (coefs.f3 * coefs.out1) - (coefs.f4 * coefs.out2)
    ) >> 16;
    coefs.in2 = coefs.in1;
    coefs.in1 = in;
    coefs.out2 = coefs.out1;
    coefs.out1 = out;

    int32_t mixed = ((1024 - hpf_mix) * in + hpf_mix * out) >> 10;
    mixed = std::clamp<int32_t>(mixed, -32768, 32767);
    return static_cast<int16_t>(mixed);
}