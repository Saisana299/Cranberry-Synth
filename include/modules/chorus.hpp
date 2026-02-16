#pragma once

#include "types.hpp"
#include "handlers/audio.hpp"

/**
 * @brief ステレオコーラスエフェクト
 *
 * 短いモジュレーテッドディレイラインによるコーラス効果
 * L/Rで位相が異なるLFOでディレイタイムを揺らし、ステレオ感を演出
 *
 * パラメータ:
 * - Rate:  LFO速度 (1-99 → 0.1Hz-10Hz)
 * - Depth: 変調の深さ (0-99)
 * - Mix:   ウェット信号のレベル (Q15: 0-32767)
 */

// コーラス用ディレイバッファ (最大30ms)
// base delay ~7ms + modulation ~±5ms → 最大 ~12ms 使用
// 余裕を見て30ms確保
constexpr uint32_t CHORUS_BUFFER_MS = 30;
constexpr uint32_t CHORUS_BUFFER_SIZE = (CHORUS_BUFFER_MS * SAMPLE_RATE) / 1000; // 1323 samples

// パラメータ範囲
constexpr uint8_t CHORUS_RATE_MIN = 1;
constexpr uint8_t CHORUS_RATE_MAX = 99;
constexpr uint8_t CHORUS_DEPTH_MIN = 0;
constexpr uint8_t CHORUS_DEPTH_MAX = 99;

class Chorus {
private:
    // ディレイバッファ (L/R)
    Sample16_t buffer_L[CHORUS_BUFFER_SIZE] = {};
    Sample16_t buffer_R[CHORUS_BUFFER_SIZE] = {};
    uint32_t write_pos = 0;

    // 内部LFO (位相アキュムレーター方式)
    uint32_t lfo_phase = 0;           // 0 ~ UINT32_MAX
    uint32_t lfo_phase_inc = 0;       // 位相増分（Rateから計算）

    // パラメータ
    uint8_t rate = 20;                // LFO速度 (1-99)
    uint8_t depth = 50;               // 変調深さ (0-99)
    Gain_t mix = 16384;               // ウェットミックス (Q15, default 50%)

    // 内部定数
    // ベースディレイ: 7ms = 308 samples @ 44100Hz
    static constexpr uint32_t BASE_DELAY_SAMPLES = (7 * SAMPLE_RATE) / 1000;
    // 最大変調幅: 5ms = 220 samples @ 44100Hz
    static constexpr uint32_t MAX_MOD_SAMPLES = (5 * SAMPLE_RATE) / 1000;

    // サイン波テーブル (256エントリ, Q15)
    static constexpr uint16_t SINE_TABLE_SIZE = 256;
    static const int16_t SINE_TABLE[SINE_TABLE_SIZE];

    /**
     * @brief サイン波LFO値を取得
     * @param phase 位相 (32bit)
     * @return int16_t サイン値 (-32767 ~ +32767, Q15)
     */
    inline int16_t getSineValue(uint32_t phase) const {
        // 上位8bitをインデックスとして使い、線形補間
        uint8_t idx = phase >> 24;
        uint8_t next_idx = idx + 1; // 自然にラップ
        int32_t frac = (phase >> 8) & 0xFFFF; // 補間用小数部 (16bit)

        int32_t a = SINE_TABLE[idx];
        int32_t b = SINE_TABLE[next_idx];
        return static_cast<int16_t>(a + ((b - a) * frac >> 16));
    }

    /**
     * @brief ディレイバッファからの線形補間読み出し
     * @param buffer ディレイバッファ
     * @param delay_samples ディレイサンプル数 (Q8固定小数点: 上位=整数, 下位8bit=小数)
     * @return Sample16_t 補間されたサンプル
     */
    inline Sample16_t readInterpolated(const Sample16_t* buffer, uint32_t delay_q8) const {
        uint32_t delay_int = delay_q8 >> 8;
        uint32_t frac = delay_q8 & 0xFF;

        // write_posから遡って読む
        uint32_t idx0 = (write_pos + CHORUS_BUFFER_SIZE - delay_int) % CHORUS_BUFFER_SIZE;
        uint32_t idx1 = (write_pos + CHORUS_BUFFER_SIZE - delay_int - 1) % CHORUS_BUFFER_SIZE;

        int32_t s0 = buffer[idx0];
        int32_t s1 = buffer[idx1];

        // 線形補間: s0 + (s1 - s0) * frac / 256
        return static_cast<Sample16_t>(s0 + (((s1 - s0) * static_cast<int32_t>(frac)) >> 8));
    }

    /** @brief Rateパラメータから位相増分を計算 */
    void updatePhaseInc();

public:
    void reset();
    void setRate(uint8_t rate);
    void setDepth(uint8_t depth);
    void setMix(Gain_t mix);

    /** @brief L/Rを同時に処理 (LFO位相を1回だけ進める) */
    void process(Sample16_t& left, Sample16_t& right);

    // パラメータ取得
    uint8_t getRate() const { return rate; }
    uint8_t getDepth() const { return depth; }
    Gain_t getMix() const { return mix; }
};
