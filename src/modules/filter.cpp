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
    float omega = 2.0f * M_PI * cutoff / static_cast<float>(SAMPLE_RATE);
    float alpha = std::sin(omega) / (2.0f * resonance);

    // フィルタ係数（正規化）
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * std::cos(omega);
    float a2 = 1.0f - alpha;

    float b0, b1, b2;

    if(is_highpass) {
        // ハイパスフィルタ係数
        b0 = (1.0f + std::cos(omega)) / 2.0f;
        b1 = -(1.0f + std::cos(omega));
        b2 = (1.0f + std::cos(omega)) / 2.0f;
    } else {
        // ローパスフィルタ係数
        b0 = (1.0f - std::cos(omega)) / 2.0f;
        b1 = 1.0f - std::cos(omega);
        b2 = (1.0f - std::cos(omega)) / 2.0f;
    }

    // 係数を正規化して固定小数点に変換
    Coefs coefs;
    coefs.f0 = static_cast<int32_t>((b0 / a0) * COEF_SCALE);
    coefs.f1 = static_cast<int32_t>((b1 / a0) * COEF_SCALE);
    coefs.f2 = static_cast<int32_t>((b2 / a0) * COEF_SCALE);
    coefs.f3 = static_cast<int32_t>((a1 / a0) * COEF_SCALE);
    coefs.f4 = static_cast<int32_t>((a2 / a0) * COEF_SCALE);

    return coefs;
}

/**
 * @brief ローパスフィルター設定
 *
 * @param cutoff カットオフ周波数
 * @param resonance Q値
 */
void Filter::setLowPass(float cutoff, float resonance) {
    auto coefs = calculate_biquad(cutoff, resonance, false);
    lpf_coefs_L = coefs;
    lpf_coefs_R = coefs;
}

/**
 * @brief ハイパスフィルター設定
 *
 * @param cutoff カットオフ周波数
 * @param resonance Q値
 */
void Filter::setHighPass(float cutoff, float resonance) {
    auto coefs = calculate_biquad(cutoff, resonance, true);
    hpf_coefs_L = coefs;
    hpf_coefs_R = coefs;
}