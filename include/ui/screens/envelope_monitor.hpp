#pragma once

#include "ui/ui.hpp"
#include "modules/synth.hpp"
#include "utils/algorithm.hpp"

/**
 * @brief エンベロープモニター画面
 *
 * 6つのオペレーターのエンベロープ進行状況をリアルタイムで表示する。
 * 3列×2行のグリッドに、各OPのエンベロープ形状（R/Lパラメータから描画）と
 * 現在のレベル（ドット＋レベルバー）を表示する。
 *
 * キャリアはシアン、モジュレーターはイエローで色分け。
 * アクティブなフェーズのセグメントはハイライト表示。
 */
class EnvelopeMonitorScreen : public Screen {
private:
    // === レイアウト定数 ===
    static constexpr int16_t HEADER_H = 12;
    static constexpr int16_t COLS = 3;
    static constexpr int16_t ROWS = 2;
    static constexpr int16_t CELL_GAP = 1;
    static constexpr int16_t CELL_W = (SCREEN_WIDTH - (COLS - 1) * CELL_GAP) / COLS;  // 42
    static constexpr int16_t CELL_H = (SCREEN_HEIGHT - HEADER_H - (ROWS - 1) * CELL_GAP) / ROWS; // 57

    // セル内のレイアウト
    static constexpr int16_t LABEL_H = 9;     // OPラベル行
    static constexpr int16_t GRAPH_H = 34;    // エンベロープ曲線エリア
    static constexpr int16_t BAR_H = 5;       // レベルバー
    static constexpr int16_t BAR_GAP = 2;     // グラフとバーの間

    // ノート名テーブル
    static constexpr const char* NOTE_NAMES[12] = {
        "C", "C#", "D", "D#", "E", "F",
        "F#", "G", "G#", "A", "A#", "B"
    };

public:
    EnvelopeMonitorScreen() = default;

    void onEnter(UIManager* manager) override {
        this->manager = manager;
        manager->invalidate();
        manager->triggerFullTransfer();
    }

    bool isAnimated() const override { return true; }

    void handleInput(uint8_t button) override {
        if (button == BTN_CXL || button == BTN_ET) {
            manager->popScreen();
            return;
        }
    }

    void draw(GFXcanvas16& canvas) override {
        canvas.fillScreen(Color::BLACK);

        Synth& synth = Synth::getInstance();

        // アルゴリズム情報（キャリア判定用）
        uint8_t algo_id = synth.getCurrentAlgorithmId();
        uint8_t output_mask = Algorithms::get(algo_id).output_mask;

        // エンベロープデータ取得
        Synth::EnvMonitorInfo info;
        synth.getEnvMonitorInfo(info);

        // ヘッダー描画
        drawHeader(canvas, info);

        // 6オペレーターのセルを描画
        for (uint8_t op = 0; op < MAX_OPERATORS; ++op) {
            uint8_t col = op % COLS;
            uint8_t row = op / COLS;
            int16_t cx = col * (CELL_W + CELL_GAP);
            int16_t cy = HEADER_H + row * (CELL_H + CELL_GAP);

            bool is_carrier = (output_mask & (1 << op)) != 0;

            drawOperatorCell(canvas, cx, cy, op,
                             synth.getOperatorEnv(op),
                             info.levels[op], info.states[op],
                             is_carrier,
                             synth.getOperatorOsc(op).isEnabled());
        }

        manager->triggerFullTransfer();
    }

private:
    // === ヘッダー描画 ===
    void drawHeader(GFXcanvas16& canvas, const Synth::EnvMonitorInfo& info) {
        canvas.setTextSize(1);
        canvas.setTextColor(Color::WHITE);
        canvas.setCursor(2, 2);
        canvas.print("ENV MONITOR");

        // 現在のノート表示
        if (info.note < 128) {
            char buf[12];
            int8_t octave = static_cast<int8_t>(info.note / 12) - 1;
            sprintf(buf, "%s%d v%d", NOTE_NAMES[info.note % 12], octave, info.velocity);
            // 右寄せ
            int16_t tw = strlen(buf) * 6;
            canvas.setCursor(SCREEN_WIDTH - tw - 2, 2);
            canvas.setTextColor(Color::MD_TEAL);
            canvas.print(buf);
        }

        canvas.drawFastHLine(0, HEADER_H - 1, SCREEN_WIDTH, Color::DARK_SLATE);
    }

