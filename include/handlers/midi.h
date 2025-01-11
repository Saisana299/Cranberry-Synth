#ifndef MIDI_HANDLER_H
#define MIDI_HANDLER_H

#include <Audio.h>
#include <MIDI.h>

#include <handlers/audio.h>

class MIDIHandler {
private:
    AudioHandler audio_hdl;
public:
    void init(AudioHandler &audio);    // 初期化
    void process(); // 処理
};

#endif