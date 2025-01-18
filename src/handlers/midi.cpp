#include "handlers/midi.h"

void MIDIHandler::init() {
    instance = this;
    MIDI.setHandleNoteOn(handleNoteOnStatic);
    MIDI.setHandleNoteOff(handleNoteOffStatic);
    MIDI.begin(MIDI_CHANNEL_OMNI);
}

/**
 * @brief 
 * 
 * @param ch 
 * @param note 
 * @param velocity 
 */
void MIDIHandler::handleNoteOn(uint8_t ch, uint8_t note, uint8_t velocity) {
    //
}

/**
 * @brief 
 * 
 * @param ch 
 * @param note 
 * @param velocity 
 */
void MIDIHandler::handleNoteOff(uint8_t ch, uint8_t note, uint8_t velocity) {
    //
}

/**
 * @brief MIDIデータ読み込み
 */
void MIDIHandler::process() {
    MIDI.read();
}

// static
void MIDIHandler::handleNoteOnStatic(uint8_t ch, uint8_t note, uint8_t velocity) {
    if (instance) instance->handleNoteOn(ch, note, velocity);
}
void MIDIHandler::handleNoteOffStatic(uint8_t ch, uint8_t note, uint8_t velocity) {
    if (instance) instance->handleNoteOff(ch, note, velocity);
}