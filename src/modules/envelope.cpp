#include "modules/envelope.h"

/** @brief エンベロープを初期位置に戻す */
void Envelope::reset() {
    state = EnvState::Attack;
    elapsed = 0;
    current_level = 0.0f;
    prev_level = 0.0f;
}

/** @brief エンベロープをリリースに移行 */
void Envelope::release() {
    if(state != EnvState::Release) {
        prev_level = current_level;
        elapsed = 0;
        state = EnvState::Release;
    }
}

/**
 * @brief エンベロープ状態を更新します
 *
 * @param adsr ADSRの設定
 * @param dt 加算するサンプル数
 */
void Envelope::update(const ADSRConfig& adsr, uint32_t dt) {
    switch(state) {
        case EnvState::Attack:
            if(elapsed < adsr.attack_samples) {
                current_level = AudioMath::lerp(prev_level, 1.0f, static_cast<float>(elapsed) / adsr.attack_samples);
                break;
            }
            prev_level = current_level;
            elapsed -= adsr.attack_samples;
            state = EnvState::Decay;
            [[fallthrough]];

        case EnvState::Decay:
            if(elapsed < adsr.decay_samples) {
                current_level = AudioMath::lerp(prev_level, adsr.sustain_level, static_cast<float>(elapsed) / adsr.decay_samples);
                break;
            }
            prev_level = current_level;
            elapsed -= adsr.decay_samples;
            state = EnvState::Sustain;
            [[fallthrough]];

        case EnvState::Sustain:
            current_level = adsr.sustain_level;
            break;

        case EnvState::Release:
            current_level = elapsed < adsr.release_samples
                ? AudioMath::lerp(prev_level, 0.0f, static_cast<float>(elapsed) / adsr.release_samples)
                : 0.0f;
            break;
    }
    elapsed += dt;
}

/**
 * @brief 現在のレベルを返します
 *
 * @return float Min: 0.0, Max: 1.0
 */
float Envelope::currentLevel() {
    return current_level;
}

/**
 * @brief エンベロープ終了判定
 *
 * @return 終了していれば `true` を返す
 */
bool Envelope::isFinished() {
    return (state == EnvState::Release && current_level == 0.0f);
}