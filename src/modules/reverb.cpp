#include "modules/reverb.hpp"
#include <algorithm>

/**
 * @brief 全バッファをクリア、係数を再計算
 */
void Reverb::reset() {
    comb_L1.clear(); comb_L2.clear(); comb_L3.clear(); comb_L4.clear();
    comb_L5.clear(); comb_L6.clear(); comb_L7.clear(); comb_L8.clear();
    comb_R1.clear(); comb_R2.clear(); comb_R3.clear(); comb_R4.clear();
    comb_R5.clear(); comb_R6.clear(); comb_R7.clear(); comb_R8.clear();

    allpass_L1.clear(); allpass_L2.clear(); allpass_L3.clear(); allpass_L4.clear();
    allpass_R1.clear(); allpass_R2.clear(); allpass_R3.clear(); allpass_R4.clear();

    updateCoefficients();
}

/**
 * @brief ルームサイズを設定
 * @param size RoomSize (0-99)
 */
void Reverb::setRoomSize(uint8_t size) {
    room_size = std::clamp<uint8_t>(size, REVERB_ROOM_MIN, REVERB_ROOM_MAX);
    updateCoefficients();
}

/**
 * @brief ダンピングを設定
 * @param damp Damping (0-99)
 */
void Reverb::setDamping(uint8_t damp) {
    damping = std::clamp<uint8_t>(damp, REVERB_DAMP_MIN, REVERB_DAMP_MAX);
    updateCoefficients();
}

/**
 * @brief ウェットミックスを設定
 * @param m Mix (Q15: 0-32767)
 */
void Reverb::setMix(Gain_t m) {
    mix = std::clamp<Gain_t>(m, 0, Q15_MAX);
}

/**
 * @brief パラメータから内部係数を再計算
 *
 * Freeverb の係数マッピング:
 * - feedback = roomoffset + roomscale * roomsize
 *   roomoffset = 0.7, roomscale = 0.28 → range 0.7 ~ 0.98
 * - damp1 = damping * dampscale
 *   dampscale = 0.4 → range 0 ~ 0.4
 * - damp2 = 1 - damp1
 *
 * すべてQ15に変換して整数演算で処理
 */
void Reverb::updateCoefficients() {
    // feedback: room_size 0-99 → 0.7 ~ 0.98
    // Q15: 22937 ~ 32112
    // feedback = 22937 + (room_size * 93)  (93 ≈ (32112-22937)/99)
    feedback_q15 = static_cast<int16_t>(22937 + (static_cast<int32_t>(room_size) * 93));
    if (feedback_q15 > 32500) feedback_q15 = 32500; // 安全マージン

    // damp1: damping 0-99 → 0.0 ~ 0.4
    // Q15: 0 ~ 13107
    // damp1 = damping * 132  (132 ≈ 13107/99)
    damp1_q15 = static_cast<int16_t>(static_cast<int32_t>(damping) * 132);
    if (damp1_q15 > 13107) damp1_q15 = 13107;

    // damp2 = 1.0 - damp1 (Q15)
    damp2_q15 = static_cast<int16_t>(Q15_MAX - damp1_q15);
}

/**
 * @brief リバーブ処理 (L/R同時)
 *
 * 1. 入力をモノラルミックスし、ゲインを下げる (飽和防止)
 * 2. 8つの並列コムフィルタで残響を生成
 * 3. 4つの直列オールパスフィルタで拡散
 * 4. ドライ + ウェット×mix で出力
 *
 * @param left  L入力/出力
 * @param right R入力/出力
 */
void Reverb::process(Sample16_t& left, Sample16_t& right) {
    // --- 入力準備: モノラルミックス、ゲインを下げて飽和防止 ---
    // input = (L + R) >> 4  (÷16)
    // コム合算後の >>3 (÷8) と合わせて合計÷128
    // room_size 0-65 ではコム内部バッファがクリップせず、
    // 66以上では緩やかなサチュレーションが発生（アナログ的な質感）
    int32_t input = (static_cast<int32_t>(left) + static_cast<int32_t>(right)) >> 4;
    Sample16_t input_s = static_cast<Sample16_t>(std::clamp<int32_t>(input, -32767, 32767));

    // --- 並列コムフィルタ (L/R各8本) ---
    int32_t out_L = 0;
    out_L += comb_L1.process(input_s, feedback_q15, damp1_q15, damp2_q15);
    out_L += comb_L2.process(input_s, feedback_q15, damp1_q15, damp2_q15);
    out_L += comb_L3.process(input_s, feedback_q15, damp1_q15, damp2_q15);
    out_L += comb_L4.process(input_s, feedback_q15, damp1_q15, damp2_q15);
    out_L += comb_L5.process(input_s, feedback_q15, damp1_q15, damp2_q15);
    out_L += comb_L6.process(input_s, feedback_q15, damp1_q15, damp2_q15);
    out_L += comb_L7.process(input_s, feedback_q15, damp1_q15, damp2_q15);
    out_L += comb_L8.process(input_s, feedback_q15, damp1_q15, damp2_q15);

    int32_t out_R = 0;
    out_R += comb_R1.process(input_s, feedback_q15, damp1_q15, damp2_q15);
    out_R += comb_R2.process(input_s, feedback_q15, damp1_q15, damp2_q15);
    out_R += comb_R3.process(input_s, feedback_q15, damp1_q15, damp2_q15);
    out_R += comb_R4.process(input_s, feedback_q15, damp1_q15, damp2_q15);
    out_R += comb_R5.process(input_s, feedback_q15, damp1_q15, damp2_q15);
    out_R += comb_R6.process(input_s, feedback_q15, damp1_q15, damp2_q15);
    out_R += comb_R7.process(input_s, feedback_q15, damp1_q15, damp2_q15);
    out_R += comb_R8.process(input_s, feedback_q15, damp1_q15, damp2_q15);

    // --- 直列オールパスフィルタ (L/R各4本) ---
    // 8本のコム合算を÷8して16bit範囲に収める
    Sample16_t wet_L = static_cast<Sample16_t>(std::clamp<int32_t>(out_L >> 3, -32767, 32767));
    wet_L = allpass_L1.process(wet_L);
    wet_L = allpass_L2.process(wet_L);
    wet_L = allpass_L3.process(wet_L);
    wet_L = allpass_L4.process(wet_L);

    Sample16_t wet_R = static_cast<Sample16_t>(std::clamp<int32_t>(out_R >> 3, -32767, 32767));
    wet_R = allpass_R1.process(wet_R);
    wet_R = allpass_R2.process(wet_R);
    wet_R = allpass_R3.process(wet_R);
    wet_R = allpass_R4.process(wet_R);

    // --- ドライ/ウェットミックス ---
    // dry_gain = Q15_MAX - mix,  wet_gain = mix
    int16_t dry_gain = static_cast<int16_t>(Q15_MAX - mix);

    int32_t final_L = (static_cast<int32_t>(left) * dry_gain >> 15)
                    + (static_cast<int32_t>(wet_L) * mix >> 15);
    int32_t final_R = (static_cast<int32_t>(right) * dry_gain >> 15)
                    + (static_cast<int32_t>(wet_R) * mix >> 15);

    left  = static_cast<Sample16_t>(std::clamp<int32_t>(final_L, -32767, 32767));
    right = static_cast<Sample16_t>(std::clamp<int32_t>(final_R, -32767, 32767));
}
