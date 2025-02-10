#include "modules/envelope.hpp"
//todo 指数カーブ化を検討
/** @brief エンベロープを初期位置に戻す */
void Envelope::reset(Memory& mem) {
    mem.state = State::Attack;
    mem.elapsed = 0.0f;
    mem.current_level = 0.0f;
    mem.prev_level = 0.0f;
}

/** @brief エンベロープをリリースに移行 */
void Envelope::release(Memory& mem) {
    if(mem.state != State::Release) {
        mem.prev_level = mem.current_level;
        mem.elapsed = 0.0f;
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
                // AudioMath::lerp(mem.prev_level, 1.0f, static_cast<float>(mem.elapsed) / attack_samples); の展開
                float t = mem.elapsed * attack_inv;
                mem.current_level = mem.prev_level + t * (1.0f - mem.prev_level);
                break;
            }
            mem.prev_level = mem.current_level;
            mem.elapsed -= attack_samples;
            mem.state = State::Decay;
            [[fallthrough]];

        case State::Decay:
            if(mem.elapsed < decay_samples) {
                // AudioMath::lerp(mem.prev_level, sustain_level, static_cast<float>(mem.elapsed) / decay_samples); の展開
                float t = mem.elapsed * decay_inv;
                mem.current_level = mem.prev_level + t * (sustain_level - mem.prev_level);
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
                // AudioMath::lerp(mem.prev_level, 0.0f, static_cast<float>(mem.elapsed) / release_samples) の展開
                float t = mem.elapsed * release_inv;
                mem.current_level = mem.prev_level * (1.0f - t);
            }
            else {
                mem.current_level = 0.0f;
            }
            break;
    }
    ++mem.elapsed;
}

/**
 * @brief アタックを設定
 *
 * @param attack_ms 1.0ms - 10000.0ms
 */
void Envelope::setAttack(float attack_ms) {
    if(attack_ms < 0.001f || attack_ms > 10.0f) return;
    attack_samples = attack_ms * SAMPLE_RATE;
    attack_inv = 1.0f / static_cast<float>(attack_samples);
}

/**
 * @brief ディケイを設定
 *
 * @param decay_ms 1.0ms - 10000.0ms
 */
void Envelope::setDecay(float decay_ms) {
    if(decay_ms < 0.001f || decay_ms > 10.0f) return;
    decay_samples = decay_ms * SAMPLE_RATE;
    decay_inv = 1.0f / static_cast<float>(decay_samples);
}

/**
 * @brief リリースを設定
 *
 * @param release_ms 1.0ms - 10000.0ms
 */
void Envelope::setRelease(float release_ms) {
    if(release_ms < 0.001f || release_ms > 10.0f) return;
    release_samples = release_ms * SAMPLE_RATE;
    release_inv = 1.0f / static_cast<float>(release_samples);
}

/**
 * @brief サステインを設定
 *
 * @param sustain_level 0.0 - 1.0
 */
void Envelope::setSustain(float sustain_level) {
    if(sustain_level < 0.0f || sustain_level > 1.0f) return;
    this->sustain_level = sustain_level;
}