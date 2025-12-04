#pragma once

#include "display/gfx.hpp"

class SerialHandler {
private:
    bool initialized = false;
    uint8_t command_buffer[3];
    uint8_t command_index = 0;

    void executeCommand() {
        Serial.printf("CMD: %02X %02X %02X\n",
            command_buffer[0], command_buffer[1], command_buffer[2]);
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

            command_buffer[command_index] = static_cast<uint8_t>(inByte);
            command_index++;

            if(command_index >= 3) {
                executeCommand();
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
        if(initialized) Serial.print(msg);
    }
};

extern SerialHandler serial_hdl;