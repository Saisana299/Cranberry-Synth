#pragma once
#include <cstdint>

class Color {
public:
    // ==========================================
    //  Basic / Primary Colors (原色・基本色)
    //  LEDの点灯テストや警告色として明確な色
    // ==========================================
    static constexpr uint16_t BLACK       = 0x0000;
    static constexpr uint16_t WHITE       = 0xFFFF;
    static constexpr uint16_t RED         = 0xF800;
    static constexpr uint16_t GREEN       = 0x07E0;
    static constexpr uint16_t BLUE        = 0x001F;
    static constexpr uint16_t YELLOW      = 0xFFE0;
    static constexpr uint16_t CYAN        = 0x07FF;
    static constexpr uint16_t MAGENTA     = 0xF81F;
    static constexpr uint16_t CRANBERRY   = 0xD800;

    // ==========================================
    //  Modern UI Colors (現代的な配色のチョイス)
    //  フラットデザインやマテリアルデザイン風の
    //  目に優しく、視認性の高い色合い
    // ==========================================

    // Modern Gray Scale
    static constexpr uint16_t OFF_WHITE   = 0xF7BE; // #F5F5F5
    static constexpr uint16_t MD_GRAY     = 0x7BEF; // #7B7D7B
    static constexpr uint16_t DARK_SLATE  = 0x2945; // #282C34
    static constexpr uint16_t CHARCOAL    = 0x18C3; // #181818

    // Modern Accents (彩度を抑えつつ鮮やかな色)
    static constexpr uint16_t MD_RED      = 0xE243; // #E74C3C
    static constexpr uint16_t MD_ORANGE   = 0xEB64; // #E67E22
    static constexpr uint16_t MD_YELLOW   = 0xFED0; // #F1C40F
    static constexpr uint16_t MD_GREEN    = 0x2D70; // #2ECC71
    static constexpr uint16_t MD_BLUE     = 0x349B; // #3498DB
    static constexpr uint16_t MD_PURPLE   = 0x9CD9; // #9B59B6
    static constexpr uint16_t MD_TEAL     = 0x1CC8; // #1ABC9C

    // Soft / Pastel Tones (背景や補助色用)
    static constexpr uint16_t SKY_BLUE    = 0x867F; // #87CEFA
    static constexpr uint16_t SALMON      = 0xFC0E; // #FA8072
    static constexpr uint16_t CREAM       = 0xFFFA; // #FFFDD0
    static constexpr uint16_t LAVENDER    = 0xE73F; // #E6E6FA
    static constexpr uint16_t BEIGE       = 0xF7BB; // #F5F5DC
    static constexpr uint16_t GOLD        = 0xFEA0; // #FFD700
};