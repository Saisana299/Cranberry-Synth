#include "handlers/file.hpp"

void FileHandler::init() {
    if(!SD.begin(BUILTIN_SDCARD)) {
        //
        while(true);
    }
    SMF.begin(&(SD.sdfs));
    SMF.setMidiHandler(midiCallbackStatic);
    SMF.looping(true);
}

void FileHandler::midiCallback(midi_event *pev) {
    switch (pev->data[0]) {
        case 0x90:
            Synth::getInstance()->noteOn(pev->data[1], pev->data[2], 0);
            break;

        case 0x80:
            Synth::getInstance()->noteOff(pev->data[1], pev->data[2]);
            break;
    }
}

void FileHandler::midiCallbackStatic(midi_event *pev) {
    if (instance) instance->midiCallback(pev);
}

void FileHandler::play() {
    if (instance) instance->SMF.load(const_cast<const char*>("demo1.mid"));
}

void FileHandler::stop() {
    if (instance) instance->SMF.close();
}

void FileHandler::process() {
    if (!SMF.isEOF()) {
        SMF.getNextEvent();
    }
}