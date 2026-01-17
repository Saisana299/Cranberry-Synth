#include "modules/filter.hpp"

/**
 * @brief Biquad フィルタ係数を計算
 *
 * @param cutoff カットオフ周波数
 * @param resonance Q値
 * @param is_highpass true=ハイパス, false=ローパス
 * @return Coefs 計算された係数
 */
Filter::Coefs Filter::calculate_biquad(float cutoff, float resonance, bool is_highpass) {
    // 入力値の妥当性チェック
    cutoff = std::clamp(cutoff, CUTOFF_MIN, CUTOFF_MAX);
    resonance = std::clamp(resonance, RESONANCE_MIN, RESONANCE_MAX);

    // 角周波数と減衰係数を計算
    float omega = 2.0f * M_PI * cutoff / (float)SAMPLE_RATE;
    float sn, cs;

    sn = std::sin(omega);
    cs = std::cos(omega);

    float alpha = sn / (2.0f * resonance);

    // フィルタ係数（正規化）
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cs;
    float a2 = 1.0f - alpha;

    float b0, b1, b2;

    if(is_highpass) {
        // ハイパスフィルタ係数
        b0 = (1.0f + cs) * 0.5f;
        b1 = -(1.0f + cs);
        b2 = (1.0f + cs) * 0.5f;
    } else {
        // ローパスフィルタ係数
        b0 = (1.0f - cs) * 0.5f;
        b1 = 1.0f - cs;
        b2 = (1.0f - cs) * 0.5f;
    }

    float inv_a0 = 1.0f / a0;

    // 係数を正規化して固定小数点に変換
    Coefs c;
    c.b0 = (int32_t)((b0 * inv_a0) * COEF_SCALE);
    c.b1 = (int32_t)((b1 * inv_a0) * COEF_SCALE);
    c.b2 = (int32_t)((b2 * inv_a0) * COEF_SCALE);
    c.a1 = (int32_t)((a1 * inv_a0) * COEF_SCALE);
    c.a2 = (int32_t)((a2 * inv_a0) * COEF_SCALE);

    return c;
}

/**
 * @brief ローパスフィルター設定
 *
 * @param cutoff カットオフ周波数
 * @param resonance Q値
 */
void Filter::setLowPass(float cutoff, float resonance) {
    lpf_cutoff = cutoff;
    lpf_resonance = resonance;
    lpf_coefs = calculate_biquad(cutoff, resonance, false);
}

/**
 * @brief ハイパスフィルター設定
 *
 * @param cutoff カットオフ周波数
 * @param resonance Q値
 */
void Filter::setHighPass(float cutoff, float resonance) {
    // 低すぎるカットオフは固定小数点精度の問題でノイズが発生するため制限
    cutoff = std::max(cutoff, HPF_CUTOFF_MIN);
    hpf_cutoff = cutoff;
    hpf_resonance = resonance;
    hpf_coefs = calculate_biquad(cutoff, resonance, true);
}

void Filter::reset() {
    lpf_state_L = {}; lpf_state_R = {};
    hpf_state_L = {}; hpf_state_R = {};
}

FASTRUN void Filter::processBlock(Sample16_t* bufL, Sample16_t* bufR, size_t size) {
    // ローカル変数にコピーしてアクセス速度向上
    const bool lpf_active = (lpf_mix > 0);
    const bool hpf_active = (hpf_mix > 0);

    // 両方バイパスなら即リターン
    if (!lpf_active && !hpf_active) return;

    for (size_t i = 0; i < size; ++i) {
        Sample16_t l = bufL[i];
        Sample16_t r = bufR[i];

        // LPF適用
        if (lpf_active) {
            // Mix最大(Q15_MAX)のときは関数内で分岐最適化される
            l = process_with_mix(lpf_coefs, lpf_state_L, l, lpf_mix);
            r = process_with_mix(lpf_coefs, lpf_state_R, r, lpf_mix);
        }

        // HPF適用
        if (hpf_active) {
            l = process_with_mix(hpf_coefs, hpf_state_L, l, hpf_mix);
            r = process_with_mix(hpf_coefs, hpf_state_R, r, hpf_mix);
        }

        bufL[i] = l;
        bufR[i] = r;
    }
}