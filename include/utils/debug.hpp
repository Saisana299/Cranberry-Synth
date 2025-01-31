#pragma once

class Debug {
private:
    // static constexpr HardwareSerial& DebugSerial = Serial6;
    static constexpr usb_serial_class& DebugSerial = Serial;
    static inline bool initialized = false;

    static inline void init() {
        if (!initialized) {
            DebugSerial.begin(115200);
            initialized = true;
        }
    }

public:
    static inline void enable() {
        init();
    }

    static inline void println(const String& msg) {
        if(!initialized) return;
        DebugSerial.println(msg);
    }

    static inline void print(const String& msg) {
        if(!initialized) return;
        DebugSerial.print(msg);
    }
};