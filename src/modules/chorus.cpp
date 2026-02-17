#include "modules/chorus.hpp"
#include <cmath>
#include <algorithm>

// サイン波テーブル (256エントリ, Q15: -32767 ~ +32767)
const int16_t Chorus::SINE_TABLE[SINE_TABLE_SIZE] = {
         0,    804,   1608,   2410,   3212,   4011,   4808,   5602,
      6393,   7179,   7962,   8739,   9512,  10278,  11039,  11793,
     12539,  13279,  14010,  14732,  15446,  16151,  16846,  17530,
     18204,  18868,  19519,  20159,  20787,  21403,  22005,  22594,
     23170,  23731,  24279,  24811,  25329,  25832,  26319,  26790,
     27245,  27683,  28105,  28510,  28898,  29268,  29621,  29956,
     30273,  30571,  30852,  31113,  31356,  31580,  31785,  31971,
     32137,  32285,  32412,  32521,  32609,  32678,  32728,  32757,
     32767,  32757,  32728,  32678,  32609,  32521,  32412,  32285,
     32137,  31971,  31785,  31580,  31356,  31113,  30852,  30571,
     30273,  29956,  29621,  29268,  28898,  28510,  28105,  27683,
     27245,  26790,  26319,  25832,  25329,  24811,  24279,  23731,
     23170,  22594,  22005,  21403,  20787,  20159,  19519,  18868,
     18204,  17530,  16846,  16151,  15446,  14732,  14010,  13279,
     12539,  11793,  11039,  10278,   9512,   8739,   7962,   7179,
      6393,   5602,   4808,   4011,   3212,   2410,   1608,    804,
         0,   -804,  -1608,  -2410,  -3212,  -4011,  -4808,  -5602,
     -6393,  -7179,  -7962,  -8739,  -9512, -10278, -11039, -11793,
    -12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530,
    -18204, -18868, -19519, -20159, -20787, -21403, -22005, -22594,
    -23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790,
    -27245, -27683, -28105, -28510, -28898, -29268, -29621, -29956,
    -30273, -30571, -30852, -31113, -31356, -31580, -31785, -31971,
    -32137, -32285, -32412, -32521, -32609, -32678, -32728, -32757,
    -32767, -32757, -32728, -32678, -32609, -32521, -32412, -32285,
    -32137, -31971, -31785, -31580, -31356, -31113, -30852, -30571,
    -30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683,
    -27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731,
    -23170, -22594, -22005, -21403, -20787, -20159, -19519, -18868,
    -18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
    -12539, -11793, -11039, -10278,  -9512,  -8739,  -7962,  -7179,
     -6393,  -5602,  -4808,  -4011,  -3212,  -2410,  -1608,   -804
};

/** @brief コーラスバッファをリセット */
void Chorus::reset() {
    for (uint32_t i = 0; i < CHORUS_BUFFER_SIZE; ++i) {
        buffer_L[i] = 0;
        buffer_R[i] = 0;
    }
    write_pos = 0;
    lfo_phase = 0;
    updatePhaseInc();
}

/**
 * @brief LFO速度を設定
 * @param r Rate (1-99 → 0.1Hz ~ 10Hz)
 */
void Chorus::setRate(uint8_t r) {
    rate = std::clamp<uint8_t>(r, CHORUS_RATE_MIN, CHORUS_RATE_MAX);
    updatePhaseInc();
}

/**
 * @brief 変調深さを設定
 * @param d Depth (0-99)
 */
void Chorus::setDepth(uint8_t d) {
    depth = std::clamp<uint8_t>(d, CHORUS_DEPTH_MIN, CHORUS_DEPTH_MAX);
}

/**
 * @brief ウェットミックスを設定
 * @param m Mix (Q15: 0-32767)
 */
void Chorus::setMix(Gain_t m) {
    mix = std::clamp<Gain_t>(m, 0, Q15_MAX);
}

/**
 * @brief Rateパラメータから位相増分を計算
 *
 * rate 1-99 → 0.1Hz - 10Hz (指数カーブ)
 * phase_inc = freq * UINT32_MAX / SAMPLE_RATE
 */
