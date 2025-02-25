#pragma once

#include "display/gfx.hpp"

class SerialHandler {
private:
    static inline bool initialized = false;
    static inline uint8_t command_buffer[3];
    static inline uint8_t command_index = 0;
    static inline bool command_ready = false;

    static inline void init() {
        if (!initialized) {
            Serial.begin(115200);
            initialized = true;
        }
    }

public:
    SerialHandler() {
        init();
    }

    inline void process() {
        if(!initialized) return;
        while (Serial.available() > 0) {
            uint8_t data = Serial.read();
            command_buffer[command_index] = data;
            command_index++;

            if (command_index >= 3) {
                command_ready = true;
                command_index = 0;
            } else {
                command_ready = false;
            }
        }

        if (command_ready) {
            command_ready = false;
        }
    }

    static inline void println(const String& msg) {
        if(!initialized) return;
        Serial.println(msg);
    }

    static inline void print(const String& msg) {
        if(!initialized) return;
        Serial.print(msg);
    }
};