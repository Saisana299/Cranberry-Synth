#include "tools/midi_player.hpp"

void MIDIPlayer::init() {
    if(!SD.begin(BUILTIN_SDCARD)) {
        //TODO SDが無い場合を考慮
        is_initialized = false;
        return;
    }
    SMF.begin(&(SD.sdfs));
    SMF.setMidiHandler(midiCallbackStatic);
    SMF.looping(true);
    is_initialized = true;
}

void MIDIPlayer::midiCallback(midi_event *pev) {
    if (pev->data[0] >= 0xF0) return;

    uint8_t status = pev->data[0] & 0xF0;
    uint8_t channel = pev->data[0] & 0x0F;
    uint8_t note = pev->data[1];
    uint8_t velocity = pev->data[2];

    // 全MIDIイベントでSTATUS LEDを点灯
    state_.setLedStatus(true);

    AudioNoInterrupts();

    switch (status) {
        case 0x90:
            if(velocity > 0) {
                synth_.getInstance().noteOn(note, velocity, channel+1);
            } else {
                synth_.noteOff(note, channel+1);
            }
            break;

        case 0x80:
            synth_.noteOff(note, channel+1);
            break;
    }

    AudioInterrupts();
}

void MIDIPlayer::midiCallbackStatic(midi_event *pev) {
    if (instance) instance->midiCallback(pev);
}

void MIDIPlayer::play(const char* path) {
    // 初期化に失敗している、またはインスタンスがない場合は何もしない
    if (!instance || !instance->is_initialized) {
        Serial.println("Cannot play: MIDIPlayer not ready.");
        return;
    }

    // ファイル読み込みのエラーチェック
    int err = instance->SMF.load(path);
    if (err != MD_MIDIFile::E_OK) {
        Serial.printf("SMF Load Error: %d\n", err);
        instance->is_playing = false;
        instance->current_filename.clear();
    } else {
        instance->is_playing = true;
        instance->current_filename = path;
    }
}

void MIDIPlayer::stop() {
    if (instance && instance->is_initialized) {
        instance->is_playing = false;
        instance->current_filename.clear();
        instance->SMF.close();

        AudioNoInterrupts();
        instance->synth_.reset();
        AudioInterrupts();
    }
}

void MIDIPlayer::process() {
    // 初期化されていなければ処理しない
    if (!is_initialized || !is_playing) return;

    if (!SMF.isEOF()) {
        SMF.getNextEvent();
    }
}