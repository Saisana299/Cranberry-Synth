#ifndef MIDI_HANDLER_H
#define MIDI_HANDLER_H

#include <Audio.h>
#include <MIDI.h>

class MIDIHandler {
public:
    void init();    // 初期化
    void process(); // 処理
};

#endif