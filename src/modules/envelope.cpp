#include "modules/envelope.hpp"

static_assert((Envelope::EXP_TABLE_SIZE & (Envelope::EXP_TABLE_SIZE - 1)) == 0, "EXP_TABLE_SIZE must be power of 2");

/** @brief エンベロープを初期位置に戻す */
void Envelope::reset(Memory& mem) {
    mem.state = EnvelopeState::Attack;
    mem.log_level = MAX_ATTENUATION;
    mem.current_level = 0;
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
            // 減衰量を減らす（音量を増加させる）
            // rateが最大値だと瞬時に音量が最大になる
            if (mem.log_level > attack_rate) {
                mem.log_level -= attack_rate;
            }
            // アタックが終了したら減衰量を0にしてDecayへ
            else {
                mem.log_level = 0;
                mem.state = EnvelopeState::Decay;
            }
            break;

        case EnvelopeState::Decay:
            // 減衰量を増やす（音量を減少させる）
            // rateが最大値だとすぐにsustain_log_levelになる
            if (mem.log_level + decay_rate < sustain_log_level) {
                mem.log_level += decay_rate;
            }
            // 減衰量が最大
            else {
                mem.log_level = sustain_log_level;
                mem.state = EnvelopeState::Sustain;
            }
            break;

        case EnvelopeState::Sustain:
            // sustain_log_levelが最大値だとフルボリュームで維持
            // 途中でsustainが変更された場合の処理
            if(mem.log_level != sustain_log_level) {
                if(mem.log_level < sustain_log_level) mem.log_level += decay_rate;
                else mem.log_level = sustain_log_level;
            }
            break;

        case EnvelopeState::Release:
            // 減衰量が「最大減衰量 - 変化量」より小さいなら、減衰量を増やす。
            // rateが最大値なら瞬時に音が消える
            if (mem.log_level + release_rate < MAX_ATTENUATION) {
                mem.log_level += release_rate;
            // 最大減衰量に達したら終了
            } else {
                mem.log_level = MAX_ATTENUATION;
                mem.state = EnvelopeState::Idle;
            }
            break;

        case EnvelopeState::Idle:
            mem.log_level = MAX_ATTENUATION;
            break;
    }

    // 音量レベルを更新
    uint32_t index = (mem.log_level >> FIXED_POINT_SHIFT) & EXP_TABLE_MASK;
    mem.current_level = exp_table[index];
}

FASTRUN void Envelope::processBlock(Memory& mem, int16_t* output_buffer, size_t num_samples) {
    for (size_t i = 0; i < num_samples; ++i) {
        update(mem);
        output_buffer[i] = mem.current_level;
    }
}

/**
 * @brief アタックを設定
 *
 * @param rate_0_99 0 - 99
 */
void Envelope::setAttack(uint8_t rate_0_99) {
    attack_rate = rate_table[clamp_param(rate_0_99)];
}

/**
 * @brief ディケイを設定
 *
 * @param rate_0_99 0 - 99
 */
void Envelope::setDecay(uint8_t rate_0_99) {
    decay_rate = rate_table[clamp_param(rate_0_99)];
}

/**
 * @brief リリースを設定
 *
 * @param rate_0_99 0 - 99
 */
void Envelope::setRelease(uint8_t rate_0_99) {
    release_rate = rate_table[clamp_param(rate_0_99)];
}

/**
 * @brief サステインを設定
 *
 * @param level_0_99 0 - 99
 */
void Envelope::setSustain(uint8_t level_0_99) {
    sustain_log_level = level_to_attenuation_table[clamp_param(level_0_99)];
}