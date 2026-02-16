#pragma once

#include <vector>
#include <string>

#include "ui/ui.hpp"
#include "tools/midi_player.hpp"

class MIDIPlayerScreen : public Screen {
private:
    static constexpr int16_t HEADER_H = 12;
    static constexpr int16_t STATUS_H = 24;  // 再生状態表示エリア
    static constexpr int16_t LIST_Y   = HEADER_H + 1 + STATUS_H + 3;  // ファイルリスト開始Y (余白+区切り線)
    static constexpr int16_t ITEM_H   = 12;
    static constexpr int16_t FOOTER_Y = SCREEN_HEIGHT - 12;
    static constexpr int16_t LIST_H   = FOOTER_Y - LIST_Y;
    static constexpr int16_t VISIBLE_ITEMS = LIST_H / ITEM_H;

    enum CursorZone {
        ZONE_STOP = 0,  // 停止ボタン（ステータスエリア内）
        ZONE_LIST,      // ファイルリスト
        ZONE_BACK,      // 戻るボタン
    };

    CursorZone zone = ZONE_LIST;
    int8_t listIndex = 0;   // ファイルリスト内カーソル
    int8_t scrollTop = 0;   // スクロールオフセット

    std::vector<std::string> files;
    bool needsFullRedraw = true;

    // ファイル名からディレクトリ部分を除去して表示名を返す
    const char* displayName(const std::string& path) {
        size_t pos = path.rfind('/');
        if (pos != std::string::npos && pos + 1 < path.size()) {
            return path.c_str() + pos + 1;
        }
        return path.c_str();
    }

public:
    MIDIPlayerScreen() = default;

    void onEnter(UIManager* manager) override {
        this->manager = manager;
        files = MIDIPlayer::listFiles("/");
        if (files.empty()) {
            listIndex = 0;
        } else {
            listIndex = 0;
        }
        zone = files.empty() ? ZONE_BACK : ZONE_LIST;
        scrollTop = 0;
        needsFullRedraw = true;
        manager->invalidate();
        manager->triggerFullTransfer();
    }

    bool isAnimated() const override {
        // 再生中はステータスを更新する
        return MIDIPlayer::isPlaying();
    }

    void handleInput(uint8_t button) override {
        bool moved = false;

        // --- 上下移動 ---
        if (button == BTN_UP || button == BTN_UP_LONG) {
            if (zone == ZONE_LIST) {
                if (listIndex > 0) {
                    listIndex--;
                    if (listIndex < scrollTop) scrollTop = listIndex;
                } else {
                    zone = ZONE_STOP;
                }
            } else if (zone == ZONE_BACK) {
                if (!files.empty()) {
                    zone = ZONE_LIST;
                    listIndex = (int8_t)(files.size() - 1);
                    // スクロール調整
                    if (listIndex >= scrollTop + VISIBLE_ITEMS) {
                        scrollTop = listIndex - VISIBLE_ITEMS + 1;
                    }
                } else {
                    zone = ZONE_STOP;
                }
            } else if (zone == ZONE_STOP) {
                zone = ZONE_BACK;
            }
            moved = true;
        }
        else if (button == BTN_DN || button == BTN_DN_LONG) {
            if (zone == ZONE_STOP) {
                if (!files.empty()) {
                    zone = ZONE_LIST;
                    listIndex = 0;
                    scrollTop = 0;
                } else {
                    zone = ZONE_BACK;
                }
            } else if (zone == ZONE_LIST) {
                if (listIndex < (int8_t)(files.size() - 1)) {
                    listIndex++;
                    if (listIndex >= scrollTop + VISIBLE_ITEMS) {
                        scrollTop = listIndex - VISIBLE_ITEMS + 1;
                    }
                } else {
                    zone = ZONE_BACK;
                }
            } else if (zone == ZONE_BACK) {
                zone = ZONE_STOP;
            }
            moved = true;
        }

        // --- 決定 ---
        else if (button == BTN_ET) {
            if (zone == ZONE_LIST && !files.empty()) {
                // 選択されたファイルを再生
                MIDIPlayer::stop();
                MIDIPlayer::play(files[listIndex].c_str());
                needsFullRedraw = true;
            }
            else if (zone == ZONE_STOP) {
                MIDIPlayer::stop();
                needsFullRedraw = true;
            }
            else if (zone == ZONE_BACK) {
                manager->popScreen();
                return;
            }
        }

        // --- キャンセル ---
        else if (button == BTN_CXL) {
            manager->popScreen();
            return;
        }

        if (moved) {
            needsFullRedraw = true;
            manager->invalidate();
        }
    }

