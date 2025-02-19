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
    void setModulation(
        Oscillator* mod_osc,
        Envelope* mod_env,
        Oscillator::Memory* mod_osc_mems,
        Envelope::Memory* mod_env_mems
    );
    void setFeedback(bool is_feedback);
    void setLevel(int16_t level);
    void setLevelNonLinear(uint8_t level);
    void setWavetable(uint8_t table_id);
    void setCoarse(float coarse);
    void setFine(float fine_level);
    void setDetune(int8_t detune_cents);

    /** @brief オシレーターの状態 */
    inline bool isActive() {
        return enabled;
    }

    /**
     * @brief オシレーターの状態を更新
     *
     * @param note_id ノートID
    */
    inline void update(Memory& mem, uint8_t note_id) {
        if(!enabled) return;

        // 通常の処理
        mem.phase += mem.delta;

        // Modulationあり
        if (mod_osc && mod_env && mod_osc_mems && mod_env_mems) {
            //------ 危険地帯：mod_osc_memsとmod_env_memsの要素数がnote_id以上という前提 ------//

            // キャッシュ
            Oscillator::Memory& mod_mem = mod_osc_mems[note_id];
            Envelope::Memory& mod_env_mem = mod_env_mems[note_id];

            mod_osc->update(mod_mem, note_id);
            mod_env->update(mod_env_mem);

            // ----------------------------------------------------------------------------- //
        }
    }

    /**
     * @brief oscillatorのサンプルを取得
     *
     * @param note_id ノートID
     */
    inline int16_t getSample(Memory& mem, uint8_t note_id) {
        if(!enabled) return 0;

        // ローカル変数にキャッシュ
        const int16_t local_vel_vol = mem.vel_vol;
        const uint32_t local_phase = mem.phase;

        // キャリアのベース位相
        uint32_t base_phase = local_phase;

        // モジュレーションがある場合
        if(mod_osc && mod_env && mod_osc_mems && mod_env_mems) {
            //------ 危険地帯：mod_osc_memsとmod_env_memsの要素数がnote_id以上という前提 ------//

            // ローカルキャッシュ
            Oscillator* local_mod_osc = mod_osc;
            Envelope*   local_mod_env = mod_env;
            Oscillator::Memory* local_mod_osc_mems = mod_osc_mems;
            Envelope::Memory*   local_mod_env_mems = mod_env_mems;

            // モジュレーション用メモリの取得
            Oscillator::Memory& mod_mem = local_mod_osc_mems[note_id];
            Envelope::Memory&   mod_env_mem = local_mod_env_mems[note_id];

            // モジュレーターエンベロープレベル
            const int16_t mod_env_level = local_mod_env->currentLevel(mod_env_mem);
            // モジュレーターのサンプル取得
            const int16_t mod_sample = local_mod_osc->getSample(mod_mem, note_id);

            // モジュレーション度合いを計算
            const int32_t mod_product = (static_cast<int32_t>(mod_sample) * static_cast<int32_t>(mod_env_level)) >> 10;
            // モジュレーションの位相オフセット
            const uint32_t mod_phase_offset = static_cast<uint32_t>(abs(mod_product)) * 131072; // 131072 = (1 << 32) / 32768

            // base_phase に加減算
            base_phase += (mod_product < 0) ? -mod_phase_offset : mod_phase_offset;

            // ----------------------------------------------------------------------------- //
        }

        // 波形テーブルを参照する
        const size_t index = (base_phase >> bit_padding) & (wavetable_size - 1); // サンプル数が2^Nである前提
        const int16_t sample = wavetable[index];

        // ベロシティレベルとオシレーターレベルを適用
        const int32_t scaling = (static_cast<int32_t>(local_vel_vol) * static_cast<int32_t>(level)) >> 10;
        return static_cast<int16_t>((static_cast<int32_t>(sample) * scaling) >> 10);
    }

private:
    // 定数
    static constexpr float PHASE_SCALE_FACTOR = static_cast<float>(1ULL << 32) / SAMPLE_RATE;

    // OSC設定
    uint8_t bit_padding; // コンストラクタで初期化
    int16_t* wavetable = Wavetable::sine;
    size_t wavetable_size = sizeof(Wavetable::sine) / sizeof(Wavetable::sine[0]);
    bool enabled = false;
    int16_t level = 0;

    // ピッチ
    float coarse = 1.0f;
    float fine_level = 0.0f;
    int8_t detune_cents = 0;

    // ratioかfixedか
    bool is_fixed = false;

    // モジュレーション関連
    bool is_feedback = false;
    float feedback = 0.0f;
    Oscillator* mod_osc = nullptr;
    Envelope* mod_env = nullptr;
    Oscillator::Memory* mod_osc_mems = nullptr;
    Envelope::Memory* mod_env_mems = nullptr;
};