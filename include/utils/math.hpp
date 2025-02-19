#pragma once

#include <Arduino.h>

class AudioMath {
public:
    // 0 = L100%, 100 = C, 200 = R100%
    static inline const int16_t PAN_SIN_TABLE[201] = {
            0,   257,   515,   772,  1029,  1286,  1544,  1801,  2057,  2314,
        2571,  2827,  3084,  3340,  3596,  3851,  4107,  4362,  4617,  4872,
        5126,  5380,  5634,  5887,  6140,  6393,  6645,  6897,  7148,  7399,
        7649,  7899,  8149,  8398,  8646,  8894,  9142,  9389,  9635,  9880,
        10126, 10370, 10614, 10857, 11099, 11341, 11582, 11823, 12062, 12301,
        12539, 12777, 13013, 13249, 13484, 13718, 13952, 14184, 14415, 14646,
        14876, 15105, 15333, 15560, 15786, 16011, 16235, 16458, 16680, 16901,
        17121, 17340, 17557, 17774, 17990, 18204, 18418, 18630, 18841, 19051,
        19260, 19468, 19674, 19879, 20083, 20286, 20487, 20688, 20886, 21084,
        21280, 21475, 21669, 21862, 22053, 22242, 22431, 22617, 22803, 22987,
        23170, 23351, 23531, 23709, 23886, 24062, 24235, 24408, 24579, 24748,
        24916, 25083, 25247, 25411, 25572, 25732, 25891, 26048, 26203, 26357,
        26509, 26660, 26808, 26955, 27101, 27245, 27387, 27527, 27666, 27803,
        27938, 28072, 28204, 28334, 28462, 28589, 28714, 28837, 28958, 29078,
        29196, 29312, 29426, 29538, 29648, 29757, 29864, 29969, 30072, 30173,
        30273, 30370, 30466, 30560, 30652, 30742, 30830, 30916, 31000, 31083,
        31163, 31242, 31318, 31393, 31466, 31537, 31606, 31673, 31738, 31801,
        31862, 31921, 31978, 32033, 32086, 32137, 32187, 32234, 32279, 32322,
        32364, 32403, 32440, 32475, 32509, 32540, 32569, 32596, 32622, 32645,
        32666, 32685, 32702, 32717, 32731, 32742, 32751, 32758, 32763, 32766,
        32767,
    };

    // 0 = L100%, 100 = C, 200 = R100%
    static inline const int16_t PAN_COS_TABLE[201] = {
        32767, 32766, 32763, 32758, 32751, 32742, 32731, 32717, 32702, 32685,
        32666, 32645, 32622, 32596, 32569, 32540, 32509, 32475, 32440, 32403,
        32364, 32322, 32279, 32234, 32187, 32137, 32086, 32033, 31978, 31921,
        31862, 31801, 31738, 31673, 31606, 31537, 31466, 31393, 31318, 31242,
        31163, 31083, 31000, 30916, 30830, 30742, 30652, 30560, 30466, 30370,
        30273, 30173, 30072, 29969, 29864, 29757, 29648, 29538, 29426, 29312,
        29196, 29078, 28958, 28837, 28714, 28589, 28462, 28334, 28204, 28072,
        27938, 27803, 27666, 27527, 27387, 27245, 27101, 26955, 26808, 26660,
        26509, 26357, 26203, 26048, 25891, 25732, 25572, 25411, 25247, 25083,
        24916, 24748, 24579, 24408, 24235, 24062, 23886, 23709, 23531, 23351,
        23170, 22987, 22803, 22617, 22431, 22242, 22053, 21862, 21669, 21475,
        21280, 21084, 20886, 20688, 20487, 20286, 20083, 19879, 19674, 19468,
        19260, 19051, 18841, 18630, 18418, 18204, 17990, 17774, 17557, 17340,
        17121, 16901, 16680, 16458, 16235, 16011, 15786, 15560, 15333, 15105,
        14876, 14646, 14415, 14184, 13952, 13718, 13484, 13249, 13013, 12777,
        12539, 12301, 12062, 11823, 11582, 11341, 11099, 10857, 10614, 10370,
        10126,  9880,  9635,  9389,  9142,  8894,  8646,  8398,  8149,  7899,
        7649,  7399,  7148,  6897,  6645,  6393,  6140,  5887,  5634,  5380,
        5126,  4872,  4617,  4362,  4107,  3851,  3596,  3340,  3084,  2827,
        2571,  2314,  2057,  1801,  1544,  1286,  1029,   772,   515,   257,
            0,
    };

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

    static inline int16_t fastClampInt16(int32_t x) {
        // ARMv7E-M の DSP命令(SSAT)を直接使う例
        // SSAT <Rd>, #<imm>, <Rn> : <Rn>を符号付で#<imm>ビット飽和して<Rd>へ格納
        // #<imm> = 16 なら 16ビット範囲(-32768..32767)へ飽和
        __asm__ volatile(
            "ssat %0, #16, %1\n\t"
            : "=r" (x)
            : "r"  (x)
        );
        return static_cast<int16_t>(x);
    }

    static inline float fastAbsf(float in) {
        float out;
        __asm__ volatile(
            "vabs.f32 %0, %1  \n" // out = absolute_value(in)
            : "=t" (out)
            : "t"  (in)
        );
        return out;
    }
};