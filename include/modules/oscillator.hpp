#pragma once

#include "handlers/audio.hpp"
#include "modules/envelope.hpp"
#include "utils/math.hpp"
#include "utils/wavetable.hpp"

class Oscillator {
public:
    struct Memory {
        uint32_t phase = 0;
        uint32_t delta = 0;
        int16_t vel_vol = 0;
        uint8_t note = 60;  // エイリアシング防止用のキースケーリングに使用
    };

    Oscillator() {
        bit_padding = AudioMath::bitPadding32(wavetable_size);
    }

    void setFrequency(Memory& mem, uint8_t note);
    void setVelocity(Memory& mem, uint8_t velocity);
    void setPhase(Memory& mem, uint32_t phase);
    void reset(Memory& mem);
    void enable();
    void disable();

    void setLevel(int16_t level);
    void setLevelNonLinear(uint8_t level);
    void setWavetable(uint8_t table_id);
    void setCoarse(float coarse);
    void setFine(float fine_level);
    void setDetune(int8_t detune_cents);

    // レベルテーブル (Level 0-99 → 線形スケール 0-1024)
    // AudioMath::levelToLinear() を使用してキャッシュ
    static int16_t level_table[100];
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

    /** @brief 線形レベルを取得 (0-1024) */
    inline int16_t getLevelLinear() const {
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
     * @param mem オシレーターメモリ
     * @param note_id ノートID
     * @return int16_t オシレーター出力サンプル
     */
    FASTRUN inline int16_t getSample(Memory& mem, int32_t mod_input = 0) {
        if(!enabled) return 0;

        // ローカル変数にキャッシュ
        const int16_t local_vel_vol = mem.vel_vol;
        const uint32_t local_phase = mem.phase;

        // キャリアのベース位相
        uint32_t base_phase = local_phase;

        // モジュレーションの位相オフセット
        // mod_input の範囲: ±32767程度（オペレーター出力）
        // 位相の範囲: 0〜2^32 (1周期)
        // スケーリング: mod_input を位相スケールに変換
        //
        // エイリアシング防止: 高音域で変調深度を自動的に減らす
        // ノート60(C4)を基準として、それより高い音では変調を抑える
        // ノート84(C6)以上では変調深度が1/4になる
        const int32_t key_scale = AudioMath::MOD_KEY_SCALE_TABLE[mem.note];
        const int32_t scaled_mod = (mod_input * key_scale) >> 10;
        const int32_t mod_phase_offset = scaled_mod << MOD_PHASE_SHIFT;

        // 位相計算
        uint32_t effective_phase = base_phase + static_cast<uint32_t>(mod_phase_offset);

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
        const int32_t y0 = wavetable[index];
        const int32_t y1 = wavetable[next_index];
        const int32_t sample = y0 + (((y1 - y0) * static_cast<int32_t>(frac)) >> 16);

        // ベロシティレベルとオシレーターレベルを適用
        const int32_t scaling = (static_cast<int32_t>(local_vel_vol) * static_cast<int32_t>(level)) >> 10;
        return static_cast<int16_t>((sample * scaling) >> 10);
    }

private:
    // 定数
    static constexpr float PHASE_SCALE_FACTOR = static_cast<float>(1ULL << 32) / SAMPLE_RATE;

    // FM変調の位相シフト量
    static constexpr int MOD_PHASE_SHIFT = 18;

    struct WavetableInfo {
        const int16_t* data;
        size_t size;
    };

    static constexpr WavetableInfo WAVETABLES[4] = {
        {Wavetable::sine,     sizeof(Wavetable::sine) / sizeof(Wavetable::sine[0])},
        {Wavetable::triangle, sizeof(Wavetable::triangle) / sizeof(Wavetable::triangle[0])},
        {Wavetable::saw,      sizeof(Wavetable::saw) / sizeof(Wavetable::saw[0])},
        {Wavetable::square,   sizeof(Wavetable::square) / sizeof(Wavetable::square[0])},
    };

    // パラメータ検証
    static inline int16_t clamp_level(int16_t value) {
        return std::clamp<int16_t>(value, 0, 1024);
    }

    static inline float clamp_coarse(float value) { // TODO 小数点以下無効
        return std::clamp<float>(value, 0.0f, 31.0f);
    }

    static inline float clamp_fine(float value) { // TODO 小数点以下無効
        return std::clamp<float>(value, 0.0f, 99.0f);
    }

    static inline int8_t clamp_detune(int8_t value) {
        return std::clamp<int8_t>(value, -7, 7);
    }

    // OSC設定
    uint8_t bit_padding; // コンストラクタで初期化
    const int16_t* wavetable = Wavetable::sine;
    size_t wavetable_size = sizeof(Wavetable::sine) / sizeof(Wavetable::sine[0]);
    bool enabled = false;
    int16_t level = 0;      // 線形スケール (0-1024)
    uint8_t level_raw = 0;  // 非線形レベル (0-99)

    // ピッチ
    float coarse = 1.0f;
    float fine_level = 0.0f;
    int8_t detune_cents = 0;

    // ratioかfixedか
    bool is_fixed = false;
};