#include "modules/synth.h"

//todo: 強制リリースはバッファサイズ分で振幅0まで滑らかに減衰するようにする 次のノートは減衰処理完了後に入れる（同ループ）

void Synth::init() {
    instance = this;
    for(uint8_t i = 0; i < MAX_NOTES; i++) {
        active_notes[i] = ActiveSynthNote {0, 255, 0, 0, 0.0, 0.0, Envelope()};
    }
    float    sustain_level   = 1.0;
    uint32_t attack_samples  = 0.01 * SAMPLE_RATE;
    uint32_t decay_samples   = 0.1  * SAMPLE_RATE;
    uint32_t release_samples = 0.1  * SAMPLE_RATE;
    amp_adsr = ADSRConfig {
        attack_samples,
        decay_samples,
        sustain_level,
        release_samples
    };
}

void Synth::generate() {
    if(samples_ready) return;

    auto& wavetable = Wavetable::square;
    const size_t WAVETABLE_SIZE = sizeof(wavetable) / sizeof(wavetable[0]);

    // 出力バッファを初期化
    for (size_t i = 0; i < BUFFER_SIZE; ++i) {
        samples_L[i] = 0;
        samples_R[i] = 0;
    }

    // ノート毎処理
    for(uint8_t n = 0; n < MAX_NOTES; ++n) {
        if(active_notes[n].order != 0) {
            auto& note = active_notes[n];
            auto& phase = note.phase;
            auto& delta = note.delta;
            auto& amp_level = State::amp_level;
            auto& amp_env = note.amp_env;

            // サンプル毎処理
            for(size_t i = 0; i < BUFFER_SIZE; ++i) {
                size_t index = static_cast<size_t>(phase * WAVETABLE_SIZE) % WAVETABLE_SIZE;
                int16_t sample = wavetable[index];

                // 合計を出力バッファに追加
                samples_L[i] = samples_R[i] += sample * amp_level * amp_env.currentLevel();

                phase += delta;
                if(phase >= 1.0) phase -= 1.0;

                amp_env.update(amp_adsr, 1);
            }

            // リリースが終わっていればリセット
            if(amp_env.isFinished()) {
                resetNote(n);
            }
        }
    }

    samples_ready = true;
}

void Synth::update() {
    // 有効なノートが存在すれば生成
    // エフェクト系追加したら処理内容変更？
    if(order_max > 0) generate();
}

/**
 * @brief 
 * 
 * @param removed 
 */
void Synth::updateOrder(uint8_t removed) {
    for (uint8_t i = 0; i < MAX_NOTES; ++i) {
        if (active_notes[i].order > removed) {
            --active_notes[i].order;
            if(active_notes[i].order == 1) {
                last_index = i;
            }
        }
    }
}

/**
 * @brief 演奏するノートを追加します
 * 
 * @param note 
 * @param velocity 
 * @param channel 
 */
void Synth::noteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
    // 既に演奏している場合
    // ONの次に同じ音のONはありえない状況
    for (uint8_t i = 0; i < MAX_NOTES; ++i) {
        if(active_notes[i].note == note) return;
    }
    // MAX_NOTES個ノートがある場合
    if(order_max == MAX_NOTES) {
        // 一番古いノートを強制停止する
        resetNote(last_index);
    }
    // ノートを追加する
    for (uint8_t i = 0; i < MAX_NOTES; ++i) {
        if (active_notes[i].order == 0) {
            auto& it = active_notes[i];
            if(order_max < MAX_NOTES) ++order_max;
            float delta = AudioMath::noteToFrequency(note) / SAMPLE_RATE;
            it.order = order_max;
            it.note = note;
            it.velocity = velocity;
            it.channel = channel;
            it.phase = AudioMath::randomFloat4(0.0, 1.0);
            if(it.phase == 1.0) it.phase = 0.0;
            it.delta = delta;
            it.amp_env.reset();
            break;
        }
    }
}

/**
 * @brief ノートをリリースに移行
 * 
 * @param note 
 * @param channel 
 */
void Synth::noteOff(uint8_t note, uint8_t channel) {
    for (uint8_t i = 0; i < MAX_NOTES; ++i) {
        if (active_notes[i].note == note) {
            auto& it = active_notes[i];
            it.amp_env.release();
        }
    }
}

/**
 * @brief ノートをリセット
 * 
 * @param index
 */
void Synth::resetNote(uint8_t index) {
    uint8_t removed = 1;
    auto& it = active_notes[index];
    removed = it.order;
    it.order = 0;
    it.note = 255;
    it.velocity = 0;
    it.channel = 0;
    it.phase = 0.0;
    it.delta = 0.0;
    it.amp_env.reset();
    if(order_max > 0) --order_max;
    // 他ノートorder更新
    updateOrder(removed);
}