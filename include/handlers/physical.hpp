#pragma once

#include <Arduino.h>
#include <atomic>

#include "tools/midi_player.hpp"
#include "utils/state.hpp"

// エンコーダーのピン
constexpr uint8_t ENC_A_PIN = 15; // GPIO6_IO19
constexpr uint8_t ENC_B_PIN = 17; // GPIO6_IO22

// タクトスイッチのピン
constexpr uint8_t SW_UP_PIN  = 3; // GPIO9_IO5
constexpr uint8_t SW_DN_PIN  = 4; // GPIO9_IO6
constexpr uint8_t SW_L_PIN   = 5; // GPIO9_IO8
constexpr uint8_t SW_R_PIN   = 2; // GPIO9_IO4
constexpr uint8_t SW_ENT_PIN = 19; // GPIO6_IO16
constexpr uint8_t SW_CXL_PIN = 18; // GPIO6_IO17
constexpr uint8_t SW_ENC_PIN = 16; // GPIO6_IO23

// 押下判定までの時間
constexpr uint32_t TIME_DEBOUNCE    = 10;
constexpr uint32_t TIME_LONG_PRESS  = 1000;
constexpr uint32_t TIME_REPEAT_INTERVAL = 100;
constexpr uint32_t TIME_ENCODER_DEBOUNCE = 5;

// ピンマスク
constexpr uint32_t ENC_A_MASK = 1UL << 19;
constexpr uint32_t ENC_B_MASK = 1UL << 22;

struct ButtonConfig {
    const uint8_t pin;
    const uint8_t id_short;
    const uint8_t id_long;
    const bool active_high;
    const uint32_t pin_mask;
};

struct ButtonState {
    bool is_pressed = false;
    bool long_triggered = false;
    uint32_t press_start_time = 0;
    uint32_t last_repeat_time = 0;
};

struct EncoderState {
    uint8_t last_encoded = 0;
    uint32_t last_change_time = 0;
};

class PhysicalHandler {
private:
    State& state_;

    static volatile std::atomic<int32_t> encoder_raw_value;
    static volatile std::atomic<uint8_t> encoder_last_encoded;

    static void updateEncoderISR();

    /** @brief ボタンの状態を保持 */
    static constexpr ButtonConfig BUTTON_CONFIGS[7] = {
        {SW_UP_PIN,  BTN_UP,  BTN_UP_LONG,  false, 1UL << 5},
        {SW_DN_PIN,  BTN_DN,  BTN_DN_LONG,  false, 1UL << 6},
        {SW_L_PIN,   BTN_L,   BTN_L_LONG,   false, 1UL << 8},
        {SW_R_PIN,   BTN_R,   BTN_R_LONG,   false, 1UL << 4},
        {SW_ENT_PIN, BTN_ET,  BTN_ET_LONG,  false, 1UL << 16},
        {SW_CXL_PIN, BTN_CXL, BTN_CXL_LONG, false, 1UL << 17},
        {SW_ENC_PIN, BTN_ET,  BTN_ET_LONG,  true,  1UL << 23}  // エンコーダークリック = 決定ボタン
    };

    ButtonState button_states[7] = {};
    uint32_t last_encoder_debounce_time = 0;

    void process_button(size_t btn_idx, uint32_t now);

public:
    PhysicalHandler(State& state) : state_(state) {}

    void init();
    void process();
};