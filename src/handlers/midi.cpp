#include "handlers/midi.hpp"

/** @brief MIDIデータ受信の開始 */
void MIDIHandler::begin() {
    usbMIDI.setHandleNoteOn(handleNoteOnStatic);
    usbMIDI.setHandleNoteOff(handleNoteOffStatic);
    usbMIDI.setHandlePitchChange(handlePitchBendStaticUsb);
    usbMIDI.setHandleControlChange(handleControlChangeStaticUsb);
    MIDI.setHandleNoteOn(handleNoteOnStatic);
    MIDI.setHandleNoteOff(handleNoteOffStatic);
    MIDI.setHandlePitchBend(handlePitchBendStaticSerial);
    MIDI.setHandleControlChange(handleControlChangeStaticSerial);
    MIDI.begin(MIDI_CHANNEL_OMNI);
}

/** @brief MIDIデータ受信の停止 */
void MIDIHandler::stop() {
    state_.setLedMidi(false);
    usbMIDI.setHandleNoteOn(nullptr);
    usbMIDI.setHandleNoteOff(nullptr);
    usbMIDI.setHandlePitchChange(nullptr);
    usbMIDI.setHandleControlChange(nullptr);
    MIDI.setHandleNoteOn(nullptr);
    MIDI.setHandleNoteOff(nullptr);
    MIDI.setHandlePitchBend(nullptr);
    MIDI.setHandleControlChange(nullptr);
}

/**
 * @brief ノートON受信時の処理
 *
 * NoteOn velocity=0 は MIDI規格上 NoteOff として扱う。
 * Teensy usbMIDI はこの変換を自動で行わないため、ここで明示的に処理する。
 * Arduino MIDI Library (Serial) は HandleNullVelocityNoteOnAsNoteOff=true がデフォルトだが
 * 安全のためこちらでも対応する。
 *
 * @param ch チャンネル番号
 * @param note ノート番号
 * @param velocity ベロシティ
 */
void MIDIHandler::handleNoteOn(uint8_t ch, uint8_t note, uint8_t velocity) {
    // NoteOn velocity=0 → NoteOff として処理
    if (note <= MIDI_MAX_NOTE && velocity == 0) {
        handleNoteOff(ch, note, velocity);
        return;
    }

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

/**
 * @brief USB MIDI Control Changeコールバック
 * CC#120: All Sound Off — 全ノートを即座にリセット
 * CC#123: All Notes Off — 全ノートをリリース
 */
void MIDIHandler::handleControlChangeStaticUsb(uint8_t ch, uint8_t cc, uint8_t value) {
    if (instance && instance->state_.getModeState() == MODE_SYNTH) {
        instance->handleControlChange(ch, cc, value);
    }
}

/**
 * @brief Serial MIDI Control Changeコールバック
 */
void MIDIHandler::handleControlChangeStaticSerial(uint8_t ch, uint8_t cc, uint8_t value) {
    if (instance && instance->state_.getModeState() == MODE_SYNTH) {
        instance->handleControlChange(ch, cc, value);
    }
}

/**
 * @brief Control Change処理
 *
 * @param ch チャンネル番号
 * @param cc CC番号
 * @param value 値
 */
void MIDIHandler::handleControlChange(uint8_t ch, uint8_t cc, uint8_t value) {
    switch (cc) {
        case 120: // All Sound Off — 即座に全ノートリセット
            Synth::getInstance().reset();
            Synth::getInstance().setPitchBend(0);
            break;
        case 123: // All Notes Off — 全ノートをリリース
            Synth::getInstance().allNotesOff();
            break;
        default:
            break;
    }
}
