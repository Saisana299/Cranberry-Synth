#include "modules/envelope.hpp"

/** @brief エンベロープを初期位置に戻す */
void Envelope::reset(Memory& mem) {
    mem.state = State::Attack;
    mem.log_level = (EXP_TABLE_SIZE - 1) << FIXED_POINT_SHIFT;
    mem.current_level = 0;
}

/** @brief エンベロープをリリースに移行 */
void Envelope::release(Memory& mem) {
    if(mem.state != State::Release) {
        mem.state = State::Release;
    }
}

/**
 * @brief エンベロープ状態を更新します
 *
 * @param adsr ADSRの設定
 */
void Envelope::update(Memory& mem) {
    switch(mem.state) {
        // ここでは減衰量で音量を変化させる
        case State::Attack:
            // 減衰量を減らす（音量を増加させる）
            // rateが最大値だと瞬時に音量が最大になる
            if (mem.log_level > attack_rate) {
                mem.log_level -= attack_rate;
            }
            // アタックが終了したら減衰量を0にしてDecayへ
            else {
                mem.log_level = 0;
                mem.state = State::Decay;
            }
            break;

        case State::Decay:
            // 減衰量を増やす（音量を減少させる）
            // rateが最大値だとすぐにsustain_log_levelになる
            if (mem.log_level < sustain_log_level) {
                mem.log_level += decay_rate;
            }
            // 減衰量が最大
            else {
                mem.log_level = sustain_log_level;
                mem.state = State::Sustain;
            }
            break;

        case State::Sustain:
            // sustain_log_levelが最大値だとフルボリュームで維持
            break;

        case State::Release:
            // 目標値（最大減衰量）を固定小数点スケールに合わせる
            const uint32_t max_attenuation = (EXP_TABLE_SIZE - 1) << FIXED_POINT_SHIFT;
            // 減衰量が「最大減衰量 - 変化量」より小さいなら、減衰量を増やす。
            // rateが最大値なら瞬時に音が消える
            if (mem.log_level < max_attenuation - release_rate) {
                mem.log_level += release_rate;
            // 最大減衰量に達したら終了
            } else {
                mem.log_level = max_attenuation;
            }
            break;
    }

    // 音量レベルを更新
    mem.current_level = exp_table[mem.log_level >> FIXED_POINT_SHIFT];
}

/**
 * @brief アタックを設定
 *
 * @param attack_ms 1ms - 10000ms
 */
void Envelope::setAttack(uint8_t attack_rate_0_99) {
    attack_rate = rate_table[attack_rate_0_99];
}

/**
 * @brief ディケイを設定
 *
 * @param decay_ms 1ms - 10000ms
 */
void Envelope::setDecay(uint8_t decay_rate_0_99) {
    decay_rate = rate_table[decay_rate_0_99];
}

/**
 * @brief リリースを設定
 *
 * @param release_ms 1ms - 10000ms
 */
void Envelope::setRelease(uint8_t release_rate_0_99) {
    release_rate = rate_table[release_rate_0_99];
}

/**
 * @brief サステインを設定
 *
 * @param sustain_level 0 - 1024 (1.0)
 */
void Envelope::setSustain(int16_t sustain_level_0_1023) {
    // 線形レベルから対数レベルへ簡易変換
    if(sustain_level_0_1023 < 0) sustain_level_0_1023 = 0;
    if(sustain_level_0_1023 > 1023) sustain_level_0_1023 = 1023;

    // 目的の線形レベルに最も近い対数レベルを探す (簡易的な逆引き)
    // 本来はこれもテーブル化するか、より効率的な探索を行う
    int32_t min_diff = 1024;
    for (uint32_t i = 0; i < EXP_TABLE_SIZE; ++i) {
        int32_t diff = std::abs(static_cast<int32_t>(sustain_level_0_1023) - exp_table[i]);
        if(diff < min_diff) {
            min_diff = diff;
            sustain_log_level = i << FIXED_POINT_SHIFT;
        }
    }
}