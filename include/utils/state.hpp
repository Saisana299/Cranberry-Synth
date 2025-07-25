#pragma once

/* Synth Mode */
constexpr uint8_t MODE_SYNTH = 0x00;

/* Button State */
constexpr uint8_t BTN_NONE = 0x00;
constexpr uint8_t BTN_UP   = 0x01;
constexpr uint8_t BTN_DN   = 0x02;
constexpr uint8_t BTN_L    = 0x03;
constexpr uint8_t BTN_R    = 0x04;
constexpr uint8_t BTN_ET   = 0x05;
constexpr uint8_t BTN_CXL  = 0x06;
constexpr uint8_t BTN_EC   = 0x07;
constexpr uint8_t BTN_EC_L = 0x08;
constexpr uint8_t BTN_EC_R = 0x09;

/* Button State (Long) */
constexpr uint8_t BTN_UP_LONG  = 0x11;
constexpr uint8_t BTN_DN_LONG  = 0x12;
constexpr uint8_t BTN_L_LONG   = 0x13;
constexpr uint8_t BTN_R_LONG   = 0x14;
constexpr uint8_t BTN_ET_LONG  = 0x15;
constexpr uint8_t BTN_CXL_LONG = 0x16;
constexpr uint8_t BTN_EC_LONG  = 0x17;

class State {
public:
    static inline bool led_midi = false;
    static inline bool led_audio = false;
    static inline uint8_t mode_state = MODE_SYNTH;
    static inline uint8_t btn_state = BTN_NONE;
};