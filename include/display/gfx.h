#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128
// MOSI1_PIN = 26
// SCK1_PIN  = 27
#define DC_PIN        40
#define CS_PIN        38
#define RST_PIN       41
class GFX_SSD1351 {
private:
    Adafruit_SSD1351 display;
    void init();

public:
    GFX_SSD1351(): display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI1, CS_PIN, DC_PIN, RST_PIN) {
        init();
    }
    void testdrawtext(char *text, uint16_t color);
};