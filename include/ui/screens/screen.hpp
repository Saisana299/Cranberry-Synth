#pragma once

#include "display/gfx.hpp"
#include "ui/ui.hpp"
#include "utils/state.hpp"

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
protected:
    // 自身の画面からUIマネージャーの機能（画面遷移など）を呼び出すために使います。
    UIManager* manager = nullptr;
};