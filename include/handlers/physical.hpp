#pragma once

#include <Arduino.h>

#include "tools/midi_player.hpp"
#include "utils/state.hpp"

// エンコーダーのピン
constexpr uint8_t ENC_A_PIN = 15;
constexpr uint8_t ENC_B_PIN = 17;

// タクトスイッチのピン
constexpr uint8_t SW_UP_PIN  = 3;
constexpr uint8_t SW_DN_PIN  = 4;
constexpr uint8_t SW_L_PIN   = 5;
constexpr uint8_t SW_R_PIN   = 2;
constexpr uint8_t SW_ENT_PIN = 19;
constexpr uint8_t SW_CXL_PIN = 18;
constexpr uint8_t SW_ENC_PIN = 16;

// 押下判定までの時間
constexpr uint32_t TIME_DEBOUNCE    = 10;
constexpr uint32_t TIME_LONG_PRESS  = 1000;

class PhysicalHandler {
private:
    State& state_;

    static volatile int32_t encoder_value;
    static volatile uint8_t last_encoded;

    static void updateEncoderISR();

    /** @brief ボタンの状態を保持 */
    struct Button {
        const uint8_t pin;        // ピン番号
        const uint8_t id_short;
        const uint8_t id_long;

        bool is_pressed = false;
        bool long_triggered = false;
        uint32_t press_start_time = 0;
    };
    Button buttons[7] = {
        {SW_UP_PIN,  BTN_UP,  BTN_UP_LONG},
        {SW_DN_PIN,  BTN_DN,  BTN_DN_LONG},
        {SW_L_PIN,   BTN_L,   BTN_L_LONG},
        {SW_R_PIN,   BTN_R,   BTN_R_LONG},
        {SW_ENT_PIN, BTN_ET,  BTN_ET_LONG},
        {SW_CXL_PIN, BTN_CXL, BTN_CXL_LONG},
        {SW_ENC_PIN, BTN_EC,  BTN_EC_LONG}
    };

public:
    PhysicalHandler(State& state) : state_(state) {}

    void init();

    void process();
};