void Chorus::updatePhaseInc() {
    // rate=1 → 0.1Hz, rate=50 → ~1.5Hz, rate=99 → 10Hz
    // 指数マッピング: freq = 0.1 * (100)^((rate-1)/98) → 0.1 ~ 10Hz
    float freq = 0.1f * powf(100.0f, static_cast<float>(rate - 1) / 98.0f);
    // phase_inc = freq / SAMPLE_RATE * 2^32
    lfo_phase_inc = static_cast<uint32_t>((freq / SAMPLE_RATE) * 4294967296.0);
}

/**
 * @brief コーラス処理 (L/R同時)
 *
 * - バッファにドライ信号を書き込み
 * - L: LFO位相そのまま, R: LFO位相+90° でディレイ読み出し
 * - ドライ + ウェット×mix で出力
 *
 * @param left  L入力/出力
 * @param right R入力/出力
 */
void Chorus::process(Sample16_t& left, Sample16_t& right) {
    // バッファにドライ信号を書き込み
    buffer_L[write_pos] = left;
    buffer_R[write_pos] = right;

    // 変調量を計算 (depth 0-99 → 0 ~ MAX_MOD_SAMPLES サンプル)
    // mod_range: depth=0 → 0, depth=99 → MAX_MOD_SAMPLES (220 samples = 5ms)
    uint32_t mod_range = (static_cast<uint32_t>(depth) * MAX_MOD_SAMPLES) / CHORUS_DEPTH_MAX;

    // L: LFO位相そのまま, R: +90° (= +UINT32_MAX/4)
    int16_t lfo_l = getSineValue(lfo_phase);
    int16_t lfo_r = getSineValue(lfo_phase + 0x40000000U);  // +90°

    // LFO値 (-32767~+32767) → モジュレーション量 (Q8サンプル数)
    // delay = base_delay + lfo_value * mod_range / 32767
    // Q8形式で計算 (小数部8bit)
    int32_t mod_l = (static_cast<int32_t>(lfo_l) * static_cast<int32_t>(mod_range)) / Q15_MAX;
    int32_t mod_r = (static_cast<int32_t>(lfo_r) * static_cast<int32_t>(mod_range)) / Q15_MAX;

    uint32_t base_delay_q8 = BASE_DELAY_SAMPLES << 8;
    uint32_t delay_l_q8 = static_cast<uint32_t>(static_cast<int32_t>(base_delay_q8) + (mod_l << 8));
    uint32_t delay_r_q8 = static_cast<uint32_t>(static_cast<int32_t>(base_delay_q8) + (mod_r << 8));

    // 安全範囲にクランプ (1 ~ CHORUS_BUFFER_SIZE-2 サンプル, Q8)
    constexpr uint32_t MIN_DELAY_Q8 = 1 << 8;
    constexpr uint32_t MAX_DELAY_Q8 = (CHORUS_BUFFER_SIZE - 2) << 8;
    delay_l_q8 = std::clamp(delay_l_q8, MIN_DELAY_Q8, MAX_DELAY_Q8);
    delay_r_q8 = std::clamp(delay_r_q8, MIN_DELAY_Q8, MAX_DELAY_Q8);

    // 補間読み出し
    Sample16_t raw_wet_l = readInterpolated(buffer_L, delay_l_q8);
    Sample16_t raw_wet_r = readInterpolated(buffer_R, delay_r_q8);

    // L/Rウェット信号をクロスブレンド (93.75:6.25 = 15:1)
    // 位相干渉による片チャンネルのキャンセルを防止
    int32_t wet_l = (static_cast<int32_t>(raw_wet_l) * 15 + static_cast<int32_t>(raw_wet_r)) >> 4;
    int32_t wet_r = (static_cast<int32_t>(raw_wet_r) * 15 + static_cast<int32_t>(raw_wet_l)) >> 4;

    // ドライ + ウェット × mix (Q15乗算)
    int32_t out_l = static_cast<int32_t>(left) + ((wet_l * mix) >> Q15_SHIFT);
    int32_t out_r = static_cast<int32_t>(right) + ((wet_r * mix) >> Q15_SHIFT);

    left = static_cast<Sample16_t>(std::clamp<int32_t>(out_l, SAMPLE16_MIN, SAMPLE16_MAX));
    right = static_cast<Sample16_t>(std::clamp<int32_t>(out_r, SAMPLE16_MIN, SAMPLE16_MAX));

    // バッファポインタ更新
    write_pos = (write_pos + 1) % CHORUS_BUFFER_SIZE;

    // LFO位相更新
    lfo_phase += lfo_phase_inc;
}
