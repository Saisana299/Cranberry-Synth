#include "handlers/midi.hpp"

/** @brief MIDIデータ受信の開始 */
void MIDIHandler::begin() {
    usbMIDI.setHandleNoteOn(handleNoteOnStatic);
    usbMIDI.setHandleNoteOff(handleNoteOffStatic);
    usbMIDI.setHandlePitchChange(handlePitchBendStaticUsb);
    MIDI.setHandleNoteOn(handleNoteOnStatic);
    MIDI.setHandleNoteOff(handleNoteOffStatic);
    MIDI.setHandlePitchBend(handlePitchBendStaticSerial);
    MIDI.begin(MIDI_CHANNEL_OMNI);
}

/** @brief MIDIデータ受信の停止 */
void MIDIHandler::stop() {
    state_.setLedMidi(false);
    usbMIDI.setHandleNoteOn(nullptr);
    usbMIDI.setHandleNoteOff(nullptr);
    usbMIDI.setHandlePitchChange(nullptr);
    MIDI.setHandleNoteOn(nullptr);
    MIDI.setHandleNoteOff(nullptr);
    MIDI.setHandlePitchBend(nullptr);
}

/**
 * @brief ノートON受信時の処理
 *
 * @param ch チャンネル番号
 * @param note ノート番号
 * @param velocity ベロシティ
 */
void MIDIHandler::handleNoteOn(uint8_t ch, uint8_t note, uint8_t velocity) {
    if(!isValidNoteOn(note, velocity)) return;

    if (state_.getModeState() == MODE_SYNTH) {
        Synth::getInstance().noteOn(note, velocity, ch);
    }
}

/**
 * @brief ノートOFF受信時の処理
 *
 * @param ch チャンネル番号
 * @param note ノート番号
 * @param velocity ベロシティ
 */
void MIDIHandler::handleNoteOff(uint8_t ch, uint8_t note, uint8_t velocity) {
    if(!isValidNoteOff(note, velocity)) return;

    if (state_.getModeState() == MODE_SYNTH) {
        Synth::getInstance().noteOff(note, ch);
    }
}

/** @brief MIDIデータ読み込み */
void MIDIHandler::process() {
    bool midi_activity = false;

    // USB MIDI: バッファ内の全メッセージを処理（whileで排出）
    // Note On/Off, Pitch Bend はすべてコールバックで自動処理
    while(usbMIDI.read()) {
        midi_activity = true;
    }

    // Serial MIDI: バッファ内の全メッセージを処理
    while(MIDI.read()) {
        midi_activity = true;
    }

    if(midi_activity != last_midi_state && state_.getModeState() != MODE_TITLE) {
        last_midi_state = midi_activity;
        state_.setLedMidi(midi_activity);
    }
}

/** @brief instance->handleNoteOn */
void MIDIHandler::handleNoteOnStatic(uint8_t ch, uint8_t note, uint8_t velocity) {
    if (instance) instance->handleNoteOn(ch, note, velocity);
}
/** @brief instance->handleNoteOff */
void MIDIHandler::handleNoteOffStatic(uint8_t ch, uint8_t note, uint8_t velocity) {
    if (instance) instance->handleNoteOff(ch, note, velocity);
}

/**
 * @brief USB MIDIピッチベンドコールバック
 * Teensy usbMIDI.setHandlePitchChange: 値は 0-16383 (中心=8192)
 */
void MIDIHandler::handlePitchBendStaticUsb(uint8_t ch, int bend) {
    if (instance && instance->state_.getModeState() == MODE_SYNTH) {
        Synth::getInstance().setPitchBend(static_cast<int16_t>(bend - 8192));
    }
}

/**
 * @brief Serial MIDIピッチベンドコールバック
 * Arduino MIDI Library setHandlePitchBend: 値は -8192～+8191 (中心=0)
 */
void MIDIHandler::handlePitchBendStaticSerial(uint8_t ch, int bend) {
    if (instance && instance->state_.getModeState() == MODE_SYNTH) {
        Synth::getInstance().setPitchBend(static_cast<int16_t>(bend));
    }
}
