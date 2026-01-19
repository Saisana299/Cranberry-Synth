#pragma once

#include <algorithm>
#include <cstdint>
#include <cmath>

// ============================================
// 固定小数点型定義
// ============================================

// === Q23形式:  オーディオ経路の内部演算用 ===
// 範囲: -1.0 ～ +1.0 = -8388607 ～ +8388607 (対称)
// 精度: 約0.00000012 (144dB SNR)
using Audio24_t = int32_t;
constexpr int Q23_SHIFT = 23;
constexpr Audio24_t Q23_ONE = 1 << Q23_SHIFT;          // 8388608 = 1.0
constexpr Audio24_t Q23_MAX = 8388607;                 // 最大値
constexpr Audio24_t Q23_MIN = -8388607;                // 最小値 (対称)

// === Q15形式: レベル/ゲイン用 ===
// 範囲: 0.0 ～ 1.0 = 0 ～ 32767
// 精度: 約0.00003 (96dB SNR)
using Gain_t = int16_t;
constexpr int Q15_SHIFT = 15;
constexpr Gain_t Q15_ONE = 1 << Q15_SHIFT;             // 32767 = 1.0
constexpr Gain_t Q15_MAX = 32767;
constexpr Gain_t Q15_ZERO = 0;

// === Q31形式: 位相変調の高精度計算用 ===
// 範囲: -1.0 ～ +1.0 = -2147483647 ～ +2147483647 (対称)
using ModDepth_t = int32_t;
constexpr int Q31_SHIFT = 31;
constexpr ModDepth_t Q31_ONE = 0x7FFFFFFF;             // 2147483647 = 1.0
constexpr ModDepth_t Q31_MAX = 2147483647;             // 最大値
constexpr ModDepth_t Q31_MIN = -2147483647;            // 最小値 (対称)

// === 位相:  32bit符号なし ===
using Phase_t = uint32_t;
constexpr Phase_t PHASE_MAX = 0xFFFFFFFF;

// === エンベロープレベル: Q24対数形式 ===
// 大きいほど音が大きい（通常の減衰量とは逆）
// 2^24 = 1オクターブ = 2倍の音量変化
using EnvLevel_t = int32_t;
constexpr int ENV_Q24_SHIFT = 24;
constexpr EnvLevel_t ENV_LEVEL_MIN = 16 << 16;     // 最小レベル (実質無音)
constexpr EnvLevel_t ENV_LEVEL_MAX = 3840 << 16;   // 最大レベル
constexpr EnvLevel_t ENV_JUMPTARGET = 1716 << 16;  // アタックカーブ用閾値

// === 16bit DAC出力 ===
using Sample16_t = int16_t;
constexpr Sample16_t SAMPLE16_MAX = 32767;
constexpr Sample16_t SAMPLE16_MIN = -32767;                // 対称

// ============================================
// 変換関数
// ============================================

/**
 * @brief Q23 → 16bit DAC出力
 * @param x Q23オーディオサンプル
 * @return 16bit DAC用サンプル
 */
inline Sample16_t Q23_to_Sample16(Audio24_t x) {
    // 23bit → 15bit (8bitシフト)
    int32_t shifted = x >> 8;

    // クリッピング
    if (shifted > SAMPLE16_MAX) return SAMPLE16_MAX;
    if (shifted < SAMPLE16_MIN) return SAMPLE16_MIN;

    return static_cast<Sample16_t>(shifted);
}

/**
 * @brief 16bit → Q23変換
 * @param x 16bitサンプル
 * @return Q23サンプル
 */
inline Audio24_t Sample16_to_Q23(Sample16_t x) {
    return static_cast<Audio24_t>(x) << 8;
}

/**
 * @brief float → Q23変換
 * @param x 浮動小数点 (-1.0 ～ +1.0)
 * @return Q23サンプル
 */
inline Audio24_t float_to_Q23(float x) {
    return static_cast<Audio24_t>(x * Q23_ONE);
}

/**
 * @brief Q23 → float変換
 * @param x Q23サンプル
 * @return 浮動小数点 (-1.0 ～ +1.0)
 */
inline float Q23_to_float(Audio24_t x) {
    return static_cast<float>(x) / Q23_ONE;
}

/**
 * @brief float → Q15変換
 * @param x 浮動小数点 (0.0 ～ 1.0)
 * @return Q15ゲイン
 */