    // === オペレーターセル描画 ===
    void drawOperatorCell(GFXcanvas16& canvas,
                          int16_t x, int16_t y,
                          uint8_t op_index,
                          const Envelope& env,
                          EnvGain_t currentLevel,
                          Envelope::EnvelopeState state,
                          bool is_carrier,
                          bool is_enabled) {
        // セル枠線
        canvas.drawRect(x, y, CELL_W, CELL_H, Color::DARK_SLATE);

        // 無効なオペレーターはグレーアウト
        uint16_t accentColor = is_carrier ? Color::CYAN : Color::MD_YELLOW;
        uint16_t dimColor    = Color::DARK_SLATE;

        if (!is_enabled) {
            accentColor = Color::CHARCOAL;
            dimColor = Color::CHARCOAL;
        }

        // --- OPラベル ---
        canvas.setTextSize(1);
        canvas.setTextColor(accentColor);
        canvas.setCursor(x + 2, y + 1);
        canvas.print("OP");
        canvas.print(op_index + 1);

        // フェーズ状態テキスト
        canvas.setCursor(x + CELL_W - 14, y + 1);
        canvas.setTextColor(is_enabled ? Color::WHITE : Color::CHARCOAL);
        switch (state) {
            case Envelope::EnvelopeState::Phase1: canvas.print("A");  break;
            case Envelope::EnvelopeState::Phase2: canvas.print("D1"); break;
            case Envelope::EnvelopeState::Phase3: canvas.print("D2"); break;
            case Envelope::EnvelopeState::Phase4: canvas.print("R");  break;
            default:                              canvas.print("-");  break;
        }

        // --- エンベロープ形状グラフ ---
        int16_t gx = x + 2;                    // グラフ左端
        int16_t gy = y + LABEL_H + 1;           // グラフ上端
        int16_t gw = CELL_W - 4;               // グラフ幅
        int16_t gh = GRAPH_H;                   // グラフ高さ

        // エンベロープパラメータ取得
        uint8_t r1 = env.getRate1(), r2 = env.getRate2();
        uint8_t r3 = env.getRate3(), r4 = env.getRate4();
        uint8_t l1 = env.getLevel1(), l2 = env.getLevel2();
        uint8_t l3 = env.getLevel3(), l4 = env.getLevel4();

        // セグメント幅の計算（レートに反比例, 最小幅保証）
        // 高いレート = 速い = 短いセグメント
        int16_t w[4];
        {
            float weights[4] = {
                101.0f - r1, 101.0f - r2, 101.0f - r3, 101.0f - r4
            };
            float total = weights[0] + weights[1] + weights[2] + weights[3];
            int16_t used = 0;
            for (int i = 0; i < 3; ++i) {
                w[i] = std::max<int16_t>(3, static_cast<int16_t>(gw * weights[i] / total));
                used += w[i];
            }
            w[3] = gw - used;  // 残り
            if (w[3] < 3) w[3] = 3;
        }

        // X座標
        int16_t xp[5];
        xp[0] = gx;
        xp[1] = xp[0] + w[0];
        xp[2] = xp[1] + w[1];
        xp[3] = xp[2] + w[2];
        xp[4] = gx + gw - 1;

        // Y座標（Level 99 = 上端, 0 = 下端）
        auto levelToY = [gy, gh](uint8_t level) -> int16_t {
            return gy + gh - 1 - static_cast<int16_t>(level * (gh - 1) / 99);
        };

        int16_t yp[5];
        yp[0] = levelToY(0);   // 開始点（レベル0）
        yp[1] = levelToY(l1);  // L1 (Attack到達)
        yp[2] = levelToY(l2);  // L2 (Decay1到達)
        yp[3] = levelToY(l3);  // L3 (Sustain)
        yp[4] = levelToY(l4);  // L4 (Release到達)

        // エンベロープ形状描画（各セグメント、アクティブフェーズをハイライト）
        drawEnvSegment(canvas, xp[0], yp[0], xp[1], yp[1],
                       state == Envelope::EnvelopeState::Phase1, accentColor, dimColor);
        drawEnvSegment(canvas, xp[1], yp[1], xp[2], yp[2],
                       state == Envelope::EnvelopeState::Phase2, accentColor, dimColor);
        drawEnvSegment(canvas, xp[2], yp[2], xp[3], yp[3],
                       state == Envelope::EnvelopeState::Phase3, accentColor, dimColor);
        drawEnvSegment(canvas, xp[3], yp[3], xp[4], yp[4],
                       state == Envelope::EnvelopeState::Phase4, accentColor, dimColor);

        // --- 現在レベルマーカー（ドット）---
        if (is_enabled && currentLevel > 0) {
            // Q24 → ピクセルY
            int32_t h_px = static_cast<int32_t>(
                (static_cast<int64_t>(currentLevel) * (gh - 1)) >> 24
            );
            if (h_px > gh - 1) h_px = gh - 1;
            int16_t levelY = gy + gh - 1 - static_cast<int16_t>(h_px);

            // 現在のフェーズに応じてX位置を推定
            int16_t dotX = estimateDotX(state, currentLevel, xp, l1, l2, l3, l4);

            // ドット描画（3×3のクロス）
            canvas.drawPixel(dotX, levelY, Color::WHITE);
            canvas.drawPixel(dotX - 1, levelY, Color::WHITE);
            canvas.drawPixel(dotX + 1, levelY, Color::WHITE);
            canvas.drawPixel(dotX, levelY - 1, Color::WHITE);
            canvas.drawPixel(dotX, levelY + 1, Color::WHITE);
        }

        // --- レベルバー ---
        int16_t bx = x + 2;
        int16_t by = y + LABEL_H + GRAPH_H + BAR_GAP + 1;
        int16_t bw = CELL_W - 4;

        canvas.drawRect(bx, by, bw, BAR_H, dimColor);

        if (is_enabled && currentLevel > 0) {
            int16_t fillW = static_cast<int16_t>(
                (static_cast<int64_t>(currentLevel) * (bw - 2)) >> 24
            );
            if (fillW < 1) fillW = 1;
            if (fillW > bw - 2) fillW = bw - 2;
            canvas.fillRect(bx + 1, by + 1, fillW, BAR_H - 2, accentColor);
        }
    }

