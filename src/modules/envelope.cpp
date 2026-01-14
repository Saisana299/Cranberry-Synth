#include "modules/envelope.hpp"

static_assert((Envelope::EXP_TABLE_SIZE & (Envelope::EXP_TABLE_SIZE - 1)) == 0, "EXP_TABLE_SIZE must be power of 2");

/** @brief エンベロープを初期位置に戻す（ノートオン時） */
void Envelope::reset(Memory& mem) {
    mem.state = EnvelopeState::Phase1;

    // 新規ノートの場合（完全に無音 or Idle状態）は初期化
    if (mem.log_level >= MAX_ATTENUATION) {
        mem.log_level = MAX_ATTENUATION;
        mem.current_level = 0;
    }
    // リトリガーの場合は現在のlog_levelを維持し、そこからPhase1を再開
    // ただしcurrent_levelはlog_levelに合わせて更新
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
 */
FASTRUN void Envelope::update(Memory& mem) {

    switch(mem.state) {
        case EnvelopeState::Phase1:
            // Phase1: 現在レベル → L1（アタック）
            // 非線形（指数的）カーブ: 残り距離の一定割合を移動
            if (rate1 == MAX_ATTENUATION) {
                // 即座に到達
                mem.log_level = target_level1;
                mem.state = EnvelopeState::Phase2;
            } else if (mem.log_level > target_level1) {
                // 減衰量を減らす（音量を上げる）方向
                uint32_t distance = mem.log_level - target_level1;
                // rate1_paramに基づいてシフト量を決定
                // rate 0: shift=18 (非常に遅い), rate 96: shift=2 (非常に速い)
                uint32_t shift = 18 - (rate1_param / 6);
                uint32_t step = distance >> shift;
                if (step == 0) step = 1;  // 最低1を保証（収束を確実に）

                if (step >= distance) {
                    mem.log_level = target_level1;
                    mem.state = EnvelopeState::Phase2;
                } else {
                    mem.log_level -= step;
                }
            } else if (mem.log_level < target_level1) {
                // 減衰量を増やす（音量を下げる）方向
                uint32_t next = mem.log_level + rate1;
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

        case EnvelopeState::Phase2:
            // Phase2: L1 → L2（ディケイ1）
            if (rate2 == MAX_ATTENUATION) {
                mem.log_level = target_level2;
                mem.state = EnvelopeState::Phase3;
            } else if (mem.log_level < target_level2) {
                // 減衰量を増やす（音量を下げる）方向
                uint32_t next = mem.log_level + rate2;
                if (next >= target_level2) {
                    mem.log_level = target_level2;
                    mem.state = EnvelopeState::Phase3;
                } else {
                    mem.log_level = next;
                }
            } else if (mem.log_level > target_level2) {
                // 減衰量を減らす（音量を上げる）方向
                uint32_t step = (mem.log_level - target_level2 < rate2) ?
                                (mem.log_level - target_level2) : rate2;
                mem.log_level -= step;
                if (mem.log_level <= target_level2) {
                    mem.log_level = target_level2;
                    mem.state = EnvelopeState::Phase3;
                }
            } else {
                mem.state = EnvelopeState::Phase3;
            }
            break;

        case EnvelopeState::Phase3:
            // Phase3: L2 → L3（ディケイ2/サステイン）
            // ノートオフまでこのフェーズに留まる
            if (rate3 == MAX_ATTENUATION) {
                mem.log_level = target_level3;
                // Phase3はサステインなので、留まる
            } else if (mem.log_level < target_level3) {
                uint32_t next = mem.log_level + rate3;
                if (next >= target_level3) {
                    mem.log_level = target_level3;
                } else {
                    mem.log_level = next;
                }
            } else if (mem.log_level > target_level3) {
                uint32_t step = (mem.log_level - target_level3 < rate3) ?
                                (mem.log_level - target_level3) : rate3;
                mem.log_level -= step;
                if (mem.log_level <= target_level3) {
                    mem.log_level = target_level3;
                }
            }
            // L3に到達してもPhase3のまま（サステイン維持）
            break;

        case EnvelopeState::Phase4:
            // Phase4: 現在レベル → L4（リリース）
            if (rate4 == MAX_ATTENUATION) {
                mem.log_level = target_level4;
                mem.state = EnvelopeState::Idle;
            } else if (mem.log_level < target_level4) {
                uint32_t next = mem.log_level + rate4;
                if (next >= target_level4) {
                    mem.log_level = target_level4;
                    mem.state = EnvelopeState::Idle;
                } else {
                    mem.log_level = next;
                }
            } else if (mem.log_level > target_level4) {
                uint32_t step = (mem.log_level - target_level4 < rate4) ?
                                (mem.log_level - target_level4) : rate4;
                mem.log_level -= step;
                if (mem.log_level <= target_level4) {
                    mem.log_level = target_level4;
                    mem.state = EnvelopeState::Idle;
                }
            } else {
                mem.state = EnvelopeState::Idle;
            }
            break;

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
