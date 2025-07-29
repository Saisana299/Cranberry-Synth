#pragma once

#include <vector>
#include <string>
#include <functional>
#include "ui/ui.hpp"
#include "utils/state.hpp"

// メニューを表示するための具体的な画面クラスです。
class MenuScreen : public Screen {
public:
    // メニュー項目が選択された時に実行される処理を、関数オブジェクトとして定義します。
    // これにより、各項目に柔軟な動作（ラムダ式など）を割り当てられます。
    using MenuItemAction = std::function<void(UIManager*)>;

    struct MenuItem {
        std::string label;
        MenuItemAction action;
    };

    // コンストラクタ：メニューのタイトルを受け取ります。
    MenuScreen(std::string title) : title(std::move(title)) {}

    // メニュー項目を追加します。
    void addItem(std::string label, MenuItemAction action) {
        items.push_back({std::move(label), action});
    }

    void handleInput(uint8_t button) override {
        if (button == BTN_DN) {
            cursorPos = (cursorPos + 1) % items.size();
        } else if (button == BTN_UP) {
            cursorPos = (cursorPos + items.size() - 1) % items.size();
        } else if (button == BTN_ET) {
            // 選択された項目のアクションを実行
            if (!items.empty()) {
                items[cursorPos].action(manager);
            }
        } else if (button == BTN_CXL) {
            // メニューを閉じる
            manager->popScreen();
        }
    }

    void draw(GFXcanvas16& canvas) override {
        canvas.fillScreen(Color::BLACK);
        canvas.setTextSize(1);

        // タイトルを描画
        canvas.setCursor(2, 2);
        canvas.setTextColor(Color::YELLOW);
        canvas.print(title.c_str());
        canvas.drawFastHLine(0, 12, canvas.width(), Color::GRAY);

        // メニュー項目を描画
        for (int i = 0; i < items.size(); ++i) {
            if (i == cursorPos) {
                // カーソルが当たっている項目は反転表示
                canvas.fillRect(0, 16 + i * 10, canvas.width(), 10, Color::WHITE);
                canvas.setTextColor(Color::BLACK);
                canvas.setCursor(4, 17 + i * 10);
            } else {
                canvas.fillRect(0, 16 + i * 10, canvas.width(), 10, Color::BLACK);
                canvas.setTextColor(Color::WHITE);
                canvas.setCursor(4, 17 + i * 10);
            }
            canvas.print(items[i].label.c_str());
        }
    }

private:
    std::string title;
    std::vector<MenuItem> items;
    int cursorPos = 0;
};