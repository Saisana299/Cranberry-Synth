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

    // MIDI Note (0-127) to Frequency Table
    static inline const float NOTE_FREQ_TABLE[128] = {
           8.176f,    8.662f,    9.177f,    9.723f,    10.301f,    10.913f,    11.562f,    12.250f,   12.978f,   13.750f,   14.568f,   15.434f,
          16.352f,   17.324f,   18.354f,   19.445f,    20.602f,    21.827f,    23.125f,    24.500f,   25.957f,   27.500f,   29.135f,   30.868f,
          32.703f,   34.648f,   36.708f,   38.891f,    41.203f,    43.654f,    46.249f,    49.000f,   51.913f,   55.000f,   58.270f,   61.735f,
          65.406f,   69.296f,   73.416f,   77.782f,    82.407f,    87.307f,    92.499f,    97.999f,  103.826f,  110.000f,  116.541f,  123.471f,
         130.813f,  138.591f,  146.832f,  155.563f,   164.814f,   174.614f,   184.997f,   195.998f,  207.652f,  220.000f,  233.082f,  246.942f,
         261.626f,  277.183f,  293.665f,  311.127f,   329.628f,   349.228f,   369.994f,   391.995f,  415.305f,  440.000f,  466.164f,  493.883f,
         523.251f,  554.365f,  587.330f,  622.254f,   659.255f,   698.456f,   739.989f,   783.991f,  830.609f,  880.000f,  932.328f,  987.767f,
        1046.502f, 1108.731f, 1174.659f, 1244.508f,  1318.510f,  1396.913f,  1479.978f,  1567.982f, 1661.219f, 1760.000f, 1864.655f, 1975.533f,
        2093.005f, 2217.461f, 2349.318f, 2489.016f,  2637.020f,  2793.826f,  2959.955f,  3135.963f, 3322.438f, 3520.000f, 3729.310f, 3951.066f,
        4186.009f, 4434.922f, 4698.636f, 4978.032f,  5274.041f,  5587.652f,  5919.911f,  6271.927f, 6644.875f, 7040.000f, 7458.620f, 7902.133f,
        8372.018f, 8869.844f, 9397.273f, 9956.063f, 10548.080f, 11175.300f, 11839.820f, 12543.850f
    };

    static constexpr float ONE_OVER_127 = 1.0f / 127.0f;
    static constexpr float INT16_TO_FLOAT = 1.0f / 32767.0f;

    // ベロシティテーブル
    // velocity 0-127 を 64エントリに圧縮（velocity >> 1 でインデックス）
    // 出力は 0-254 の範囲、非線形カーブで低ベロシティの感度が高い
    static inline const uint8_t VELOCITY_TABLE[64] = {
        0, 70, 86, 97, 106, 114, 121, 126, 132, 138, 142, 148, 152, 156, 160, 163,
        166, 170, 173, 174, 178, 181, 184, 186, 189, 190, 194, 196, 198, 200, 202,
        205, 206, 209, 211, 214, 216, 218, 220, 222, 224, 225, 227, 229, 230, 232,
        233, 235, 237, 238, 240, 241, 242, 243, 244, 246, 246, 248, 249, 250, 251,
        252, 253, 254
    };

    // レベル → TL (Total Level) 変換テーブル
    // Level 0 = TL 127 (無音), Level 99 = TL 0 (最大音量)
    static constexpr uint8_t LEVEL_TO_TL[100] = {
        127, 122, 118, 114, 110, 107, 104, 102, 100, 98,  // 0-9
         96,  94,  92,  90,  88,  86,  85,  84,  82,  81,  // 10-19
         79,  78,  77,  76,  75,  74,  73,  72,  71,  70,  // 20-29
         69,  68,  67,  66,  65,  64,  63,  62,  61,  60,  // 30-39
         59,  58,  57,  56,  55,  54,  53,  52,  51,  50,  // 40-49
         49,  48,  47,  46,  45,  44,  43,  42,  41,  40,  // 50-59
         39,  38,  37,  36,  35,  34,  33,  32,  31,  30,  // 60-69
         29,  28,  27,  26,  25,  24,  23,  22,  21,  20,  // 70-79
         19,  18,  17,  16,  15,  14,  13,  12,  11,  10,  // 80-89
          9,   8,   7,   6,   5,   4,   3,   2,   1,   0   // 90-99
    };

    /**
     * @brief レベル(0-99)を線形スケール(0-1024)に変換
     * @param level オペレータレベル (0-99)
     * @return int16_t 線形スケールの音量 (0-1024)
     */
    static inline int16_t levelToLinear(uint8_t level) {
        if (level >= 100) level = 99;
        uint8_t tl = LEVEL_TO_TL[level];
        if (tl >= 127) {
            return 0;
        }
        // TLを対数スケールから線形スケールに変換
        // dB = -0.75 * TL (約96dB range for TL 0-127)
        float db = -0.75f * tl;
        float linear = powf(10.0f, db / 20.0f);
        return static_cast<int16_t>(linear * 1024.0f);
    }

    /**
     * @brief MIDIノートを周波数に変換
     *
     * @param note
     * @return float
     */
    static inline float noteToFrequency(uint8_t note) {
        if (note > 127) note = 127;
        return NOTE_FREQ_TABLE[note];
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
        float pitch_mod = coarse * (1.0f + fine_level);
        float detune_factor = 1.0f;
        if(detune_cents != 0) {
            detune_factor = powf(2.0f, detune_cents * 0.00083333333f);
        }
        return noteToFrequency(note) * pitch_mod * detune_factor;
    }

    /**
     * @brief 固定周波数モードで周波数を計算
     *
     * @param detune_cents デチューンするcent数
     * @param coarse 粗調整 (0-31、下位2ビット0-3が使用される)
     * @param fine_level 微調整 (0-99)
     * @return float 固定周波数（Hz）
     *
     * 計算式: freq = 10^(coarse & 3) * exp(ln(10) * fine / 100)
     * - coarse & 3 = 0: 1Hz ベース (1.0 ~ 9.77 Hz)
     * - coarse & 3 = 1: 10Hz ベース (10 ~ 97.7 Hz)
     * - coarse & 3 = 2: 100Hz ベース (100 ~ 977 Hz)
     * - coarse & 3 = 3: 1000Hz ベース (1000 ~ 9772 Hz)
     */
    static inline float fixedToFrequency(int8_t detune_cents, float coarse, float fine_level) {
        // freq = 10^(coarse & 3) * exp(ln(10) * fine / 100)
        // ln(10) ≈ 2.302585
        static constexpr float LN10 = 2.302585093f;
        static constexpr float FIXED_BASE[4] = {1.0f, 10.0f, 100.0f, 1000.0f};

        uint8_t coarse_int = static_cast<uint8_t>(coarse);
        float base_freq = FIXED_BASE[coarse_int & 0x03];
        float freq = base_freq * expf(LN10 * fine_level * 0.01f);

        // デチューン適用
        float detune_factor = 1.0f;
        if (detune_cents != 0) {
            detune_factor = powf(2.0f, detune_cents * 0.00083333333f);
        }

        return freq * detune_factor;
    }

    /**
     * @brief ベロシティを音量レベルに変換
     *
     * @param velocity MIDIベロシティ (0-127)
     * @return float 0.0〜1.0 の音量係数
     */
    static inline float velocityToAmplitude(uint8_t velocity) {
        if (velocity == 0) return 0.0f;
        if (velocity > 127) velocity = 127;

        // テーブルから非線形値を取得 (0-254)
        uint8_t vel_idx = velocity >> 1;  // 0-63
        float vel_val = static_cast<float>(VELOCITY_TABLE[vel_idx]);

        // 254を1.0にマッピング
        return vel_val * (1.0f / 254.0f);
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
        if(size == 0) return 32;
        return __builtin_clz(size) + 1;
    }

    /**
     * @brief パン値から左チャンネルの係数を取得
     *
     * @param m_pan -1.0[Left] ~ 1.0[Right]
     * @return float
     */
    static inline float lChPanCoef(float m_pan) {
        return getPanFromTable(m_pan, PAN_COS_TABLE);
    }

    /**
     * @brief パン値から右チャンネルの係数を取得
     *
     * @param m_pan -1.0[Left] ~ 1.0[Right]
     * @return float
     */
    static inline float rChPanCoef(float m_pan) {
        return getPanFromTable(m_pan, PAN_SIN_TABLE);
    }

    static inline float getPanFromTable(float pan, const int16_t* table) {
        // pan -1.0 ~ 1.0 を 0 ~ 200 に変換
        float index_f = (pan + 1.0f) * 100.0f;

        // 範囲制限
        if (index_f <= 0.0f) return table[0] * INT16_TO_FLOAT;
        if (index_f >= 200.0f) return table[200] * INT16_TO_FLOAT;

        // 整数部と小数部に分離
        int index_i = static_cast<int>(index_f);
        float frac = index_f - index_i;

        // 線形補間: table[i] * (1-t) + table[i+1] * t
        float val1 = table[index_i];
        float val2 = table[index_i + 1];

        return (val1 + frac * (val2 - val1)) * INT16_TO_FLOAT;
    }

    static inline int16_t fastClampInt16(int32_t in) {
        // ARMv7E-M の DSP命令(SSAT)を直接使う例
        // SSAT <Rd>, #<imm>, <Rn> : <Rn>を符号付で#<imm>ビット飽和して<Rd>へ格納
        // #<imm> = 16 なら 16ビット範囲(-32768..32767)へ飽和
        int32_t out;
        __asm__ volatile(
            "ssat %0, #16, %1\n\t"
            : "=r" (out)
            : "r"  (in)
        );
        return static_cast<int16_t>(out);
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