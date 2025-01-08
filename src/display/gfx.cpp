#include <display/gfx.h>

GFX_SSD1351::GFX_SSD1351():
    display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI1, CS_PIN, DC_PIN, RST_PIN) {
}

void GFX_SSD1351::init() {
    display.begin();
}