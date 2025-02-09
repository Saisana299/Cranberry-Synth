#include "handlers/midi.hpp"

/** @brief MIDIハンドラ初期化 */
void MIDIHandler::init() {
    usbMIDI.setHandleNoteOn(handleNoteOnStatic);
    usbMIDI.setHandleNoteOff(handleNoteOffStatic);
    MIDI.setHandleNoteOn(handleNoteOnStatic);
    MIDI.setHandleNoteOff(handleNoteOffStatic);
    MIDI.begin(MIDI_CHANNEL_OMNI);
}

/**
 * @brief ノートON受信時の処理
 *
 * @param ch チャンネル番号
 * @param note ノート番号
 * @param velocity ベロシティ
 */
void MIDIHandler::handleNoteOn(uint8_t ch, uint8_t note, uint8_t velocity) {
    auto& mode_state = State::mode_state;
    if (mode_state == MODE_SYNTH)
        Synth::getInstance()->noteOn(note, velocity, ch);
}

/**
 * @brief ノートOFF受信時の処理
 *
 * @param ch チャンネル番号
 * @param note ノート番号
 * @param velocity ベロシティ
 */
void MIDIHandler::handleNoteOff(uint8_t ch, uint8_t note, uint8_t velocity) {
    auto& mode_state = State::mode_state;
    if (mode_state == MODE_SYNTH)
        Synth::getInstance()->noteOff(note, ch);
}

/** @brief MIDIデータ読み込み */
void MIDIHandler::process() {
    auto& led_midi = State::led_midi;
    bool temp = false;
    if(usbMIDI.read()){
        temp = true;
    }
    if(MIDI.read()){
        temp = true;
    }
    if(temp) {
        led_midi = !led_midi;
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