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

    /**
     * @brief エンコーダー回転イベント（回転量 delta 付き）
     * デフォルト実装: |delta| 回分の BTN_L/BTN_R として handleInput に委譲
     * パラメータ画面はオーバーライドして効率的に処理可能
     */
    virtual void handleEncoder(int16_t delta) {
        uint8_t btn = (delta > 0) ? BTN_R : BTN_L;
        int16_t count = delta < 0 ? -delta : delta;
        if (count > 20) count = 20; // 過剰ループ防止
        for (int16_t i = 0; i < count; ++i) {
            handleInput(btn);
        }
    }

    virtual void draw(GFXcanvas16& canvas) = 0;
    virtual bool isAnimated() const { return false; }

protected:
    UIManager* manager = nullptr;
    uint8_t cursorPos = 0;
};