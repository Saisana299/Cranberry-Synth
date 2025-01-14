#ifndef DEBUG_H
#define DEBUG_H

class Debug {
private:
    static constexpr HardwareSerial& DebugSerial = Serial1;
    static bool initialized;

    static void initialize() {
        if (!initialized) {
            DebugSerial.begin(115200);
            initialized = true;
        }
    }

public:
    static void println(const String& msg) {
        initialize();
        DebugSerial.println(msg);
    }

    static void print(const String& msg) {
        initialize();
        DebugSerial.print(msg);
    }
};

bool Debug::initialized = false;

#endif