    void draw(GFXcanvas16& canvas) override {
        if (needsFullRedraw) {
            canvas.fillScreen(Color::BLACK);
            drawHeader(canvas);
            drawStatus(canvas);
            drawFileList(canvas);
            drawFooter(canvas);
            needsFullRedraw = false;
            manager->triggerFullTransfer();
            return;
        }

        // アニメーション時はステータスのみ更新
        if (MIDIPlayer::isPlaying()) {
            drawStatus(canvas);
            manager->transferPartial(0, HEADER_H + 1, SCREEN_WIDTH, STATUS_H);
        }
    }

private:
    void drawHeader(GFXcanvas16& canvas) {
        canvas.fillRect(0, 0, SCREEN_WIDTH, HEADER_H, Color::BLACK);
        canvas.setTextSize(1);
        canvas.setTextColor(Color::WHITE);
        canvas.setCursor(2, 2);
        canvas.print("MIDI PLAYER");
        canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::WHITE);
    }

    void drawStatus(GFXcanvas16& canvas) {
        int16_t y = HEADER_H + 1;
        canvas.fillRect(0, y, SCREEN_WIDTH, STATUS_H, Color::BLACK);

        bool playing = MIDIPlayer::isPlaying();
        bool stopSelected = (zone == ZONE_STOP);

        // STOPボタン
        int16_t btnX = 2;
        int16_t btnY = y + 2;
        int16_t btnW = 28;
        int16_t btnH = 10;

        if (stopSelected) {
            canvas.drawRect(btnX, btnY, btnW, btnH, Color::WHITE);
        }
        canvas.setTextSize(1);
        canvas.setTextColor(stopSelected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(btnX + 3, btnY + 1);
        canvas.print("STOP");

        // 再生ステータスアイコン + ファイル名
        int16_t infoY = y + 13;
        canvas.setTextSize(1);

        if (playing) {
            // ▶ マーク
            canvas.fillTriangle(3, infoY, 3, infoY + 6, 8, infoY + 3, Color::MD_GREEN);

            // ファイル名表示
            const char* fname = MIDIPlayer::getFilename();
            if (fname) {
                // ファイル名からパスを除去
                const char* base = fname;
                const char* p = fname;
                while (*p) {
                    if (*p == '/') base = p + 1;
                    p++;
                }
                canvas.setTextColor(Color::WHITE);
                canvas.setCursor(12, infoY);
                canvas.print(base);
            }
        } else {
            // ■ マーク
            canvas.fillRect(3, infoY, 6, 6, Color::MD_GRAY);
            canvas.setTextColor(Color::MD_GRAY);
            canvas.setCursor(12, infoY);
            canvas.print("Stopped");
        }

        canvas.drawFastHLine(0, y + STATUS_H, SCREEN_WIDTH, Color::DARK_SLATE);
    }

    void drawFileList(GFXcanvas16& canvas) {
        int16_t y = LIST_Y;

        if (files.empty()) {
            canvas.setTextSize(1);
            canvas.setTextColor(Color::MD_GRAY);
            canvas.setCursor(10, y + 10);
            canvas.print("No .mid files");
            return;
        }

        int16_t end = scrollTop + VISIBLE_ITEMS;
        if (end > (int16_t)files.size()) end = (int16_t)files.size();

        for (int i = scrollTop; i < end; i++) {
            int16_t itemY = y + (i - scrollTop) * ITEM_H;
            bool selected = (zone == ZONE_LIST && i == listIndex);

            canvas.fillRect(0, itemY, SCREEN_WIDTH, ITEM_H, Color::BLACK);
            canvas.setTextSize(1);

            if (selected) {
                canvas.fillRect(2, itemY + 2, 3, 8, Color::WHITE);
            }

            canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
            canvas.setCursor(8, itemY + 2);
            canvas.print(displayName(files[i]));
        }

        // スクロールインジケーター
        if ((int16_t)files.size() > VISIBLE_ITEMS) {
            int16_t barH = LIST_H;
            int16_t thumbH = max((int16_t)4, (int16_t)(barH * VISIBLE_ITEMS / (int16_t)files.size()));
            int16_t thumbY = LIST_Y + (int16_t)((barH - thumbH) * scrollTop / ((int16_t)files.size() - VISIBLE_ITEMS));
            canvas.fillRect(SCREEN_WIDTH - 2, LIST_Y, 2, barH, Color::BLACK);
            canvas.fillRect(SCREEN_WIDTH - 2, thumbY, 2, thumbH, Color::DARK_SLATE);
        }
    }

    void drawFooter(GFXcanvas16& canvas) {
        canvas.drawFastHLine(0, FOOTER_Y, SCREEN_WIDTH, Color::WHITE);
        drawBackButton(canvas, zone == ZONE_BACK);
    }

    void drawBackButton(GFXcanvas16& canvas, bool selected) {
        int16_t x = 2;
        int16_t y = FOOTER_Y + 2;
        int16_t w = 24;
        int16_t h = 10;

        canvas.fillRect(x, y, w, h, Color::BLACK);

        if (selected) {
            canvas.drawRect(x, y, w, h, Color::WHITE);
        }

        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(x + 2, y + 1);
        canvas.print("<");

        manager->transferPartial(x, y, w, h);
    }
};
