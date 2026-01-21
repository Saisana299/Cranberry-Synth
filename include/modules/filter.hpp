#pragma once

#include "handlers/audio.hpp"
#include "types.hpp"

class Filter {
public:
    static constexpr float CUTOFF_MIN = 20.0f;
    static constexpr float CUTOFF_MAX = 20000.0f;
    static constexpr float HPF_CUTOFF_MIN = 100.0f;  // HPFは100Hz以上（固定小数点精度の都合）
    static constexpr float RESONANCE_MIN = 0.1f;
    static constexpr float RESONANCE_MAX = 10.0f;
    static constexpr float RESONANCE_DEFAULT = 0.70710678f; // 1.0 / sqrt(2)

private:
    // 固定小数点
    static constexpr int COEF_SHIFT = 14;
    static constexpr float COEF_SCALE = (1 << COEF_SHIFT);

    Gain_t lpf_mix = Q15_MAX;
    Gain_t hpf_mix = Q15_MAX;

    // パラメータ保存用
    float lpf_cutoff = 20000.0f;
    float lpf_resonance = RESONANCE_DEFAULT;
    float hpf_cutoff = 20.0f;
    float hpf_resonance = RESONANCE_DEFAULT;

    // 係数構造体
    struct Coefs {
        int32_t b0 = 0, b1 = 0, b2 = 0; // Feedforward
        int32_t a1 = 0, a2 = 0;         // Feedback
    };
    Coefs lpf_coefs = {};
    Coefs hpf_coefs = {};

    // 状態構造体
    struct State {
        Sample16_t x1 = 0, x2 = 0; // Input history
        Sample16_t y1 = 0, y2 = 0; // Output history
    };
    State lpf_state_L = {}, lpf_state_R = {};
    State hpf_state_L = {}, hpf_state_R = {};

    /**
     * @brief Biquad 係数を計算
     *
     * @param cutoff カットオフ周波数 (Hz)
     * @param resonance Q値
     * @param is_highpass true=ハイパス, false=ローパス
     * @return Coefs 計算された係数
     */
    static Coefs calculate_biquad(float cutoff, float resonance, bool is_highpass);

    /**
     * @brief Biquad フィルタ処理（共通ルーチン）
     *
     * @param coefs フィルタ係数
     * @param in 入力サンプル
     * @param mix ミックス量
     * @return Sample16_t 処理済みサンプル
     */
    inline Sample16_t process_biquad_core(const Coefs& c, State& s, Sample16_t in) const {
        int64_t acc = 0;

        // Feedforward: b0*x0 + b1*x1 + b2*x2
        acc += (int64_t)c.b0 * in;
        acc += (int64_t)c.b1 * s.x1;
        acc += (int64_t)c.b2 * s.x2;

        // Feedback: -a1*y1 - a2*y2
        // (Direct From I: y[n] = b*x - a*y)
        acc -= (int64_t)c.a1 * s.y1;
        acc -= (int64_t)c.a2 * s.y2;

        // 固定小数点のシフトを戻す
        int32_t out = (int32_t)(acc >> COEF_SHIFT);

        if (out > SAMPLE16_MAX) out = SAMPLE16_MAX;
        else if (out < SAMPLE16_MIN) out = SAMPLE16_MIN;

        // 履歴更新
        s.x2 = s.x1;
        s.x1 = in;
        s.y2 = s.y1;
        s.y1 = (Sample16_t)out;

        return (Sample16_t)out;
    }

    inline Sample16_t process_with_mix(const Coefs& c, State& s, Sample16_t in, Gain_t mix) const {
        // Mixが0なら計算自体をスキップ (CPU負荷削減)
        if (mix <= 0) {
            return in;
        }

        Sample16_t filtered = process_biquad_core(c, s, in);

        // Mixが最大なら計算結果をそのまま返す
        if (mix >= Q15_MAX) return filtered;

        // Dry/Wet Mix (Q15)
        int32_t mixed = ((int32_t)(Q15_MAX - mix) * in + (int32_t)mix * filtered) >> Q15_SHIFT;

        // 最終段の安全クリップ
        if (mixed > SAMPLE16_MAX) return SAMPLE16_MAX;
        if (mixed < SAMPLE16_MIN) return SAMPLE16_MIN;
        return (Sample16_t)mixed;
    }

public:
    Filter() {
        setLowPass(20000.0f, RESONANCE_DEFAULT);
        setHighPass(120.0f, RESONANCE_DEFAULT);
    }

    /**
     * @brief ローパスフィルタ設定
     *
     * @param cutoff カットオフ周波数 (Hz) [20-20000]
     * @param resonance Q値 [0.1-10.0]
     */
    void setLowPass(float cutoff, float resonance = RESONANCE_DEFAULT);

    /**
     * @brief ハイパスフィルタ設定
     *
     * @param cutoff カットオフ周波数 (Hz) [20-20000]
     * @param resonance Q値 [0.1-10.0]
     */
    void setHighPass(float cutoff, float resonance = RESONANCE_DEFAULT);

    // 状態リセット
    void reset();

    /**
     * @brief フィルタのドライ/ウェット混合比設定
     *
     * @param mix ミックス量 [0-Q15_MAX] (0=ドライ, Q15_MAX=ウェット)
     */
    void setLpfMix(Gain_t mix) { lpf_mix = std::clamp<Gain_t>(mix, 0, Q15_MAX); }
    void setHpfMix(Gain_t mix) { hpf_mix = std::clamp<Gain_t>(mix, 0, Q15_MAX); }

    /**
     * @brief フィルタ処理
     *
     * @param in 入力サンプル
     * @return Sample16_t 処理済みサンプル
     */
    FASTRUN Sample16_t processLpfL(Sample16_t in) { return process_with_mix(lpf_coefs, lpf_state_L, in, lpf_mix);}
    FASTRUN Sample16_t processLpfR(Sample16_t in) { return process_with_mix(lpf_coefs, lpf_state_R, in, lpf_mix); }
    FASTRUN Sample16_t processHpfL(Sample16_t in) { return process_with_mix(hpf_coefs, hpf_state_L, in, hpf_mix); }
    FASTRUN Sample16_t processHpfR(Sample16_t in) { return process_with_mix(hpf_coefs, hpf_state_R, in, hpf_mix); }

    FASTRUN void processBlock(Sample16_t* bufL, Sample16_t* bufR, size_t size);

    // パラメータ取得
    float getLpfCutoff() const { return lpf_cutoff; }
    float getLpfResonance() const { return lpf_resonance; }
    Gain_t getLpfMix() const { return lpf_mix; }
    float getHpfCutoff() const { return hpf_cutoff; }
    float getHpfResonance() const { return hpf_resonance; }
    Gain_t getHpfMix() const { return hpf_mix; }
};