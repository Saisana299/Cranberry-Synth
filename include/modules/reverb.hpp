#pragma once

#include "types.hpp"
#include "handlers/audio.hpp"

/**
 * @brief Freeverbベースのステレオリバーブエフェクト
 *
 * 8つの並列コムフィルタ + 4つの直列オールパスフィルタによるアルゴリズミックリバーブ。
 * L/Rで異なるディレイ長を使用してステレオ感を演出。
 *
 * パラメータ:
 * - RoomSize: 残響の長さ (0-99)
 * - Damping:  高域減衰量 (0-99)
 * - Mix:      ウェット信号のレベル (Q15: 0-32767)
 */

// パラメータ範囲
constexpr uint8_t REVERB_ROOM_MIN = 0;
constexpr uint8_t REVERB_ROOM_MAX = 99;
constexpr uint8_t REVERB_DAMP_MIN = 0;
constexpr uint8_t REVERB_DAMP_MAX = 99;

// --- Freeverb コムフィルタ / オールパスフィルタのバッファ長 ---
// オリジナル Freeverb の定数 (44100Hz基準)
constexpr uint16_t COMB_TUNING_L1 = 1116;
constexpr uint16_t COMB_TUNING_L2 = 1188;
constexpr uint16_t COMB_TUNING_L3 = 1277;
constexpr uint16_t COMB_TUNING_L4 = 1356;
constexpr uint16_t COMB_TUNING_L5 = 1422;
constexpr uint16_t COMB_TUNING_L6 = 1491;
constexpr uint16_t COMB_TUNING_L7 = 1557;
constexpr uint16_t COMB_TUNING_L8 = 1617;

constexpr uint16_t ALLPASS_TUNING_L1 = 556;
constexpr uint16_t ALLPASS_TUNING_L2 = 441;
constexpr uint16_t ALLPASS_TUNING_L3 = 341;
constexpr uint16_t ALLPASS_TUNING_L4 = 225;

// ステレオスプレッド (R側にオフセット)
constexpr uint16_t STEREO_SPREAD = 23;

constexpr uint16_t COMB_TUNING_R1 = COMB_TUNING_L1 + STEREO_SPREAD;
constexpr uint16_t COMB_TUNING_R2 = COMB_TUNING_L2 + STEREO_SPREAD;
constexpr uint16_t COMB_TUNING_R3 = COMB_TUNING_L3 + STEREO_SPREAD;
constexpr uint16_t COMB_TUNING_R4 = COMB_TUNING_L4 + STEREO_SPREAD;
constexpr uint16_t COMB_TUNING_R5 = COMB_TUNING_L5 + STEREO_SPREAD;
constexpr uint16_t COMB_TUNING_R6 = COMB_TUNING_L6 + STEREO_SPREAD;
constexpr uint16_t COMB_TUNING_R7 = COMB_TUNING_L7 + STEREO_SPREAD;
constexpr uint16_t COMB_TUNING_R8 = COMB_TUNING_L8 + STEREO_SPREAD;

constexpr uint16_t ALLPASS_TUNING_R1 = ALLPASS_TUNING_L1 + STEREO_SPREAD;
constexpr uint16_t ALLPASS_TUNING_R2 = ALLPASS_TUNING_L2 + STEREO_SPREAD;
constexpr uint16_t ALLPASS_TUNING_R3 = ALLPASS_TUNING_L3 + STEREO_SPREAD;
constexpr uint16_t ALLPASS_TUNING_R4 = ALLPASS_TUNING_L4 + STEREO_SPREAD;

// 総バッファサイズ (概算: ~25500サンプル = ~50KB)
constexpr uint32_t REVERB_TOTAL_SAMPLES =
    COMB_TUNING_L1 + COMB_TUNING_L2 + COMB_TUNING_L3 + COMB_TUNING_L4 +
    COMB_TUNING_L5 + COMB_TUNING_L6 + COMB_TUNING_L7 + COMB_TUNING_L8 +
    COMB_TUNING_R1 + COMB_TUNING_R2 + COMB_TUNING_R3 + COMB_TUNING_R4 +
    COMB_TUNING_R5 + COMB_TUNING_R6 + COMB_TUNING_R7 + COMB_TUNING_R8 +
    ALLPASS_TUNING_L1 + ALLPASS_TUNING_L2 + ALLPASS_TUNING_L3 + ALLPASS_TUNING_L4 +
    ALLPASS_TUNING_R1 + ALLPASS_TUNING_R2 + ALLPASS_TUNING_R3 + ALLPASS_TUNING_R4;

// --- コムフィルタ (ローパスフィードバック付き) ---
template <uint16_t SIZE>
struct CombFilter {
    Sample16_t buffer[SIZE] = {};
    uint16_t index = 0;
    int16_t filterstore = 0;  // ローパスフィルタの状態

