#pragma once

#include <Audio.h>
#include <MIDI.h>

#include "modules/synth.hpp"
#include "utils/state.hpp"

class MIDIHandler {
private:
    // MIDI
    MIDI_NAMESPACE::SerialMIDI<HardwareSerial> serialMIDI = Serial1;
    MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial>> MIDI = serialMIDI;

    // Callback
    static inline void handleNoteOnStatic(uint8_t ch, uint8_t note, uint8_t velocity);
    static inline void handleNoteOffStatic(uint8_t ch, uint8_t note, uint8_t velocity);

    // インスタンス保持用
    static inline MIDIHandler* instance = nullptr;

    State& state_;

    void init();

    /**
     * @brief ノートON受信時の処理
     *
     * @param ch チャンネル番号
     * @param note ノート番号
     * @param velocity ベロシティ
     */
    void handleNoteOn(uint8_t ch, uint8_t note, uint8_t velocity);

    /**
     * @brief ノートOFF受信時の処理
     *
     * @param ch チャンネル番号
     * @param note ノート番号
     * @param velocity ベロシティ
     */
    void handleNoteOff(uint8_t ch, uint8_t note, uint8_t velocity);

public:
    MIDIHandler(State& state) : state_(state) {
        instance = this;
        init();
    }

    /** @brief MIDIデータ読み込み */
    void process();
};