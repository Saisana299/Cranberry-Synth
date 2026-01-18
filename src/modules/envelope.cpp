#include "modules/envelope.hpp"

static_assert((Envelope::EXP_TABLE_SIZE & (Envelope::EXP_TABLE_SIZE - 1)) == 0, "EXP_TABLE_SIZE must be power of 2");

/** @brief エンベロープを初期位置に戻す（ノートオン時） */
void Envelope::reset(Memory& mem) {
    // 新規ノートの場合（完全に無音 or Idle状態）は初期化
    if (mem.log_level >= MAX_ATTENUATION || mem.state == EnvelopeState::Idle) {
        mem.log_level = MAX_ATTENUATION;
        mem.current_level = 0;
        mem.state = EnvelopeState::Phase1;
    } else {
        // リトリガーの場合：現在のlog_levelを維持
        // 現在の音量がtarget_level1より高い（減衰量が小さい）場合は
        // Phase1をスキップしてPhase2から開始（音量が下がるのを防止）
        if (mem.log_level <= target_level1) {
            // 現在の音量 >= level1 → Phase2から開始
            mem.state = EnvelopeState::Phase2;
        } else {
            // 現在の音量 < level1 → Phase1から開始（通常のアタック）
            mem.state = EnvelopeState::Phase1;
        }
    }
    // current_levelをlog_levelに合わせて更新
    uint32_t index = (mem.log_level >> FIXED_POINT_SHIFT) & EXP_TABLE_MASK;
    mem.current_level = exp_table[index];
}

/** @brief エンベロープをリリースフェーズに移行（ノートオフ時） */
void Envelope::release(Memory& mem) {
    if(mem.state != EnvelopeState::Phase4 && mem.state != EnvelopeState::Idle) {
        mem.state = EnvelopeState::Phase4;
    }
}

/** @brief エンベロープを完全にIdle状態にリセット（ノートスロット解放時） */
void Envelope::clear(Memory& mem) {
    mem.state = EnvelopeState::Idle;
    mem.log_level = MAX_ATTENUATION;
    mem.current_level = 0;
}

/**
 * @brief エンベロープ状態を更新します
 *
 * Phase1: 現在レベル → L1 (Rate1)
 * Phase2: L1 → L2 (Rate2)
 * Phase3: L2 → L3 (Rate3) - サステイン
 * Phase4: 現在レベル → L4 (Rate4) - リリース
 *
 * Rate Scaling: 高いノートほどエンベロープが速くなる
 * rate_scaling_deltaをrate_paramに加算して効果を適用
 */
