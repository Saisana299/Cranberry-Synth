#include "handlers/file.hpp"

void FileHandler::init() {
    if(!SD.begin(BUILTIN_SDCARD)) {
        //todo SDが無い場合を考慮
        while(true);
    }
    SMF.begin(&(SD.sdfs));
    SMF.setMidiHandler(midiCallbackStatic);
    SMF.looping(true);
}

void FileHandler::midiCallback(midi_event *pev) {
    uint8_t status = pev->data[0] & 0xF0;
    uint8_t channel = pev->data[0] & 0x0F;
    bool temp = false;//todo

    switch (status) {
        case 0x90:
            Synth::getInstance()->noteOn(pev->data[1], pev->data[2], channel+1);
            temp = true;//todo
            break;

        case 0x80:
            Synth::getInstance()->noteOff(pev->data[1], channel+1);
            temp = false;//todo
            break;
    }

    state_.setLedMidi(temp);//todo
}

void FileHandler::midiCallbackStatic(midi_event *pev) {
    if (instance) instance->midiCallback(pev);
}

void FileHandler::play(const char* path) {
    if (instance) instance->SMF.load(path);
}

void FileHandler::stop() {
    if (instance) {
        instance->SMF.close();
        Synth::getInstance()->reset();
    }
}

void FileHandler::process() {
    if (!SMF.isEOF()) {
        SMF.getNextEvent();
    }
}