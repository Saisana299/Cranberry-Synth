#pragma once

#include <Arduino.h>
#include <atomic>

#include "tools/midi_player.hpp"
#include "utils/state.hpp"

// エンコーダーのピン
constexpr uint8_t ENC_A_PIN = 15; // GPIO6_IO19
constexpr uint8_t ENC_B_PIN = 17; // GPIO6_IO22

// タクトスイッチのピン
constexpr uint8_t SW_UP_PIN  = 37; // GPIO7_IO19
constexpr uint8_t SW_DN_PIN  = 36; // GPIO7_IO18
constexpr uint8_t SW_L_PIN   = 35; // GPIO7_IO28
constexpr uint8_t SW_R_PIN   = 34; // GPIO7_IO29
constexpr uint8_t SW_ENT_PIN = 22; // GPIO6_IO24
constexpr uint8_t SW_CXL_PIN = 14; // GPIO6_IO18
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
    volatile uint32_t* const gpio_psr;  // GPIO Port Status Register
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
    static const ButtonConfig BUTTON_CONFIGS[7];

    ButtonState button_states[7] = {};
    uint32_t last_encoder_debounce_time = 0;

    void process_button(size_t btn_idx, uint32_t now);

public:
    PhysicalHandler(State& state) : state_(state) {}

    void init();
    void process();
};