inline Gain_t float_to_Q15(float x) {
    return static_cast<Gain_t>(x * Q15_ONE);
}

/**
 * @brief Q15 → float変換
 * @param x Q15ゲイン
 * @return 浮動小数点 (0.0 ～ 1.0)
 */
inline float Q15_to_float(Gain_t x) {
    return static_cast<float>(x) / Q15_ONE;
}

/**
 * @brief Q23 × Q15 = Q23 乗算
 * @param audio Q23オーディオサンプル
 * @param gain Q15ゲイン
 * @return Q23オーディオサンプル
 * @note (Q23 × Q15) >> 15 = Q23
 */
inline Audio24_t Q23_mul_Q15(Audio24_t audio, Gain_t gain) {
    // 64bit演算で精度確保
    int64_t result = (static_cast<int64_t>(audio) * static_cast<int64_t>(gain)) >> Q15_SHIFT;

    // クリッピング
    if (result > Q23_MAX) return Q23_MAX;
    if (result < Q23_MIN) return Q23_MIN;

    return static_cast<Audio24_t>(result);
}

/**
 * @brief Q15 × Q15 = Q15 乗算
 * @param a Q15ゲイン
 * @param b Q15ゲイン
 * @return Q15ゲイン
 */
inline Gain_t Q15_mul_Q15(Gain_t a, Gain_t b) {
    int32_t result = (static_cast<int32_t>(a) * static_cast<int32_t>(b)) >> Q15_SHIFT;

    if (result > Q15_MAX) return Q15_MAX;
    if (result < 0) return 0;

    return static_cast<Gain_t>(result);
}

/**
 * @brief Q23サンプル加算（クリッピング付き）
 * @param a Q23サンプル
 * @param b Q23サンプル
 * @return Q23サンプル
 */
inline Audio24_t Q23_add(Audio24_t a, Audio24_t b) {
    int64_t result = static_cast<int64_t>(a) + static_cast<int64_t>(b);

    if (result > Q23_MAX) return Q23_MAX;
    if (result < Q23_MIN) return Q23_MIN;

    return static_cast<Audio24_t>(result);
}

/**
 * @brief 線形補間 (Q23)
 * @param y0 Q23サンプル0
 * @param y1 Q23サンプル1
 * @param frac 補間係数 (0～65535, 16bit)
 * @return Q23サンプル
 */
inline Audio24_t Q23_lerp(Audio24_t y0, Audio24_t y1, uint16_t frac) {
    // (y1 - y0) * frac / 65536 + y0
    int64_t diff = static_cast<int64_t>(y1) - static_cast<int64_t>(y0);
    int64_t interpolated = y0 + ((diff * frac) >> 16);

    if (interpolated > Q23_MAX) return Q23_MAX;
    if (interpolated < Q23_MIN) return Q23_MIN;

    return static_cast<Audio24_t>(interpolated);
}

/**
 * @brief 周波数 → 位相増分変換
 * @param freq 周波数 (Hz)
 * @param sample_rate サンプリングレート (Hz)
 * @return 位相増分
 */
inline Phase_t freq_to_phase_delta(float freq, float sample_rate) {
    // delta = freq / sample_rate * 2^32
    return static_cast<Phase_t>((freq / sample_rate) * 4294967296.0);
}

/**
 * @brief FMレベルカーブ (0～99 → Q15)
 * @param level FMレベル (0～99)
 * @return Q15ゲイン
 * @note 非線形カーブ（指数的）
 */
inline Gain_t fm_level_to_Q15(uint8_t level) {
    if (level == 0) return 0;
    if (level >= 99) return Q15_MAX;

    // 非線形カーブ: (level/99)^1.5
    float normalized = level / 99.0f;
    float curved = powf(normalized, 1.5f);

    return float_to_Q15(curved);
}

/**
 * @brief Q15 → FMレベル逆変換
 * @param gain Q15ゲイン
 * @return FMレベル (0～99)
 */
inline uint8_t Q15_to_fm_level(Gain_t gain) {
    if (gain <= 0) return 0;
    if (gain >= Q15_MAX) return 99;

    float normalized = Q15_to_float(gain);
    float original = powf(normalized, 1.0f / 1.5f);  // 逆変換

    return static_cast<uint8_t>(original * 99.0f + 0.5f);
}