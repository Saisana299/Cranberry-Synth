#pragma once

#include "display/gfx.hpp"

constexpr uint8_t CMD_BUFFER_MAX = 64;
constexpr uint8_t CMD_MIN_LENGTH = 1;

class SerialHandler {
private:
    bool initialized = false;
    uint8_t command_buffer[CMD_BUFFER_MAX];
    uint8_t command_index = 0;

    void executeCommand(const uint8_t* cmd, uint8_t len);

    inline void resetBuffer() {
        command_index = 0;
    }

public:
    SerialHandler() {};

    void begin() {
        if (!initialized) {
            Serial.begin(115200);
            initialized = true;
        }
    }

    void process() {
        if(!initialized) return;

        // 1フレームあたりの処理バイト数を制限する
        int processed_count = 0;
        const int MAX_BYTES_PER_FRAME = 64;

        while (Serial.available() > 0 && processed_count < MAX_BYTES_PER_FRAME) {
            int inByte = Serial.read();
            processed_count++; // カウンタインクリメント

            if (inByte < 0) break;

            // バッファオーバーフロー対策
            if(command_index >= CMD_BUFFER_MAX) {
                Serial.println("ERR: Command buffer overflow");
                resetBuffer();
                continue;
            }

            // CR (\r) は無視
            if(inByte == '\r') {
                continue;
            }

            // LF (\n) でコマンド確定
            if(inByte == '\n') {
                if(command_index > CMD_MIN_LENGTH) {
                    executeCommand(command_buffer, command_index);
                }
                resetBuffer(); // 処理が終わったらリセット
            } else {
                // 通常の文字ならバッファに追加
                command_buffer[command_index++] = static_cast<uint8_t>(inByte);
            }
        }
    }

    template <typename... Args>
     void print(const char *format, Args... args) {
        if(initialized) Serial.printf(format, args...);
    }

    void println(const char* msg) {
        if(initialized) Serial.println(msg);
    }

    void println(const String& msg) {
        if(initialized) Serial.println(msg);
    }
};

extern SerialHandler serial_hdl;