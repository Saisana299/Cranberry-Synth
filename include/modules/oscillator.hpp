#pragma once

#include "types.hpp"
#include "handlers/audio.hpp"
#include "modules/envelope.hpp"
#include "utils/math.hpp"
#include "utils/wavetable.hpp"

class Oscillator {
public:
    struct Memory {
        Phase_t phase = 0;
        Phase_t delta = 0;
        Gain_t vel_vol = 0;
        uint8_t note = 60;  // エイリアシング防止用のキースケーリングに使用
    };

    Oscillator() {
        bit_padding = AudioMath::bitPadding32(wavetable_size);
    }

    void setFrequency(Memory& mem, uint8_t note);
    void setVelocity(Memory& mem, uint8_t velocity);
    void setPhase(Memory& mem, Phase_t phase);
    void reset(Memory& mem);
    void enable();
    void disable();

    void setLevel(Gain_t level);
    void setLevelNonLinear(uint8_t level);
    void setWavetable(uint8_t table_id);
    void setCoarse(float coarse);
    void setFine(float fine_level);
    void setDetune(int8_t detune_cents);  // -50 to +50 cents

    // レベルテーブル (Level 0-99 → Q15スケール 0-32767)
    // AudioMath::levelToLinear() を使用してキャッシュ
    static Gain_t level_table[100];
    static bool table_initialized;
    static void initTable() {
        if (table_initialized) return;

        for (size_t i = 0; i < 100; ++i) {
            level_table[i] = AudioMath::levelToLinear(i);
        }
        table_initialized = true;
    }

    /** @brief オシレーターの状態 */
    inline bool isActive() const {
        return enabled;
    }

    /** @brief オシレーターが有効かどうか */
    inline bool isEnabled() const {
        return enabled;
    }

    /** @brief 波形テーブルIDを取得 */
    inline uint8_t getWavetableId() const {
        // 現在の波形テーブルポインタからIDを逆引き
        for (uint8_t i = 0; i < 4; i++) {
            if (wavetable == WAVETABLES[i].data) {
                return i;
            }
        }
        return 0;
    }

    /** @brief レベルを取得 (0-99) */
    inline uint8_t getLevel() const {
        return level_raw;
    }

    /** @brief 線形レベルを取得 (Q15: 0-32767) */
    inline Gain_t getLevelLinear() const {
        return level;
    }

    /** @brief コースを取得 */
    inline float getCoarse() const {
        return coarse;
    }

    /** @brief ファインを取得 */
    inline float getFine() const {
        return fine_level;
    }

    /** @brief デチューンを取得 */
    inline int8_t getDetune() const {
        return detune_cents;
    }

    /** @brief FIXEDモードかどうかを取得 */
    inline bool isFixed() const {
        return is_fixed;
    }

    /** @brief FIXEDモードを設定 */
    void setFixed(bool fixed) {
        this->is_fixed = fixed;
    }

    /**
     * @brief オシレーターの状態を更新
     *
     * @param mem オシレーターメモリ
     * @param note_id ノートID
    */
    FASTRUN inline void update(Memory& mem) {
        if(enabled) mem.phase += mem.delta;
    }

    /**
     * @brief oscillatorのサンプルを取得
     *
     * 波形出力のみを返す。レベル・ベロシティはエンベロープ側で適用。
     *
     * @param mem オシレーターメモリ
     * @param mod_input 変調入力 (Q23)
     * @return Audio24_t オシレーター出力サンプル (Q23)
     */
    FASTRUN inline Audio24_t getSample(Memory& mem, Audio24_t mod_input = 0) {
        if(!enabled) return 0;

        // ローカル変数にキャッシュ
        const Phase_t local_phase = mem.phase;

        // キャリアのベース位相
        Phase_t base_phase = local_phase;

        // モジュレーションの位相オフセット
        // mod_inputをそのまま位相に加算
        // 位相は2^32で1周期
        // → mod_input (Q23) を 2^8 倍 (<<8) して位相スケールに合わせる
        // Q23 << 8 = Q31 ≒ 2^32位相スケール
        const int32_t mod_phase_offset = mod_input << MOD_PHASE_SHIFT;

        // 位相計算
        Phase_t effective_phase = base_phase + static_cast<Phase_t>(mod_phase_offset);

        // 波形テーブルを線形補間で参照
        // bit_padding: 32 - log2(wavetable_size) で、上位ビットがインデックス
        const uint32_t phase_shifted = effective_phase >> bit_padding;
        const size_t index = phase_shifted & (wavetable_size - 1);
        const size_t next_index = (index + 1) & (wavetable_size - 1);

        // 補間のための小数部を取得（bit_padding未満のビット）
        // 補間精度: 16ビット (0〜65535)
        const uint32_t frac_mask = (1U << bit_padding) - 1;
        const uint32_t frac = (effective_phase & frac_mask) >> (bit_padding - 16);

        // 線形補間: y0 + (y1 - y0) * frac / 65536
        const Audio24_t y0 = wavetable[index];
        const Audio24_t y1 = wavetable[next_index];
        const Audio24_t sample = y0 + (((y1 - y0) * static_cast<int32_t>(frac)) >> 16);

        // 波形出力のみを返す
        // レベル(Output Level)とベロシティはエンベロープ(outlevel)で適用される
        return sample;
    }

private:
    // 定数
    static constexpr float PHASE_SCALE_FACTOR = static_cast<float>(1ULL << 32) / SAMPLE_RATE;

    // FM変調の位相シフト量
    // mod_input(Q23)を位相(2^32)スケールに変換
    // 位相2^32で1周期
    // Q23 << 8 = Q31 ≒ 2^32スケール (実質2^31で半周期 ≈ π)
    // 最大変調インデックス ≈ π
    static constexpr int MOD_PHASE_SHIFT = 8;

    struct WavetableInfo {
        const Audio24_t* data;
        size_t size;
    };

    static constexpr WavetableInfo WAVETABLES[4] = {
        {Wavetable::sine,     sizeof(Wavetable::sine) / sizeof(Wavetable::sine[0])},
        {Wavetable::triangle, sizeof(Wavetable::triangle) / sizeof(Wavetable::triangle[0])},
        {Wavetable::saw,      sizeof(Wavetable::saw) / sizeof(Wavetable::saw[0])},
        {Wavetable::square,   sizeof(Wavetable::square) / sizeof(Wavetable::square[0])},
    };

    // パラメータ検証
    static inline Gain_t clamp_level(Gain_t value) {
        return std::clamp<Gain_t>(value, 0, Q15_MAX);
    }

    static inline float clamp_coarse(float value) { // TODO 小数点以下無効
        return std::clamp<float>(value, 0.0f, 31.0f);
    }

    static inline float clamp_fine(float value) { // TODO 小数点以下無効
        return std::clamp<float>(value, 0.0f, 99.0f);
    }

    static inline int8_t clamp_detune(int8_t value) {
        return std::clamp<int8_t>(value, -50, 50);
    }

    // OSC設定
    uint8_t bit_padding; // コンストラクタで初期化
    const Audio24_t* wavetable = Wavetable::sine;
    size_t wavetable_size = sizeof(Wavetable::sine) / sizeof(Wavetable::sine[0]);
    bool enabled = false;
    Gain_t level = 0;       // Q15スケール (0-32767)
    uint8_t level_raw = 0;  // 非線形レベル (0-99)

    // ピッチ
    float coarse = 1.0f;
    float fine_level = 0.0f;
    int8_t detune_cents = 0;

    // ratioかfixedか
    bool is_fixed = false;
};