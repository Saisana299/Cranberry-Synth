#pragma once

#include <Adafruit_GFX.h>

#include "display/gfx.hpp"
#include "handlers/audio.hpp"
#include "modules/envelope.hpp"
#include "modules/oscillator.hpp"
#include "modules/delay.hpp"
#include "modules/filter.hpp"
#include "utils/algorithm.hpp"
#include "utils/state.hpp"
#include "utils/math.hpp"
#include "utils/color.hpp"
#include "utils/preset.hpp"

//TODO チャンネル別で音色を選択できるようにする エフェクトの個別適用は処理速度を確認
constexpr uint8_t MAX_NOTES = 16;    // 最大同時発音数
constexpr uint8_t MAX_CHANNELS = 16; // 最大MIDIチャンネル数

class Synth {
private:
    struct SynthNote {
        uint8_t order = 0;    // ノート整理番号 0= 非発音, 1=最古, ...
        uint8_t note = 255;   // MIDIノート番号
        uint8_t velocity = 0; // MIDIベロシティ
        uint8_t channel = 0;  // MIDIチャンネル
    };
    SynthNote notes[MAX_NOTES] = {};

    volatile int8_t midi_note_to_index[128]; // MIDI番号ごとにノートのインデックスを記録　非発音時は-1

    struct OperatorState {
        Oscillator::Memory osc_mems[MAX_NOTES];
        Envelope::Memory env_mems[MAX_NOTES];
    };
    OperatorState ope_states[MAX_OPERATORS] = {};

    struct Operator {
        Oscillator osc = Oscillator{};
        Envelope env = Envelope{};
    };
    Operator operators[MAX_OPERATORS] = {};

    Delay delay = Delay{};
    uint32_t delay_remain = 0;
    bool delay_enabled = false;

    Filter filter = Filter{};
    bool lpf_enabled = false;
    bool hpf_enabled = false;

    uint8_t order_max = 0;
    uint8_t last_index = 0;

    Gain_t master_volume = static_cast<Gain_t>(Q15_MAX * 0.707); // 71% = 23170 (-3dB)
    Gain_t polyphony_divisor = Q15_MAX / MAX_NOTES; // 32767 / 16 = 2047
    uint8_t active_carriers = 1; // アクティブなキャリア数（最低1）

    // 最終スケール = master_volume / キャリア数 × polyphony_divisor
    Gain_t output_scale = static_cast<Gain_t>((static_cast<int32_t>(master_volume / active_carriers) * polyphony_divisor) >> Q15_SHIFT);

    // チャンネル別のバッファ
    Audio24_t left[MAX_CHANNELS] = {};
    Audio24_t right[MAX_CHANNELS] = {};
    Audio24_t fb_history[MAX_NOTES][2] = {};

    const Algorithm* current_algo = nullptr;
    uint8_t feedback_amount = 0; // 0=disable, 1~7
    uint8_t current_preset_id = 0; // 現在ロードされているプリセットID
    int8_t transpose = 0; // トランスポーズ (-24 ～ +24)

    FASTRUN void generate();
    void updateOrder(uint8_t removed);
    void noteReset(uint8_t index);

    Synth() {}

public:
    Synth(const Synth&) = delete;
    void operator=(const Synth&) = delete;

    static Synth& getInstance() {
        static Synth instance;
        return instance;
    };

    void init();
    FASTRUN void update();
    void noteOn(uint8_t note, uint8_t velocity, uint8_t channel);
    void noteOff(uint8_t note, uint8_t channel);
    void reset();

    void setAlgorithm(uint8_t algo_id);
    void setFeedback(uint8_t amount);
    void loadPreset(uint8_t preset_id);

    // --- 状態取得関数 ---
    uint8_t getActiveNoteCount() const {
        return order_max;
    }

    // プリセット情報
    uint8_t getCurrentPresetId() const {
        return current_preset_id;
    }

    const char* getCurrentPresetName() const;

    // アルゴリズム・フィードバック情報
    uint8_t getCurrentAlgorithmId() const;
    uint8_t getFeedbackAmount() const {
        return feedback_amount;
    }

    // オペレーター情報（読み取り専用アクセス）
    const Oscillator& getOperatorOsc(uint8_t op_index) const {
        return operators[op_index].osc;
    }

    const Envelope& getOperatorEnv(uint8_t op_index) const {
        return operators[op_index].env;
    }

    // エフェクト状態
    bool isDelayEnabled() const { return delay_enabled; }
    bool isLpfEnabled() const { return lpf_enabled; }
    bool isHpfEnabled() const { return hpf_enabled; }

    // エフェクトパラメータ取得
    int32_t getDelayTime() const { return delay.getTime(); }
    Gain_t getDelayLevel() const { return delay.getLevel(); }
    Gain_t getDelayFeedback() const { return delay.getFeedback(); }
    float getLpfCutoff() const { return filter.getLpfCutoff(); }
    float getLpfResonance() const { return filter.getLpfResonance(); }
    Gain_t getLpfMix() const { return filter.getLpfMix(); }
    float getHpfCutoff() const { return filter.getHpfCutoff(); }
    float getHpfResonance() const { return filter.getHpfResonance(); }
    Gain_t getHpfMix() const { return filter.getHpfMix(); }

    // エフェクト設定
    void setDelayEnabled(bool enabled) { delay_enabled = enabled; }
    void setLpfEnabled(bool enabled) { lpf_enabled = enabled; }
    void setHpfEnabled(bool enabled) { hpf_enabled = enabled; }

    // Delay/Filter オブジェクトへの直接アクセス（設定用）
    Delay& getDelay() { return delay; }
    Filter& getFilter() { return filter; }

    // マスター設定
    Gain_t getMasterLevel() const { return master_volume; }
    void setMasterLevel(Gain_t level) {
        master_volume = std::clamp<Gain_t>(level, 0, Q15_MAX);
        // キャリア数で割って正規化（loadPresetと同じ計算）
        output_scale = static_cast<Gain_t>((static_cast<int32_t>(master_volume / active_carriers) * polyphony_divisor) >> Q15_SHIFT);
    }

    // トランスポーズ
    int8_t getTranspose() const { return transpose; }
    void setTranspose(int8_t t) { transpose = std::clamp<int8_t>(t, -24, 24); }

    // int16_t getMasterPan() const { return master_pan; }
    // void setMasterPan(int16_t pan) { master_pan = std::clamp<int16_t>(pan, 0, 200); }
};