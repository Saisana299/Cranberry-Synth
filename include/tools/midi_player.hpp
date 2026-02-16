#pragma once

#include <MD_MIDIFile.h>
#include <SD.h>
#include <vector>
#include <string>

#include "modules/synth.hpp"
#include "utils/state.hpp"

class MIDIPlayer {
private:
    MD_MIDIFile SMF = {};

    static inline void midiCallbackStatic(midi_event *pev);

    static inline MIDIPlayer* instance = nullptr;

    bool is_initialized = false;
    bool is_playing = false;
    std::string current_filename;  // 再生中のファイル名（独自コピー）

    State& state_;
    Synth& synth_;

    void init();
    void midiCallback(midi_event *pev);

public:
    MIDIPlayer(State& state) : state_(state), synth_(Synth::getInstance()) {
        if (instance == nullptr) {
            instance = this;
        }
        init();
    }
    ~MIDIPlayer() {
        if (instance == this) instance = nullptr;
    }

    void process();
    static void play(const char* path);
    static void stop();

    /// 再生中かどうか
    static bool isPlaying() {
        return instance && instance->is_playing;
    }

    /// 現在のファイル名を取得
    static const char* getFilename() {
        if (instance && instance->is_playing && !instance->current_filename.empty()) {
            return instance->current_filename.c_str();
        }
        return nullptr;
    }

    /// SDカードから .mid ファイル一覧を取得
    static std::vector<std::string> listFiles(const char* dir = "/") {
        std::vector<std::string> files;
        if (!instance || !instance->is_initialized) return files;

        SdFile root;
        if (!root.open(dir)) return files;

        SdFile entry;
        while (entry.openNext(&root, O_RDONLY)) {
            if (!entry.isDir()) {
                char name[64];
                entry.getName(name, sizeof(name));
                size_t len = strlen(name);
                if (len > 4) {
                    const char* ext = name + len - 4;
                    if (strcasecmp(ext, ".mid") == 0 || strcasecmp(ext, ".MID") == 0) {
                        // dir が "/" の場合はそのまま、それ以外はパスを結合
                        std::string path;
                        if (strcmp(dir, "/") == 0) {
                            path = std::string("/") + name;
                        } else {
                            path = std::string(dir) + "/" + name;
                        }
                        files.push_back(path);
                    }
                }
            }
            entry.close();
        }
        root.close();
        return files;
    }
};