    inline Sample16_t process(Sample16_t input, int16_t feedback_q15, int16_t damp1_q15, int16_t damp2_q15) {
        Sample16_t output = buffer[index];

        // 1次ローパスフィルタ: filterstore = output*(1-damp) + filterstore*damp
        int32_t filt = (static_cast<int32_t>(output) * damp2_q15 +
                        static_cast<int32_t>(filterstore) * damp1_q15) >> 15;
        filterstore = static_cast<int16_t>(filt);

        // フィードバック付き書き込み: input + filtered_output * feedback
        int32_t fb = static_cast<int32_t>(filterstore) * feedback_q15 >> 15;
        int32_t sum = static_cast<int32_t>(input) + fb;
        // クリッピング
        if (sum > 32767) sum = 32767;
        if (sum < -32767) sum = -32767;
        buffer[index] = static_cast<Sample16_t>(sum);

        if (++index >= SIZE) index = 0;
        return output;
    }

    void clear() {
        for (uint16_t i = 0; i < SIZE; ++i) buffer[i] = 0;
        index = 0;
        filterstore = 0;
    }
};

// --- オールパスフィルタ ---
template <uint16_t SIZE>
struct AllpassFilter {
    Sample16_t buffer[SIZE] = {};
    uint16_t index = 0;

    // 固定フィードバック係数 0.5 (Q15 = 16384)
    static constexpr int16_t ALLPASS_FEEDBACK = 16384;

    inline Sample16_t process(Sample16_t input) {
        Sample16_t bufout = buffer[index];

        // output = -input + bufout
        int32_t output = static_cast<int32_t>(bufout) - static_cast<int32_t>(input);
        if (output > 32767) output = 32767;
        if (output < -32767) output = -32767;

        // buffer[index] = input + bufout * feedback
        int32_t fb = (static_cast<int32_t>(bufout) * ALLPASS_FEEDBACK) >> 15;
        int32_t sum = static_cast<int32_t>(input) + fb;
        if (sum > 32767) sum = 32767;
        if (sum < -32767) sum = -32767;
        buffer[index] = static_cast<Sample16_t>(sum);

        if (++index >= SIZE) index = 0;
        return static_cast<Sample16_t>(output);
    }

    void clear() {
        for (uint16_t i = 0; i < SIZE; ++i) buffer[i] = 0;
        index = 0;
    }
};

class Reverb {
private:
    // コムフィルタ (8本 × L/R)
    CombFilter<COMB_TUNING_L1> comb_L1;
    CombFilter<COMB_TUNING_L2> comb_L2;
    CombFilter<COMB_TUNING_L3> comb_L3;
    CombFilter<COMB_TUNING_L4> comb_L4;
    CombFilter<COMB_TUNING_L5> comb_L5;
    CombFilter<COMB_TUNING_L6> comb_L6;
    CombFilter<COMB_TUNING_L7> comb_L7;
    CombFilter<COMB_TUNING_L8> comb_L8;

    CombFilter<COMB_TUNING_R1> comb_R1;
    CombFilter<COMB_TUNING_R2> comb_R2;
    CombFilter<COMB_TUNING_R3> comb_R3;
    CombFilter<COMB_TUNING_R4> comb_R4;
    CombFilter<COMB_TUNING_R5> comb_R5;
    CombFilter<COMB_TUNING_R6> comb_R6;
    CombFilter<COMB_TUNING_R7> comb_R7;
    CombFilter<COMB_TUNING_R8> comb_R8;

    // オールパスフィルタ (4本 × L/R)
    AllpassFilter<ALLPASS_TUNING_L1> allpass_L1;
    AllpassFilter<ALLPASS_TUNING_L2> allpass_L2;
    AllpassFilter<ALLPASS_TUNING_L3> allpass_L3;
    AllpassFilter<ALLPASS_TUNING_L4> allpass_L4;

    AllpassFilter<ALLPASS_TUNING_R1> allpass_R1;
    AllpassFilter<ALLPASS_TUNING_R2> allpass_R2;
    AllpassFilter<ALLPASS_TUNING_R3> allpass_R3;
    AllpassFilter<ALLPASS_TUNING_R4> allpass_R4;

    // パラメータ
    uint8_t room_size = 50;    // ルームサイズ (0-99)
    uint8_t damping = 50;      // ダンピング (0-99)
    Gain_t mix = 8192;         // ウェットミックス (Q15, default 25%)

    // 派生パラメータ (Q15)
    int16_t feedback_q15 = 0;  // roomSize → フィードバック係数
    int16_t damp1_q15 = 0;     // damping → ローパス係数
    int16_t damp2_q15 = 0;     // 1 - damp1

    /** @brief パラメータから内部係数を再計算 */
    void updateCoefficients();

public:
    void reset();

    void setRoomSize(uint8_t size);
    void setDamping(uint8_t damp);
    void setMix(Gain_t mix);

    /** @brief L/Rを同時に処理 */
    void process(Sample16_t& left, Sample16_t& right);

    // パラメータ取得
    uint8_t getRoomSize() const { return room_size; }
    uint8_t getDamping() const { return damping; }
    Gain_t getMix() const { return mix; }
};
