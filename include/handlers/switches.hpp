#pragma once

#include <Arduino.h>

#include "handlers/file.hpp"

constexpr uint8_t ECB_PIN = 16;
constexpr uint8_t A_PIN = 15;
constexpr uint8_t B_PIN = 17;

constexpr uint8_t UP_PIN  = 3;
constexpr uint8_t DN_PIN  = 4;
constexpr uint8_t L_PIN   = 5;
constexpr uint8_t R_PIN   = 2;
constexpr uint8_t ET_PIN  = 19;
constexpr uint8_t CXL_PIN = 18;

constexpr uint32_t PUSH_SHORT = 200;
constexpr uint32_t PUSH_LONG  = 65000;

class Switches {
private:
    static volatile uint8_t lastEncoded;

    void init();

    static void updateEncoder();

    struct Button {
        uint8_t pin;
        uint32_t pushCount;
        bool longPushed;
    };
    Button buttons[7] = {
        {UP_PIN,  0, false},
        {DN_PIN,  0, false},
        {L_PIN,   0, false},
        {R_PIN,   0, false},
        {ET_PIN,  0, false},
        {CXL_PIN, 0, false},
        {ECB_PIN, 0, false}
    };

    uint32_t intervalCount = 0;
    int8_t playing = 0;

public:
    Switches() {
        init();
    }
    void process();
};