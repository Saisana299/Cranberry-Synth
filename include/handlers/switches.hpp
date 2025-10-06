#pragma once

#include <Arduino.h>

#include "handlers/file.hpp"
#include "utils/state.hpp"

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

    State& state_;

    void init();

    static void updateEncoder();

    struct Button {
        uint8_t pin;
        uint32_t pushCount;
        bool longPushed;
        uint8_t state;
        uint8_t stateLong;
    };
    Button buttons[7] = {
        {UP_PIN,  0, false, BTN_UP,  BTN_UP_LONG},
        {DN_PIN,  0, false, BTN_DN,  BTN_DN_LONG},
        {L_PIN,   0, false, BTN_L,   BTN_L_LONG},
        {R_PIN,   0, false, BTN_R,   BTN_R_LONG},
        {ET_PIN,  0, false, BTN_ET,  BTN_ET_LONG},
        {CXL_PIN, 0, false, BTN_CXL, BTN_CXL_LONG},
        {ECB_PIN, 0, false, BTN_EC,  BTN_EC_LONG}
    };

    uint32_t intervalCount = 0;
    int8_t playing = 0;

public:
    Switches(State& state) : state_(state) {
        init();
    }
    void process();
};