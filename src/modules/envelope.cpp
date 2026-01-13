#include "modules/envelope.hpp"

static_assert((Envelope::EXP_TABLE_SIZE & (Envelope::EXP_TABLE_SIZE - 1)) == 0, "EXP_TABLE_SIZE must be power of 2");

/** @brief エンベロープを初期位置に戻す */
void Envelope::reset(Memory& mem) {
    mem.state = EnvelopeState::Attack;

    // 完全に無音の場合のみ、初期値を設定
    if (mem.log_level >= MAX_ATTENUATION) {
        mem.log_level = MAX_ATTENUATION;
        mem.current_level = 0;
    }
    // それ以外は現在のlog_levelを維持し、そこからAttackを再開
}

/** @brief エンベロープをリリースに移行 */
void Envelope::release(Memory& mem) {
    if(mem.state != EnvelopeState::Release && mem.state != EnvelopeState::Idle) {
        mem.state = EnvelopeState::Release;
    }
}

/**
 * @brief エンベロープ状態を更新します
 *
 * @param adsr ADSRの設定
 */
FASTRUN void Envelope::update(Memory& mem) {
    switch(mem.state) {
        // ここでは減衰量で音量を変化させる
        case EnvelopeState::Attack:
            // Attack: 減衰量を小さくしていき、音量を上げる
            // rate が最大値の場合はアタックをスキップして即 Decay へ
            if (attack_rate == MAX_ATTENUATION) {
                mem.log_level = 0;
                mem.state = EnvelopeState::Decay;
            } else {
                // 現在のlog_levelから0に向かって減少
                // 高速リトリガー対応: log_levelがattack_rateより小さい場合も適切に処理
                if (mem.log_level > 0) {
                    // 現在のlog_levelとattack_rateの小さい方を引く
                    uint32_t step = (mem.log_level < attack_rate) ? mem.log_level : attack_rate;
                    mem.log_level -= step;
                    // 0に到達したらDecayへ
                    if (mem.log_level == 0) {
                        mem.state = EnvelopeState::Decay;
                    }
                } else {
                    mem.state = EnvelopeState::Decay;
                }
            }
            break;

        case EnvelopeState::Decay:
            // Decay: 減衰量を増やして音量を減らし、サスティン値へ向かう
            // rate が最大の場合はサスティン値に即移行
            if (decay_rate == MAX_ATTENUATION) {
                mem.log_level = sustain_log_level;
                mem.state = EnvelopeState::Sustain;
            } else {
                // オーバーフロー防止しつつ log_level を増やす
                uint32_t next = mem.log_level + decay_rate;
                if (next < mem.log_level) next = MAX_ATTENUATION;

                // サスティン値に達していなければ増加
                if (next < sustain_log_level) {
                    mem.log_level += decay_rate;
                }
                // 到達または超過したらサスティンへ移行
                else {
                    mem.log_level = sustain_log_level;
                    mem.state = EnvelopeState::Sustain;
                }
            }
            break;

        case EnvelopeState::Sustain:
            // Sustain: 指定された log_level を維持
            if (mem.log_level == sustain_log_level) {
                // 保持
            }

            // 現在値がサスティンより低い → 上方向へ追従
            else if (mem.log_level < sustain_log_level) {
                uint32_t next = mem.log_level + decay_rate;
                if (next < mem.log_level) next = sustain_log_level;
                mem.log_level = (next < sustain_log_level) ? next : sustain_log_level;
            }

            // 現在値がサスティンより高い → 下方向へ追従
            else {
                if (decay_rate == 0) {
                    mem.log_level = sustain_log_level;
                } else {
                    uint32_t step = decay_rate;
                    if (mem.log_level > step) {
                        uint32_t next = mem.log_level - step;
                        mem.log_level = (next > sustain_log_level) ? next : sustain_log_level;
                    } else {
                        mem.log_level = sustain_log_level;
                    }
                }
            }
            break;

        case EnvelopeState::Release:
            // Release: 減衰量を増やし、音量をゼロへ向かわせる
            // rateが最大値なら瞬時に音が消える
            if (release_rate == MAX_ATTENUATION) {
                mem.log_level = MAX_ATTENUATION;
                mem.state = EnvelopeState::Idle;
            } else {
                uint32_t next = mem.log_level + release_rate;
                if (next < mem.log_level) next = MAX_ATTENUATION;

                // まだ完全に減衰しきっていない
                if (next < MAX_ATTENUATION) {
                    mem.log_level = next;
                }

                // 無音に達したら Idle
                else {
                    mem.log_level = MAX_ATTENUATION;
                    mem.state = EnvelopeState::Idle;
                }
            }
            break;

        case EnvelopeState::Idle:
        default:
            mem.log_level = MAX_ATTENUATION;
            break;
    }

    // 音量レベルを更新
    uint32_t index = (mem.log_level >> FIXED_POINT_SHIFT) & EXP_TABLE_MASK;
    mem.current_level = exp_table[index];
}

/**
 * @brief アタックを設定
 *
 * @param rate_0_99 0 - 99
 */
void Envelope::setAttack(uint8_t rate_0_99) {
    attack_param = clamp_param(rate_0_99);
    attack_rate = rate_table[attack_param];
}

/**
 * @brief ディケイを設定
 *
 * @param rate_0_99 0 - 99
 */
void Envelope::setDecay(uint8_t rate_0_99) {
    decay_param = clamp_param(rate_0_99);
    decay_rate = rate_table[decay_param];
}

/**
 * @brief リリースを設定
 *
 * @param rate_0_99 0 - 99
 */
void Envelope::setRelease(uint8_t rate_0_99) {
    release_param = clamp_param(rate_0_99);
    release_rate = rate_table[release_param];
}

/**
 * @brief サステインを設定
 *
 * @param level_0_99 0 - 99
 */
void Envelope::setSustain(uint8_t level_0_99) {
    sustain_param = clamp_param(level_0_99);
    sustain_log_level = level_to_attenuation_table[sustain_param];
}