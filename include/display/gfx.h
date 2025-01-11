#ifndef GFX_SSD1351_H
#define GFX_SSD1351_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>

class GFX_SSD1351 {
private:
    Adafruit_SSD1351 display;

    static constexpr int16_t SCREEN_WIDTH  = 128;
    static constexpr int16_t SCREEN_HEIGHT = 128;
    static constexpr int8_t DC_PIN  = 40;
    static constexpr int8_t CS_PIN  = 38;
    static constexpr int8_t RST_PIN = 41;

    // MOSI1_PIN = 26
    // SCK1_PIN  = 27

public:
    GFX_SSD1351();
    void init();    // 初期化
    void testdrawtext(char *text, uint16_t color);
};

#endif