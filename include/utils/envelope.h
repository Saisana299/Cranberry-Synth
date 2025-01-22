#ifndef ENVELOPE_H
#define ENVELOPE_H

#include "utils/math.h"

struct ADSRConfig {
    uint32_t attack_samples;
    uint32_t decay_samples;
    float    sustain_level;
    uint32_t release_samples;
};

class Envelope {
public:
    enum class EnvState {
        Attack, Decay, Sustain, Release
    };

    void reset() {
        state = EnvState::Attack;
        elapsed = 0;
        current_level = 0;
        prev_level = 0;
    }

    void release() {
        if(state != EnvState::Release) {
            prev_level = current_level;
            elapsed = 0;
            state = EnvState::Release;
        }
    }

    void update(const ADSRConfig& adsr, uint32_t dt) {
        switch(state) {
            case EnvState::Attack:
                if(elapsed < adsr.attack_samples) {
                    current_level = AudioMath::lerp(prev_level, 1.0, elapsed / adsr.attack_samples);
                    break;
                }
                prev_level = current_level;
                elapsed -= adsr.attack_samples;
                state = EnvState::Decay;
                [[fallthrough]];

            case EnvState::Decay:
                if(elapsed < adsr.decay_samples) {
                    current_level = AudioMath::lerp(prev_level, adsr.sustain_level, elapsed / adsr.decay_samples);
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
                    ? AudioMath::lerp(prev_level, 0.0, elapsed / adsr.release_samples)
                    : 0.0;
                break;
        }
        elapsed += dt;
    }

    float currentLevel() {
        return current_level;
    }

    bool isFinished() {
        if(state == EnvState::Release && current_level == 0.0) {
            return true;
        }
        return false;
    }

private:
    EnvState state = EnvState::Attack;
    uint32_t elapsed = 0;
    float current_level = 0;
    float prev_level = 0;
};

#endif