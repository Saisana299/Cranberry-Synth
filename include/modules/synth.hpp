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

constexpr uint8_t MAX_NOTES = 16;
constexpr uint8_t MAX_CHANNELS = 16; //TODO チャンネル別で音色を選択できるようにする エフェクトの個別適用は処理速度を確認
constexpr uint8_t MAX_VOICE = 8;

class Synth {
private:
    struct SynthNote {
        uint8_t order = 0;
        uint8_t note = 255;
        uint8_t velocity = 0;
        uint8_t channel = 0;
    };
    SynthNote notes[MAX_NOTES] = {};

    // 非発音時は-1、発音時はnotesのインデックス番号
    int8_t midi_note_to_index[128];

    struct OperatorState {
        Oscillator::Memory osc_mems[MAX_NOTES];
        Envelope::Memory env_mems[MAX_NOTES];
    };
    OperatorState ope_states[MAX_OPERATORS] = {};

    struct Operator { //TODO Operator ON/OFF機能
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
    int16_t amp_level = 1 << 10;
    int16_t adjust_level = (1 << 10) / MAX_NOTES;
    int16_t master_pan = 100;

    // 本来はamp_level * キャリアの数 * adjust_levelで調整する。
    int16_t master_scale = (static_cast<uint32_t>(amp_level) * adjust_level) >> 10;

    // チャンネル別のバッファ
    int32_t left[MAX_CHANNELS] = {};
    int32_t right[MAX_CHANNELS] = {};

    const Algorithm* current_algo = nullptr;
    int32_t fb_history[MAX_NOTES][2] = {};
    uint8_t feedback_amount = 0; // 0=disable, 1~7
    uint8_t current_preset_id = 0; // 現在ロードされているプリセットID

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
    int32_t getDelayLevel() const { return delay.getLevel(); }
    int32_t getDelayFeedback() const { return delay.getFeedback(); }
    float getLpfCutoff() const { return filter.getLpfCutoff(); }
    float getLpfResonance() const { return filter.getLpfResonance(); }
    int16_t getLpfMix() const { return filter.getLpfMix(); }
    float getHpfCutoff() const { return filter.getHpfCutoff(); }
    float getHpfResonance() const { return filter.getHpfResonance(); }
    int16_t getHpfMix() const { return filter.getHpfMix(); }

    // エフェクト設定
    void setDelayEnabled(bool enabled) { delay_enabled = enabled; }
    void setLpfEnabled(bool enabled) { lpf_enabled = enabled; }
    void setHpfEnabled(bool enabled) { hpf_enabled = enabled; }

    // Delay/Filter オブジェクトへの直接アクセス（設定用）
    Delay& getDelay() { return delay; }
    Filter& getFilter() { return filter; }

    // マスター設定
    int16_t getMasterLevel() const { return amp_level; }
    void setMasterLevel(int16_t level) { 
        amp_level = std::clamp<int16_t>(level, 0, 1024);
        master_scale = (static_cast<uint32_t>(amp_level) * adjust_level) >> 10;
    }
    int16_t getMasterPan() const { return master_pan; }
    void setMasterPan(int16_t pan) { master_pan = std::clamp<int16_t>(pan, 0, 200); }
};