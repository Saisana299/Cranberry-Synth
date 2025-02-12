#include "modules/envelope.hpp"
//todo エンベロープの指数カーブ化を検討
// 案外適当な方が面白いかもしれない
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
                const int32_t diff = 1024 - static_cast<int32_t>(mem.prev_level);
                const int32_t offset = (diff * mem.elapsed * attack_inv) >> 16;
                mem.current_level = static_cast<int16_t>(mem.prev_level + offset);
                break;
            }
            mem.prev_level = mem.current_level;
            mem.elapsed -= attack_samples;
            mem.state = State::Decay;
            [[fallthrough]];

        case State::Decay:
            if(mem.elapsed < decay_samples) {
                if (mem.current_level > sustain_level) {
                    const int32_t diff = static_cast<int32_t>(mem.prev_level) - static_cast<int32_t>(sustain_level);
                    const int32_t offset = (diff * mem.elapsed * decay_inv) >> 16;
                    mem.current_level = static_cast<int16_t>(mem.prev_level - offset);
                    break;
                }
                // サステインと同じ音量になったらSustainへ移行する
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
                const int32_t diff = 0 - static_cast<int32_t>(mem.prev_level);
                const int32_t absDiff = (diff ^ (diff >> 31)) - (diff >> 31); // = -diff
                const int32_t offset = (absDiff * mem.elapsed * release_inv) >> 16;
                mem.current_level = static_cast<int16_t>(mem.prev_level - offset);
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
    attack_inv = (attack_samples != 0) ? (((uint32_t)1 << 16) + (attack_samples >> 1)) / attack_samples : 0;
}

/**
 * @brief ディケイを設定
 *
 * @param decay_ms 1ms - 10000ms
 */
void Envelope::setDecay(int16_t decay_ms) {
    if(decay_ms < 1 || decay_ms > 10000) return;
    decay_samples = (static_cast<uint32_t>(decay_ms * SAMPLE_RATE)) >> 10;
    decay_inv = (decay_samples != 0) ? (((uint32_t)1 << 16) + (decay_samples >> 1)) / decay_samples : 0;
}

/**
 * @brief リリースを設定
 *
 * @param release_ms 1ms - 10000ms
 */
void Envelope::setRelease(int16_t release_ms) {
    if(release_ms < 1 || release_ms > 10000) return;
    release_samples = (static_cast<uint32_t>(release_ms * SAMPLE_RATE)) >> 10;
    release_inv = (release_samples != 0) ? (((uint32_t)1 << 16) + (release_samples >> 1)) / release_samples : 0;
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