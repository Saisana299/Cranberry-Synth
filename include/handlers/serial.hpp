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

    inline void flushInput() {
        while(Serial.available() > 0) {
            Serial.read();
        }
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
        while (Serial.available() > 0) {
            int inByte = Serial.read();
            if (inByte < 0) break;

            command_buffer[command_index++] = static_cast<uint8_t>(inByte);

            if(command_index >= CMD_BUFFER_MAX) {
                flushInput();
                Serial.println("ERR: Command buffer overflow");
                command_index = 0;
                continue;
            }

            if(inByte == '\r') {
                continue;
            }

            if(inByte == '\n') {
                if(command_index > CMD_MIN_LENGTH) {
                    executeCommand(command_buffer, command_index);
                }
                command_index = 0;
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