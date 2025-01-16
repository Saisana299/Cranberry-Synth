#include "handlers/midi.h"

void MIDIHandler::pushNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
    if (is_enable_queue) {
        // if (note_on_queue.count() == NOTE_QUEUE_SIZE) {
        //     note_on_queue.pop();
        // }
        // note_on_queue.push(NoteQueue{note, velocity, channel});
        on = true;
    }
}

void MIDIHandler::pushNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
    if (is_enable_queue) {
        // if (note_off_queue.count() == NOTE_QUEUE_SIZE) {
        //     note_off_queue.pop();
        // }
        // note_off_queue.push(NoteQueue{note, velocity, channel});
        on = false;
    }
}

/**
 * @brief NOTE ON/OFF をキューに入れる機能の有効/無効
 * @param enable キューを有効にするか
 */
void MIDIHandler::queueMode(bool enable) {
    if(!enable) {
        // note_off_queue.clear();
        // note_on_queue.clear();
    }
    is_enable_queue = enable;
}

/**
 * @brief キューをリセットする
 * @param enable trueでキューを有効にする
 */
void MIDIHandler::queueReset(bool enable = false) {
    is_enable_queue = false;
    // note_off_queue.clear();
    // note_on_queue.clear();
    if(enable) {
        is_enable_queue = true;
    }
}

/**
 * @brief キューの状態を取得
 * @return true キュー有効
 * @return false キュー無効
 */
bool MIDIHandler::queueStatus() {
    return is_enable_queue;
}

void MIDIHandler::init() {
    is_enable_queue = false;
    MIDI.begin(MIDI_CHANNEL_OMNI);
}

/**
 * @brief UARTで受け取ったMIDIデータを処理
 */
void MIDIHandler::process() {
    uint8_t note, velocity, channel, d1, d2;
    if(MIDI.read()) {
        byte type = MIDI.getType();
        switch(type) {
            // ノートオン
            case midi::NoteOn:{
                note = MIDI.getData1();
                velocity = MIDI.getData2();
                channel = MIDI.getChannel();
                if (velocity > 0 && note <= 127) {
                    pushNoteOn(note, velocity, channel);
                    // Debug::println(String("Note On:  ch=") + channel + ", note=" + note + ", velocity=" + velocity);
                } else if(note <= 127) {
                    pushNoteOff(note, velocity, channel);
                    // Debug::println(String("Note Off: ch=") + channel + ", note=" + note);
                }
                break;
            }

            // ノートオフ
            case midi::NoteOff:{
                note = MIDI.getData1();
                velocity = MIDI.getData2();
                channel = MIDI.getChannel();
                if (note <= 127) {
                    pushNoteOff(note, velocity, channel);
                    // Debug::println(String("Note Off: ch=") + channel + ", note=" + note);
                }
                break;
            }

            // ピッチベンド
            case midi::PitchBend:{
                break;
            }

            // コントロールチェンジ
            case midi::ControlChange:{
                d1 = MIDI.getData1();
                d2 = MIDI.getData2();
                Debug::println(String("Control Change:") + d1 + " " + d2);
                break;
            }

            default:{
                break;
            }
        }
    }
}