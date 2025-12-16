#include "handlers/midi.hpp"

/** @brief MIDIデータ受信の開始 */
void MIDIHandler::begin() {
    usbMIDI.setHandleNoteOn(handleNoteOnStatic);
    usbMIDI.setHandleNoteOff(handleNoteOffStatic);
    MIDI.setHandleNoteOn(handleNoteOnStatic);
    MIDI.setHandleNoteOff(handleNoteOffStatic);
    MIDI.begin(MIDI_CHANNEL_OMNI);
}

/** @brief MIDIデータ受信の停止 */
void MIDIHandler::stop() {
    state_.setLedMidi(false);
    usbMIDI.setHandleNoteOn(nullptr);
    usbMIDI.setHandleNoteOff(nullptr);
    MIDI.setHandleNoteOn(nullptr);
    MIDI.setHandleNoteOff(nullptr);
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

    if(usbMIDI.read()) {
        // コールバックで自動処理
        midi_activity = true;
    }
    if(MIDI.read()) {
        // コールバックで自動処理
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