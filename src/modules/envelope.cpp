#include "modules/envelope.hpp"
//todo 指数カーブ化を検討
/** @brief エンベロープを初期位置に戻す */
void Envelope::reset(Memory& mem) {
    mem.state = State::Attack;
    mem.elapsed = 0;
    mem.current_level = 0;
    mem.prev_level = 0;
}

/** @brief エンベロープをリリースに移行 */
void Envelope::release(Memory& mem) {
    if(mem.state != State::Release) {
        mem.prev_level = mem.current_level;
        mem.elapsed = 0;
        mem.state = State::Release;
    }
}

/**
 * @brief エンベロープ状態を更新します
 *
 * @param adsr ADSRの設定
 * @param dt 加算するサンプル数
 */
void Envelope::update(Memory& mem) {
    switch(mem.state) {
        case State::Attack:
            if(mem.elapsed < attack_samples) {
                mem.current_level = AudioMath::lerp(static_cast<float>(mem.prev_level), 1024.0f, static_cast<float>(mem.elapsed) / attack_samples);
                break;
            }
            mem.prev_level = mem.current_level;
            mem.elapsed -= attack_samples;
            mem.state = State::Decay;
            [[fallthrough]];

        case State::Decay:
            if(mem.elapsed < decay_samples) {
                mem.current_level = AudioMath::lerp(static_cast<float>(mem.prev_level), static_cast<float>(sustain_level), static_cast<float>(mem.elapsed) / decay_samples);
                break;
            }
            mem.prev_level = mem.current_level;
            mem.elapsed -= decay_samples;
            mem.state = State::Sustain;
            [[fallthrough]];

        case State::Sustain:
            mem.current_level = sustain_level;
            break;

        case State::Release:
            if(mem.elapsed < release_samples) {
                mem.current_level = AudioMath::lerp(static_cast<float>(mem.prev_level), 0.0f, static_cast<float>(mem.elapsed) / release_samples);
            }
            else {
                mem.current_level = 0;
            }
            break;
    }
    ++mem.elapsed;
}

/**
 * @brief アタックを設定
 *
 * @param attack_ms 1ms - 10000ms
 */
void Envelope::setAttack(int16_t attack_ms) {
    if(attack_ms < 1 || attack_ms > 10000) return;
    attack_samples = (static_cast<uint32_t>(attack_ms * SAMPLE_RATE)) >> 10;
}

/**
 * @brief ディケイを設定
 *
 * @param decay_ms 1ms - 10000ms
 */
void Envelope::setDecay(int16_t decay_ms) {
    if(decay_ms < 1 || decay_ms > 10000) return;
    decay_samples = (static_cast<uint32_t>(decay_ms * SAMPLE_RATE)) >> 10;
}

/**
 * @brief リリースを設定
 *
 * @param release_ms 1ms - 10000ms
 */
void Envelope::setRelease(int16_t release_ms) {
    if(release_ms < 1 || release_ms > 10000) return;
    release_samples = (static_cast<uint32_t>(release_ms * SAMPLE_RATE)) >> 10;
}

/**
 * @brief サステインを設定
 *
 * @param sustain_level 0 - 1024 (1.0)
 */
void Envelope::setSustain(int16_t sustain_level) {
    if(sustain_level < 0 || sustain_level > 1024) return;
    this->sustain_level = sustain_level;
}