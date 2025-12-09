#include "handlers/serial.hpp"

SerialHandler serial_hdl;

void SerialHandler::executeCommand(const uint8_t* cmd, uint8_t len) {
    // コマンド解析例
    Serial.printf("RX: ");
    for(uint8_t i = 0; i < len; ++i) {
        Serial.printf("%02X ", cmd[i]);
    }
    Serial.println();

    // TODO: 実際のコマンド処理を実装
    switch(cmd[0]) {
        case 'A': case 'a':
            Serial.println("Command A received");
            break;
        case 'B': case 'b':
            Serial.println("Command B received");
            break;
        default:
            Serial.printf("Unknown command: %c\n", (char)cmd[0]);
            break;
    }
}