FASTRUN void Envelope::update(Memory& mem) {
    // Rate Scalingを適用した実効レート計算用ヘルパーラムダ
    // rate_paramにdeltaを加算し、0-99にクランプしてテーブル参照
    const int8_t rs_delta = mem.rate_scaling_delta;

    // 各フェーズ用の実効レートを事前計算
    auto getScaledRate = [rs_delta](uint8_t rate_param) -> uint32_t {
        int16_t scaled = static_cast<int16_t>(rate_param) + rs_delta;
        if (scaled < 0) scaled = 0;
        if (scaled > 99) scaled = 99;
        return rate_table[scaled];
    };

    switch(mem.state) {
        case EnvelopeState::Phase1: {
            // Phase1: 現在レベル → L1（アタック）
            // 対数カーブ方式: 現在のlog_levelに基づくステップ計算
            // これによりtarget_level1に関係なく一定速度でアタックが進む
            if (mem.log_level > target_level1) {
                // 減衰量を減らす（音量を上げる）方向
                uint32_t distance = mem.log_level - target_level1;
                // rate1_param + rate_scalingに基づいてシフト量を決定
                // rate 0: shift=18 (非常に遅い), rate 96: shift=2 (非常に速い)
                int16_t scaled_rate = static_cast<int16_t>(rate1_param) + rs_delta;
                if (scaled_rate < 0) scaled_rate = 0;
                if (scaled_rate > 99) scaled_rate = 99;
                uint32_t shift = 18 - (scaled_rate / 6);
                // 対数カーブ方式: distance → mem.log_level
                uint32_t step = mem.log_level >> shift;
                if (step == 0) step = 1;  // 最低1を保証（収束を確実に）

                if (step >= distance) {
                    mem.log_level = target_level1;
                    mem.state = EnvelopeState::Phase2;
                } else {
                    mem.log_level -= step;
                }
            } else if (mem.log_level < target_level1) {
                // 減衰量を増やす（音量を下げる）方向
                uint32_t scaled_rate1 = getScaledRate(rate1_param);
                uint32_t next = mem.log_level + scaled_rate1;
                if (next >= target_level1) {
                    mem.log_level = target_level1;
                    mem.state = EnvelopeState::Phase2;
                } else {
                    mem.log_level = next;
                }
            } else {
                mem.state = EnvelopeState::Phase2;
            }
            break;
        }

        case EnvelopeState::Phase2: {
            // Phase2: L1 → L2（ディケイ1）
            uint32_t scaled_rate2 = getScaledRate(rate2_param);
            if (mem.log_level < target_level2) {
                // 減衰量を増やす（音量を下げる）方向
                uint32_t next = mem.log_level + scaled_rate2;
                if (next >= target_level2) {
                    mem.log_level = target_level2;
                    mem.state = EnvelopeState::Phase3;
                } else {
                    mem.log_level = next;
                }
            } else if (mem.log_level > target_level2) {
                // 減衰量を減らす（音量を上げる）方向
                uint32_t step = (mem.log_level - target_level2 < scaled_rate2) ?
                                (mem.log_level - target_level2) : scaled_rate2;
                mem.log_level -= step;
                if (mem.log_level <= target_level2) {
                    mem.log_level = target_level2;
                    mem.state = EnvelopeState::Phase3;
                }
            } else {
                mem.state = EnvelopeState::Phase3;
            }
            break;
        }

        case EnvelopeState::Phase3: {
            // Phase3: L2 → L3（ディケイ2/サステイン）
            // ノートオフまでこのフェーズに留まる
            uint32_t scaled_rate3 = getScaledRate(rate3_param);
            if (mem.log_level < target_level3) {
                uint32_t next = mem.log_level + scaled_rate3;
                if (next >= target_level3) {
                    mem.log_level = target_level3;
                } else {
                    mem.log_level = next;
                }
            } else if (mem.log_level > target_level3) {
                uint32_t step = (mem.log_level - target_level3 < scaled_rate3) ?
                                (mem.log_level - target_level3) : scaled_rate3;
                mem.log_level -= step;
                if (mem.log_level <= target_level3) {
                    mem.log_level = target_level3;
                }
            }
            // L3に到達してもPhase3のまま（サステイン維持）
            break;
        }

        case EnvelopeState::Phase4: {
            // Phase4: 現在レベル → L4（リリース）
            uint32_t scaled_rate4 = getScaledRate(rate4_param);
            if (mem.log_level < target_level4) {
                uint32_t next = mem.log_level + scaled_rate4;
                if (next >= target_level4) {
                    mem.log_level = target_level4;
                    mem.state = EnvelopeState::Idle;
                } else {
                    mem.log_level = next;
                }
            } else if (mem.log_level > target_level4) {
                uint32_t step = (mem.log_level - target_level4 < scaled_rate4) ?
                                (mem.log_level - target_level4) : scaled_rate4;
                mem.log_level -= step;
                if (mem.log_level <= target_level4) {
                    mem.log_level = target_level4;
                    mem.state = EnvelopeState::Idle;
                }
            } else {
                mem.state = EnvelopeState::Idle;
            }
            break;
        }

        case EnvelopeState::Idle:
        default:
            mem.log_level = target_level4;
            break;
    }

    // 音量レベルを更新
    uint32_t index = (mem.log_level >> FIXED_POINT_SHIFT) & EXP_TABLE_MASK;
    mem.current_level = exp_table[index];
}

// ===== Rate設定 =====

