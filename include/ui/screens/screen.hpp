#pragma once

class UIManager;

class Screen {
public:
    virtual ~Screen() = default;
    virtual void onEnter(UIManager* manager) {
        this->manager = manager;
    }
    virtual void onExit() {}
    virtual void handleInput(uint8_t button) = 0;
    virtual void draw(GFXcanvas16& canvas) = 0;
    virtual bool isAnimated() const { return false; }

protected:
    UIManager* manager = nullptr;
    uint8_t cursorPos = 0;
};