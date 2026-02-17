#pragma once

#include <Adafruit_GFX.h>

#include "display/gfx.hpp"
#include "handlers/audio.hpp"
#include "modules/envelope.hpp"
#include "modules/oscillator.hpp"
#include "modules/delay.hpp"
#include "modules/filter.hpp"
#include "modules/chorus.hpp"
#include "modules/reverb.hpp"
#include "modules/lfo.hpp"
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

    Delay* delay_ptr_ = nullptr;    // 共有インスタンス (main.cpp で生成)
    uint32_t fx_tail_remain = 0;     // エフェクトテール残りサンプル数
    bool delay_enabled = false;

    Filter* filter_ptr_ = nullptr;  // 共有インスタンス (main.cpp で生成)
    bool lpf_enabled = false;
    bool hpf_enabled = false;

    Chorus* chorus_ptr_ = nullptr;  // 共有インスタンス (main.cpp で生成)
    bool chorus_enabled = false;

    Reverb* reverb_ptr_ = nullptr;  // 共有インスタンス (main.cpp で生成)
    bool reverb_enabled = false;

    Lfo lfo_;
    bool osc_key_sync_ = true;
    Gain_t op_ams_gain_[MAX_OPERATORS] = {};  // オペレーター単位 AMS Q15スケール

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

    /**
     * @brief エフェクトテール長を計算
     * Delay (フィードバック減衰考慮) + Chorus バッファ長
     * フィードバックレベルが -60dB (0.001) 以下になるまでの繰り返し回数で算出
     */
    uint32_t calcFxTail() const {
        uint32_t tail = 0;
        if (delay_enabled && delay_ptr_) {
            // delay_length (getTotalSamples) は既にフィードバック考慮済みの総テール長
            // reset() で 0 になる場合があるので、直接パラメータから計算する
            int32_t time_ms = delay_ptr_->getTime();
            Gain_t fb = delay_ptr_->getFeedback();
            if (time_ms > 0 && fb > 0) {
                float fb_ratio = static_cast<float>(fb) / Q15_MAX;
                // -60dB (0.001) まで減衰するエコー回数
                float repeats_f = (fb_ratio > 0.001f)
                    ? (logf(0.001f) / logf(fb_ratio))
                    : 1.0f;
                if (repeats_f < 1.0f) repeats_f = 1.0f;
                if (repeats_f > 500.0f) repeats_f = 500.0f;
                uint32_t total_ms = static_cast<uint32_t>(repeats_f * time_ms);
                tail = (total_ms * SAMPLE_RATE) / 1000;
                // 上限 30秒 (テール中はノート合成なし、エフェクトチェーンのみで低CPU負荷)
                constexpr uint32_t MAX_TAIL = SAMPLE_RATE * 30;
                if (tail > MAX_TAIL) tail = MAX_TAIL;
            }
        }
        if (chorus_enabled && chorus_ptr_) {
            tail = std::max(tail, static_cast<uint32_t>(CHORUS_BUFFER_SIZE));
        }
        if (reverb_enabled && reverb_ptr_) {
            // Freeverb の最長コムフィルタ長 × フィードバック減衰回数
            // ~1617 samples × ~30 repeats ≈ 1.1秒
            uint32_t reverb_tail = (SAMPLE_RATE * 3); // 概算3秒
            tail = std::max(tail, reverb_tail);
        }
        return tail;
    }

    Synth() {}

public:
    Synth(const Synth&) = delete;
    void operator=(const Synth&) = delete;

    static Synth& getInstance() {
        static Synth instance;
        return instance;
    };

    void init(Delay& shared_delay, Filter& shared_filter, Chorus& shared_chorus, Reverb& shared_reverb);
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
    bool isChorusEnabled() const { return chorus_enabled; }
    bool isReverbEnabled() const { return reverb_enabled; }

    // エフェクトパラメータ取得
    int32_t getDelayTime() const { return delay_ptr_->getTime(); }
    Gain_t getDelayLevel() const { return delay_ptr_->getLevel(); }
    Gain_t getDelayFeedback() const { return delay_ptr_->getFeedback(); }
    float getLpfCutoff() const { return filter_ptr_->getLpfCutoff(); }
    float getLpfResonance() const { return filter_ptr_->getLpfResonance(); }
    Gain_t getLpfMix() const { return filter_ptr_->getLpfMix(); }
    float getHpfCutoff() const { return filter_ptr_->getHpfCutoff(); }
    float getHpfResonance() const { return filter_ptr_->getHpfResonance(); }
    Gain_t getHpfMix() const { return filter_ptr_->getHpfMix(); }

    // エフェクト設定
    void setDelayEnabled(bool enabled) {
        if (!delay_enabled && enabled) delay_ptr_->reset();
        delay_enabled = enabled;
    }
    void setLpfEnabled(bool enabled) {
        if (!lpf_enabled && enabled) filter_ptr_->reset();
        lpf_enabled = enabled;
    }
    void setHpfEnabled(bool enabled) {
        if (!hpf_enabled && enabled) filter_ptr_->reset();
        hpf_enabled = enabled;
    }
    void setChorusEnabled(bool enabled) {
        if (!chorus_enabled && enabled) chorus_ptr_->reset();
        chorus_enabled = enabled;
    }
    void setReverbEnabled(bool enabled) {
        if (!reverb_enabled && enabled) reverb_ptr_->reset();
        reverb_enabled = enabled;
    }

    // Delay/Filter/Chorus/Reverb オブジェクトへの直接アクセス（設定用）
    Delay& getDelay() { return *delay_ptr_; }
    Filter& getFilter() { return *filter_ptr_; }
    Chorus& getChorus() { return *chorus_ptr_; }
    Reverb& getReverb() { return *reverb_ptr_; }

    // コーラスパラメータ取得
    uint8_t getChorusRate() const { return chorus_ptr_->getRate(); }
    uint8_t getChorusDepth() const { return chorus_ptr_->getDepth(); }
    Gain_t getChorusMix() const { return chorus_ptr_->getMix(); }

    // リバーブパラメータ取得
    uint8_t getReverbRoomSize() const { return reverb_ptr_->getRoomSize(); }
    uint8_t getReverbDamping() const { return reverb_ptr_->getDamping(); }
    Gain_t getReverbMix() const { return reverb_ptr_->getMix(); }

    // LFO
    Lfo& getLfo() { return lfo_; }
    const Lfo& getLfo() const { return lfo_; }
    bool getOscKeySync() const { return osc_key_sync_; }
    void setOscKeySync(bool sync) { osc_key_sync_ = sync; }

    // オペレーター単位 AMS (0-3)
    uint8_t getOperatorAms(uint8_t op) const {
        if (op >= MAX_OPERATORS) return 0;
        for (uint8_t i = 0; i < 4; ++i) {
            if (op_ams_gain_[op] == Lfo::AMS_TAB[i]) return i;
        }
        return 0;
    }
    void setOperatorAms(uint8_t op, uint8_t ams) {
        if (op < MAX_OPERATORS)
            op_ams_gain_[op] = Lfo::AMS_TAB[ams & 3];
    }

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