#include "modules/synth.h"

void Synth::init() {
    instance = this;
    for(uint8_t i = 0; i < MAX_NOTES; i++) {
        active_notes[i].order = 0;
    }
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

    for(uint8_t n = 0; n < MAX_NOTES; ++n) {
        if(active_notes[n].order != 0) {
            auto& phase = active_notes[n].phase;
            auto& delta = active_notes[n].delta;

            for(size_t i = 0; i < BUFFER_SIZE; ++i) {
                size_t index = static_cast<size_t>(phase * WAVETABLE_SIZE) % WAVETABLE_SIZE;
                int16_t sample = wavetable[index];

                // 合計を出力バッファに追加
                samples_L[i] += sample / MAX_NOTES;
                samples_R[i] += sample / MAX_NOTES;

                phase += delta;
                if(phase >= 1.0) phase -= 1.0;
            }
        }
    }

    // 出力のクリッピング
    for (size_t i = 0; i < BUFFER_SIZE; ++i) {
        samples_L[i] = max(-32768, min(32767, samples_L[i]));
        samples_R[i] = max(-32768, min(32767, samples_R[i]));
    }

    samples_ready = true;
}

void Synth::update() {
    // ノートオフ
    if(off_note != nullptr) {
        uint8_t removed = 1;
        // ノート無効化
        for (uint8_t i = 0; i < MAX_NOTES; ++i) {
            if (active_notes[i].note == off_note->note) {
                removed = active_notes[i].order;
                active_notes[i].order = 0;
                if(order_max > 0) --order_max;
                break;
            }
        }
        delete off_note;
        off_note = nullptr;
        // 他ノートorder更新
        for (uint8_t i = 0; i < MAX_NOTES; ++i) {
            if (active_notes[i].order > removed) {
                active_notes[i].order -= 1;
            }
        }
    }
    // ノートオン
    if(on_note != nullptr) {
        if(order_max == MAX_NOTES) {
            for (uint8_t i = 0; i < MAX_NOTES; ++i) {
                if(active_notes[i].note == on_note->note) {
                    return;
                }
            }
            for (uint8_t i = 0; i < MAX_NOTES; ++i) {
                if(active_notes[i].order == 1) {
                    active_notes[i].order = 0;
                    if(order_max > 0) --order_max;
                    break;
                }
            }
            // 他ノートorder更新
            for (uint8_t i = 0; i < MAX_NOTES; ++i) {
                if (active_notes[i].order > 1) {
                    active_notes[i].order -= 1;
                }
            }
        }
        for (uint8_t i = 0; i < MAX_NOTES; ++i) {
            if (active_notes[i].order == 0) {
                if(order_max < MAX_NOTES) ++order_max;
                active_notes[i].order = order_max;
                active_notes[i].note = on_note->note;
                active_notes[i].phase = 0.0;
                active_notes[i].delta = AudioMath::noteToFrequency(on_note->note) / SAMPLE_RATE;
                break;
            }
        }
        delete on_note;
        on_note = nullptr;
    }
    // 有効なノートが存在すれば生成
    //todo
    generate();
}

void Synth::addNote(uint8_t note, uint8_t velocity, uint8_t channel) {
    on_note = new MidiNote{note, velocity, channel};
}

void Synth::removeNote(uint8_t note, uint8_t channel) {
    off_note = new MidiNote{note, 0, channel};
}