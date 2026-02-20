#pragma once

#include <Audio.h>
#include <MIDI.h>

#include "modules/synth.hpp"
#include "utils/state.hpp"

constexpr uint8_t MIDI_MAX_NOTE = 127;
constexpr uint8_t MIDI_MAX_VELOCITY = 127;

class MIDIHandler {
private:
    // MIDI
    MIDI_NAMESPACE::SerialMIDI<HardwareSerial> serialMIDI = Serial7;
    MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial>> MIDI = serialMIDI;

    // Callback (Note On/Off + Pitch Bend + Control Change)
    static void handleNoteOnStatic(uint8_t ch, uint8_t note, uint8_t velocity);
    static void handleNoteOffStatic(uint8_t ch, uint8_t note, uint8_t velocity);
    static void handlePitchBendStaticUsb(uint8_t ch, int bend);
    static void handlePitchBendStaticSerial(uint8_t ch, int bend);
    static void handleControlChangeStaticUsb(uint8_t ch, uint8_t cc, uint8_t value);
    static void handleControlChangeStaticSerial(uint8_t ch, uint8_t cc, uint8_t value);

    // インスタンス保持用
    static inline MIDIHandler* instance = nullptr;

    State& state_;
    bool last_midi_state = false;

    static inline bool isValidNoteOn(uint8_t note, uint8_t velocity) {
        return note <= MIDI_MAX_NOTE && velocity <= MIDI_MAX_VELOCITY && velocity > 0;
    }

    static inline bool isValidNoteOff(uint8_t note, uint8_t velocity) {
        return note <= MIDI_MAX_NOTE && velocity <= MIDI_MAX_VELOCITY;
    }

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

    /**
     * @brief Control Change受信時の処理
     *
     * @param ch チャンネル番号
     * @param cc CC番号
     * @param value 値
     */
    void handleControlChange(uint8_t ch, uint8_t cc, uint8_t value);

public:
    MIDIHandler(State& state) : state_(state) {
        instance = this;
    }

    void begin();
    void stop();

    /** @brief MIDIデータ読み込み */
    void process();
};