    // === エンベロープセグメント描画 ===
    void drawEnvSegment(GFXcanvas16& canvas,
                        int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                        bool active, uint16_t accentColor, uint16_t dimColor) {
        uint16_t color = active ? accentColor : dimColor;
        canvas.drawLine(x0, y0, x1, y1, color);
    }

    // === ドットX位置の推定 ===
    // 現在のフェーズと現在レベルから、エンベロープ曲線上のX位置を推定
    int16_t estimateDotX(Envelope::EnvelopeState state,
                         EnvGain_t currentLevel,
                         const int16_t xp[5],
                         uint8_t l1, uint8_t l2, uint8_t l3, uint8_t l4) {
        // currentLevel (Q24) をパーセント (0-99相当) に変換
        int32_t curPct = static_cast<int32_t>(
            (static_cast<int64_t>(currentLevel) * 99) >> 24
        );

        switch (state) {
            case Envelope::EnvelopeState::Phase1: {
                // 0 → L1 へ移動中
                if (l1 == 0) return xp[0];
                int32_t progress = (curPct * 256) / l1;
                return xp[0] + static_cast<int16_t>(
                    (static_cast<int32_t>(xp[1] - xp[0]) * progress) >> 8
                );
            }
            case Envelope::EnvelopeState::Phase2: {
                // L1 → L2 へ移動中
                int32_t range = static_cast<int32_t>(l1) - l2;
                if (range == 0) return xp[1];
                int32_t dist = static_cast<int32_t>(l1) - curPct;
                int32_t progress = (dist * 256) / range;
                return xp[1] + static_cast<int16_t>(
                    (static_cast<int32_t>(xp[2] - xp[1]) * std::clamp<int32_t>(progress, 0, 256)) >> 8
                );
            }
            case Envelope::EnvelopeState::Phase3: {
                // L2 → L3 へ移動中
                int32_t range = static_cast<int32_t>(l2) - l3;
                if (range == 0) return xp[2];
                int32_t dist = static_cast<int32_t>(l2) - curPct;
                int32_t progress = (dist * 256) / range;
                return xp[2] + static_cast<int16_t>(
                    (static_cast<int32_t>(xp[3] - xp[2]) * std::clamp<int32_t>(progress, 0, 256)) >> 8
                );
            }
            case Envelope::EnvelopeState::Phase4: {
                // L3 → L4 へ移動中 (通常 L4=0)
                int32_t range = static_cast<int32_t>(l3) - l4;
                if (range == 0) return xp[3];
                int32_t dist = static_cast<int32_t>(l3) - curPct;
                int32_t progress = (dist * 256) / range;
                return xp[3] + static_cast<int16_t>(
                    (static_cast<int32_t>(xp[4] - xp[3]) * std::clamp<int32_t>(progress, 0, 256)) >> 8
                );
            }
            default:
                return xp[0]; // Idle
        }
    }
};
