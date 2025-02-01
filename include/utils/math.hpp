#pragma once

#include <Arduino.h>

class AudioMath {
public:
    /**
     * @brief MIDIノートを周波数に変換
     *
     * @param note
     * @return float
     */
    static inline float noteToFrequency(uint8_t note) {
        return 440.0f * powf(2.0f, (note - 69) / 12.0f);
    }

    /**
     * @brief 比率で周波数を計算
     *
     * @param note MIDIノート番号
     * @param detune_cents デチューンするcent数
     * @param coarse 0.5ならオク下、2ならオク上と比率を設定
     * @param fine_level coarseの調整値 1.0 で coarseの2倍 coarseが2なら4になる
     * @return float
     */
    static inline float ratioToFrequency(uint8_t note, int8_t detune_cents, float coarse, float fine_level) {
        float pitch = coarse + (coarse * fine_level);
        float detune_factor = powf(2.0, detune_cents / 1200.0f);
        return noteToFrequency(note) * pitch * detune_factor;
    }

    static inline float fixedToFrequency(int8_t detune_cents, float coarse, float fine_level) {
        //todo
        return 0.0f;
    }

    /**
     * @brief ベロシティを音量レベルに変換
     *
     * @param velocity
     * @return float
     */
    static inline float velocityToAmplitude(uint8_t velocity) {
        return velocity / 127.0f;
    }

    /**
     * @brief 線形補間
     *
     * @param a
     * @param b
     * @param t
     * @return float
     */
    static inline float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    /**
     * @brief パディングサイズを取得
     *
     * @param size
     * @return uint8_t
     */
    static inline uint8_t bitPadding32(size_t size) {
        uint8_t shift = 0;
        while (size > 1) {
            size >>= 1;
            shift++;
        }
        return 32 - shift;
    }

    /**
     * @brief パン値から左チャンネルの係数を取得
     *
     * @param m_pan -1.0[Left] ~ 1.0[Right]
     * @return float
     */
    static inline float lChPanCoef(float m_pan) {
        float normalized_pan = (m_pan + 1.0f) * 0.5f;
        return static_cast<float>(cos(HALF_PI * normalized_pan));
    }

    /**
     * @brief パン値から右チャンネルの係数を取得
     *
     * @param m_pan -1.0[Left] ~ 1.0[Right]
     * @return float
     */
    static inline float rChPanCoef(float m_pan) {
        float normalized_pan = (m_pan + 1.0f) * 0.5f;
        return static_cast<float>(sin(HALF_PI * normalized_pan));
    }
};