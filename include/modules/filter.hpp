#pragma once

#include "handlers/audio.hpp"

constexpr float CUTOFF_MIN = 20.0f;
constexpr float CUTOFF_MAX = 20000.0f;
constexpr float RESONANCE_MIN = 0.1f;
constexpr float RESONANCE_MAX = 10.0f;
constexpr float RESONANCE_DEFAULT = 1.0f / 1.41421356f;

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

    static constexpr int COEF_SHIFT = 16;
    static constexpr int MIX_SHIFT = 10;
    static constexpr float COEF_SCALE = (1 << COEF_SHIFT);

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
     * @return int16_t 処理済みサンプル
     */
    inline int16_t process_biquad(Coefs& coefs, int16_t in, int16_t mix) const {
        // IIR フィルタ計算
        int32_t out = (
            (coefs.f0 * in) + (coefs.f1 * coefs.in1) + (coefs.f2 * coefs.in2)
            - (coefs.f3 * coefs.out1) - (coefs.f4 * coefs.out2)
        ) >> COEF_SHIFT;

        // 履歴更新
        coefs.in2 = coefs.in1;
        coefs.in1 = in;
        coefs.out2 = coefs.out1;
        coefs.out1 = out;

        // ドライ/ウェット ミックス
        int32_t mixed = ((1024 - mix) * in + mix * out) >> MIX_SHIFT;
        mixed = std::clamp<int32_t>(mixed, -32768, 32767);

        return static_cast<int16_t>(mixed);
    }

public:
    /**
     * @brief ローパスフィルタ設定
     *
     * @param cutoff カットオフ周波数 (Hz) [20-20000]
     * @param resonance Q値 [0.1-10.0]
     */
    void setLowPass(float cutoff = 2000.0f, float resonance = RESONANCE_DEFAULT);

    /**
     * @brief ハイパスフィルタ設定
     *
     * @param cutoff カットオフ周波数 (Hz) [20-20000]
     * @param resonance Q値 [0.1-10.0]
     */
    void setHighPass(float cutoff = 500.0f, float resonance = RESONANCE_DEFAULT);

    /**
     * @brief ローパスフィルタ処理（左チャンネル）
     *
     * @param in 入力サンプル
     * @return int16_t 処理済みサンプル
     */
    int16_t processLpfL(int16_t in) {
        return process_biquad(lpf_coefs_L, in, lpf_mix);
    }

    /**
     * @brief ローパスフィルタ処理（右チャンネル）
     *
     * @param in 入力サンプル
     * @return int16_t 処理済みサンプル
     */
    int16_t processLpfR(int16_t in) {
        return process_biquad(lpf_coefs_R, in, lpf_mix);
    }

    /**
     * @brief ハイパスフィルタ処理（左チャンネル）
     *
     * @param in 入力サンプル
     * @return int16_t 処理済みサンプル
     */
    int16_t processHpfL(int16_t in) {
        return process_biquad(hpf_coefs_L, in, hpf_mix);
    }

    /**
     * @brief ハイパスフィルタ処理（右チャンネル）
     *
     * @param in 入力サンプル
     * @return int16_t 処理済みサンプル
     */
    int16_t processHpfR(int16_t in) {
        return process_biquad(hpf_coefs_R, in, hpf_mix);
    }

    /**
     * @brief ローパスフィルタのドライ/ウェット混合比設定
     *
     * @param mix ミックス量 [0-1024] (0=ドライ, 1024=ウェット)
     */
    inline void setLpfMix(int16_t mix) {
        lpf_mix = std::clamp<int16_t>(mix, 0, 1024);
    }

    /**
     * @brief ハイパスフィルタのドライ/ウェット混合比設定
     *
     * @param mix ミックス量 [0-1024] (0=ドライ, 1024=ウェット)
     */
    inline void setHpfMix(int16_t mix) {
        hpf_mix = std::clamp<int16_t>(mix, 0, 1024);
    }
};