/**
 * @brief Rate1（アタックレート）を設定
 * @param rate_0_99 0=最も遅い, 99=即座に到達
 */
void Envelope::setRate1(uint8_t rate_0_99) {
    rate1_param = clamp_param(rate_0_99);
    rate1 = rate_table[rate1_param];
}

/**
 * @brief Rate2（ディケイ1レート）を設定
 * @param rate_0_99 0=最も遅い, 99=即座に到達
 */
void Envelope::setRate2(uint8_t rate_0_99) {
    rate2_param = clamp_param(rate_0_99);
    rate2 = rate_table[rate2_param];
}

/**
 * @brief Rate3（ディケイ2/サステインレート）を設定
 * @param rate_0_99 0=最も遅い, 99=即座に到達
 */
void Envelope::setRate3(uint8_t rate_0_99) {
    rate3_param = clamp_param(rate_0_99);
    rate3 = rate_table[rate3_param];
}

/**
 * @brief Rate4（リリースレート）を設定
 * @param rate_0_99 0=最も遅い, 99=即座に到達
 */
void Envelope::setRate4(uint8_t rate_0_99) {
    rate4_param = clamp_param(rate_0_99);
    rate4 = rate_table[rate4_param];
}

// ===== Level設定 =====

/**
 * @brief Level1（アタック到達レベル）を設定
 * @param level_0_99 0=無音, 99=最大音量
 */
void Envelope::setLevel1(uint8_t level_0_99) {
    level1_param = clamp_param(level_0_99);
    target_level1 = level_to_attenuation_table[level1_param];
}

/**
 * @brief Level2（ディケイ1到達レベル）を設定
 * @param level_0_99 0=無音, 99=最大音量
 */
void Envelope::setLevel2(uint8_t level_0_99) {
    level2_param = clamp_param(level_0_99);
    target_level2 = level_to_attenuation_table[level2_param];
}

/**
 * @brief Level3（サステインレベル）を設定
 * @param level_0_99 0=無音, 99=最大音量
 */
void Envelope::setLevel3(uint8_t level_0_99) {
    level3_param = clamp_param(level_0_99);
    target_level3 = level_to_attenuation_table[level3_param];
}

/**
 * @brief Level4（リリース到達レベル）を設定
 * @param level_0_99 0=無音, 99=最大音量
 */
void Envelope::setLevel4(uint8_t level_0_99) {
    level4_param = clamp_param(level_0_99);
    target_level4 = level_to_attenuation_table[level4_param];
}

// ===== Rate Scaling =====

/**
 * @brief Rate Scaling sensitivity を設定
 * @param sensitivity 0-7 (0=効果なし, 7=最大効果)
 */
void Envelope::setRateScaling(uint8_t sensitivity) {
    rate_scaling_param = (sensitivity > 7) ? 7 : sensitivity;
}

/**
 * @brief Rate Scaling計算
 *
 * @param midinote MIDIノート番号 (0-127)
 * @param sensitivity Rate Scaling sensitivity (0-7)
 * @return int8_t rate増分 (0-27)
 */
int8_t Envelope::calcRateScalingDelta(uint8_t midinote, uint8_t sensitivity) {
    if (sensitivity == 0) return 0;

    // x = clamp(0, 31, midinote / 3 - 7)
    int16_t x = (midinote / 3) - 7;
    if (x < 0) x = 0;
    if (x > 31) x = 31;

    // qratedelta = (sensitivity * x) >> 3
    int8_t delta = static_cast<int8_t>((sensitivity * x) >> 3);
    return delta;
}

/**
 * @brief ノートごとのRate Scaling増分を設定
 *
 * このメソッドはノートオン時に呼び出し、ノート番号に応じた
 * rate scaling増分をMemoryに設定する
 *
 * @param mem エンベロープメモリ
 * @param midinote MIDIノート番号
 */
void Envelope::applyRateScaling(Memory& mem, uint8_t midinote) {
    mem.rate_scaling_delta = calcRateScalingDelta(midinote, rate_scaling_param);
}
