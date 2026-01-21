#pragma once

#include "handlers/audio.hpp"
#include "utils/math.hpp"
#include "types.hpp"

/**
 * @brief Keyboard Level Scaling のカーブタイプ
 *
 * スケーリングカーブ：
 * - NegLin (-LN): 負方向・線形
 * - NegExp (-EX): 負方向・指数関数的
 * - PosExp (+EX): 正方向・指数関数的
 * - PosLin (+LN): 正方向・線形
 */
enum class KeyScaleCurve : uint8_t {
    NegLin = 0,  // -LN: ブレークポイントから離れるほど音量減少（線形）
    NegExp = 1,  // -EX: ブレークポイントから離れるほど音量減少（指数）
    PosExp = 2,  // +EX: ブレークポイントから離れるほど音量増加（指数）
    PosLin = 3   // +LN: ブレークポイントから離れるほど音量増加（線形）
};

/**
 * @brief FM方式エンベロープジェネレーター
 *
 * 4つのRate (R1-R4) と 4つのLevel (L1-L4) で構成される。
 *
 * 動作フロー:
 * - Phase1: 現在レベル → L1 (Rate1で移動) [アタック]
 * - Phase2: L1 → L2 (Rate2で移動) [ディケイ1]
 * - Phase3: L2 → L3 (Rate3で移動) [ディケイ2/サステイン]
 * - Phase4: L3 → L4 (Rate4で移動) [リリース] ※ノートオフで発動
 *
 * Rate: 0=最も遅い, 99=最も速い（即座に到達）
 * Level: 0=無音, 99=最大音量
 *
 * Keyboard Level Scaling:
 * - Break Point: スケーリングの基準となるノート (0-99 → A-1～C8)
 * - Left Depth/Right Depth: 左右のスケーリング深さ (0-99)
 * - Left Curve/Right Curve: スケーリングカーブタイプ (-LN, -EX, +EX, +LN)
 */
class Envelope {

public:
    static constexpr uint32_t RATE_TABLE_SIZE = 100;
    static constexpr uint32_t LEVEL_TABLE_SIZE = 100;

    // Q24形式の対数レベル
    // level_ は Q24/doubling log format (2^24 = 1オクターブ = 2倍)
    // Exp2テーブルサイズ (1024サンプル)
    static constexpr uint32_t EXP2_LG_N_SAMPLES = 10;
    static constexpr uint32_t EXP2_N_SAMPLES = 1 << EXP2_LG_N_SAMPLES;

    // エンベロープステート
    enum class EnvelopeState {
        Phase1,  // → L1 (アタック)
        Phase2,  // L1 → L2 (ディケイ1)
        Phase3,  // L2 → L3 (ディケイ2/サステイン)
        Phase4,  // → L4 (リリース)
        Idle     // 完了/待機
    };

    struct Memory {
        EnvelopeState state = EnvelopeState::Idle;
        EnvLevel_t level_ = 0;  // Q24対数レベル (大きいほど音が大きい)
        EnvGain_t current_level = 0;  // 線形スケールの最終出力レベル (Q24: 0-16777215)
        int8_t rate_scaling_delta = 0; // Rate Scalingによるrate増分 (ノートごとに計算)
        // ノートごとのターゲットレベル (対数レベル、大きいほど音が大きい)
        EnvLevel_t target_level1 = ENV_LEVEL_MIN;
        EnvLevel_t target_level2 = ENV_LEVEL_MIN;
        EnvLevel_t target_level3 = ENV_LEVEL_MIN;
        EnvLevel_t target_level4 = ENV_LEVEL_MIN;
    };

private:
    // Rate/Level形式のパラメータ (0-99)
    // Rate: 変化速度 (0=遅い, 99=即座)
    // Level: 目標レベル (0=無音, 99=最大)
    uint8_t rate1_param = 99;   // アタックレート
    uint8_t rate2_param = 99;   // ディケイ1レート
    uint8_t rate3_param = 99;   // ディケイ2レート
    uint8_t rate4_param = 99;   // リリースレート
    uint8_t level1_param = 99;  // アタック到達レベル
    uint8_t level2_param = 99;  // ディケイ1到達レベル
    uint8_t level3_param = 99;  // サステインレベル
    uint8_t level4_param = 0;   // リリース到達レベル（通常0）

    uint8_t rate_scaling_param = 0; // Rate Scaling sensitivity (0-7)

    // === Keyboard Level Scaling パラメータ ===
    uint8_t kbd_break_point = 39;          // ブレークポイント (0-99, デフォルト39=C3)
    uint8_t kbd_left_depth = 0;            // 左側スケーリング深さ (0-99)
    uint8_t kbd_right_depth = 0;           // 右側スケーリング深さ (0-99)
    KeyScaleCurve kbd_left_curve = KeyScaleCurve::NegLin;   // 左側カーブ
    KeyScaleCurve kbd_right_curve = KeyScaleCurve::NegLin;  // 右側カーブ

    // オペレーター出力レベル (クラスメンバー)
    EnvLevel_t outlevel_ = 0;

    // === 指数スケーリング用データテーブル ===
    // 33要素: group 0-32 に対応する指数カーブ値
    static constexpr uint8_t EXP_SCALE_DATA[33] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 14, 16, 19, 23, 27, 33, 39, 47, 56, 66,
        80, 94, 110, 126, 142, 158, 174, 190, 206, 222, 238, 250
    };

    /**
     * @brief レートテーブルを生成
     *
     * inc = (4 + (qrate & 3)) << (2 + LG_N + (qrate >> 2))
     * qrate = (rate * 58) >> 6 + rate_scaling
     * ここではrate_scalingを除いた基本レートを計算
     * ※係数58はDexenvとの速度差補正 (元は41)
     */
    static constexpr std::array<uint32_t, RATE_TABLE_SIZE> generate_rate_table() {
        std::array<uint32_t, RATE_TABLE_SIZE> table{};
        constexpr int LG_N = 6;  // N=64

        for (size_t i = 0; i < RATE_TABLE_SIZE; ++i) {
            int qrate = (static_cast<int>(i) * 58) >> 6;
            if (qrate > 63) qrate = 63;
            // インクリメント計算
            table[i] = static_cast<uint32_t>((4 + (qrate & 3)) << (2 + LG_N + (qrate >> 2)));
        }
        return table;
    }

    // Exp2テーブル (Q24 log → Q24 linear)
    // 2^(x / 2^24) を計算するためのテーブル
    static constexpr std::array<int32_t, EXP2_N_SAMPLES * 2> generate_exp2_table() {
        std::array<int32_t, EXP2_N_SAMPLES * 2> table{};
        double inc = std::pow(2.0, 1.0 / EXP2_N_SAMPLES);
        double y = static_cast<double>(1 << 30);
        for (size_t i = 0; i < EXP2_N_SAMPLES; ++i) {
            table[(i << 1) + 1] = static_cast<int32_t>(y + 0.5);
            y *= inc;
        }
        for (size_t i = 0; i < EXP2_N_SAMPLES - 1; ++i) {
            table[i << 1] = table[(i << 1) + 3] - table[(i << 1) + 1];
        }
        table[(EXP2_N_SAMPLES << 1) - 2] = (1U << 31) - table[(EXP2_N_SAMPLES << 1) - 1];
        return table;
    }

    // Exp2::lookup (Q24 in, Q24 out)
    static inline int32_t exp2_lookup(int32_t x) {
        constexpr int SHIFT = 24 - EXP2_LG_N_SAMPLES;
        int lowbits = x & ((1 << SHIFT) - 1);
        int x_int = (x >> (SHIFT - 1)) & ((EXP2_N_SAMPLES - 1) << 1);
        int dy = exp2_table[x_int];
        int y0 = exp2_table[x_int + 1];
        int y = y0 + (((int64_t)dy * (int64_t)lowbits) >> SHIFT);
        return y >> (6 - (x >> 24));
    }

    static inline uint8_t clamp_param(uint8_t value) {
        return (value >= RATE_TABLE_SIZE) ? (RATE_TABLE_SIZE - 1) : value;
    }

    // レートテーブル（0-99）-> 増分値
    inline static const std::array<uint32_t, RATE_TABLE_SIZE> rate_table = generate_rate_table();

    // Exp2テーブル (対数レベル -> 線形レベルへ)
    inline static const std::array<int32_t, EXP2_N_SAMPLES * 2> exp2_table = generate_exp2_table();

public:
    void reset(Memory& mem);
    void release(Memory& mem);
    void clear(Memory& mem);  // 完全にIdle状態にリセット

    FASTRUN void update(Memory& mem);

    // 対数レベルから線形レベルへ変換
    static void updateCurrentLevel(Memory& mem);

    // Rate設定 (0-99: 0=遅い, 99=即座)
    void setRate1(uint8_t rate_0_99);
    void setRate2(uint8_t rate_0_99);
    void setRate3(uint8_t rate_0_99);
    void setRate4(uint8_t rate_0_99);

    // Level設定 (0-99: 0=無音, 99=最大)
    void setLevel1(uint8_t level_0_99);
    void setLevel2(uint8_t level_0_99);
    void setLevel3(uint8_t level_0_99);
    void setLevel4(uint8_t level_0_99);

    /**
     * @brief 現在のレベルを返す
     *
     * @return EnvGain_t Q24スケール (0-16777215)
     */
    inline EnvGain_t currentLevel(const Memory& mem) const {
        return mem.current_level;
    }

    /**
     * @brief 内部対数レベルを返す
     *
     * @return EnvLevel_t 対数レベル (大きいほど音が大きい)
     */
    inline EnvLevel_t getLevel(const Memory& mem) const {
        return mem.level_;
    }

    /**
     * @brief エンベロープ終了判定
     *
     * Idle状態、または音量レベルが十分小さい場合（聞こえない程度）に終了とみなす
     *
     * @return 終了していれば `true` を返す
     */
    inline bool isFinished(const Memory& mem) const {
        // Idle状態なら終了
        if (mem.state == EnvelopeState::Idle) return true;
        // Phase4（リリース中）で音量が非常に小さい場合も終了とみなす
        // level_がENV_LEVEL_MIN以下なら実質無音
        if (mem.state == EnvelopeState::Phase4 && mem.level_ <= ENV_LEVEL_MIN) return true;
        return false;
    }

    // Rate ゲッター (0-99)
    inline uint8_t getRate1() const { return rate1_param; }
    inline uint8_t getRate2() const { return rate2_param; }
    inline uint8_t getRate3() const { return rate3_param; }
    inline uint8_t getRate4() const { return rate4_param; }

    // Level ゲッター (0-99)
    inline uint8_t getLevel1() const { return level1_param; }
    inline uint8_t getLevel2() const { return level2_param; }
    inline uint8_t getLevel3() const { return level3_param; }
    inline uint8_t getLevel4() const { return level4_param; }

    // Rate Scaling 設定/取得 (0-7)
    void setRateScaling(uint8_t sensitivity);
    inline uint8_t getRateScaling() const { return rate_scaling_param; }

    /**
     * @brief Rate Scaling計算
     *
     * MIDIノート番号とsensitivityからrate増分を計算
     * 高いノートほどエンベロープが速くなる
     *
     * @param midinote MIDIノート番号 (0-127)
     * @param sensitivity Rate Scaling sensitivity (0-7)
     * @return int8_t rate増分 (0-27)
     */
    static int8_t calcRateScalingDelta(uint8_t midinote, uint8_t sensitivity);

    /**
     * @brief ノートごとのRate Scaling増分を設定
     *
     * @param mem エンベロープメモリ
     * @param midinote MIDIノート番号
     */
    void applyRateScaling(Memory& mem, uint8_t midinote);

    // =============================================
    // Keyboard Level Scaling
    // =============================================

    /**
     * @brief ブレークポイントを設定
     *
     * @param break_point 0-99 (0=A-1, 39=C3, 99=C8)
     */
    void setBreakPoint(uint8_t break_point);

    /**
     * @brief 左側スケーリング深さを設定
     *
     * @param depth 0-99
     */
    void setLeftDepth(uint8_t depth);

    /**
     * @brief 右側スケーリング深さを設定
     *
     * @param depth 0-99
     */
    void setRightDepth(uint8_t depth);

    /**
     * @brief 左側スケーリングカーブを設定
     *
     * @param curve KeyScaleCurve (0-3)
     */
    void setLeftCurve(KeyScaleCurve curve);

    /**
     * @brief 右側スケーリングカーブを設定
     *
     * @param curve KeyScaleCurve (0-3)
     */
    void setRightCurve(KeyScaleCurve curve);

    /**
     * @brief 左側スケーリングカーブを設定 (数値)
     *
     * @param curve 0-3 (0=-LN, 1=-EX, 2=+EX, 3=+LN)
     */
    void setLeftCurve(uint8_t curve);

    /**
     * @brief 右側スケーリングカーブを設定 (数値)
     *
     * @param curve 0-3 (0=-LN, 1=-EX, 2=+EX, 3=+LN)
     */
    void setRightCurve(uint8_t curve);

    // Keyboard Level Scaling ゲッター
    inline uint8_t getBreakPoint() const { return kbd_break_point; }
    inline uint8_t getLeftDepth() const { return kbd_left_depth; }
    inline uint8_t getRightDepth() const { return kbd_right_depth; }
    inline KeyScaleCurve getLeftCurve() const { return kbd_left_curve; }
    inline KeyScaleCurve getRightCurve() const { return kbd_right_curve; }

    /**
     * @brief スケーリングカーブ計算
     *
     * groupとdepthとカーブタイプからスケーリング値を計算
     *
     * @param group ブレークポイントからの距離グループ (0-31+)
     * @param depth スケーリング深さ (0-99)
     * @param curve カーブタイプ (KeyScaleCurve)
     * @return int スケーリング値 (正=音量増加, 負=音量減少)
     */
    static int scaleCurve(int group, int depth, KeyScaleCurve curve);

    /**
     * @brief Keyboard Level Scaling計算
     *
     * MIDIノート番号とブレークポイント、左右の深さ・カーブから
     * レベルスケーリング値を計算
     *
     * @param midinote MIDIノート番号 (0-127)
     * @param break_pt ブレークポイント (0-99)
     * @param left_depth 左側深さ (0-99)
     * @param right_depth 右側深さ (0-99)
     * @param left_curve 左側カーブ
     * @param right_curve 右側カーブ
     * @return int レベルスケーリング値 (outlevelに加算)
     */
    static int scaleLevel(int midinote, int break_pt, int left_depth, int right_depth,
                         KeyScaleCurve left_curve, KeyScaleCurve right_curve);

    /**
     * @brief Keyboard Level Scalingを適用したoutlevelを計算
     *
     * 現在のKLSパラメータを使用してレベルスケーリングを計算し、
     * outlevel_に反映する
     *
     * @param midinote MIDIノート番号 (0-127)
     * @return int レベルスケーリング値
     */
    int calcKeyboardLevelScaling(uint8_t midinote) const;

    /**
     * @brief レベル(0-99)を内部スケールに変換
     *
     * 低レベル域(0-19)は非線形テーブル参照、高レベル域(20-99)は線形(28+level)
     *
     * @param level オペレータレベル (0-99)
     * @return int 変換後の値 (0-127)
     */
    static inline int scaleoutlevel(int level) {
        return (level >= 20) ? (28 + level) : static_cast<int>(AudioMath::LOW_LEVEL_LUT[level]);
    }

    /**
     * @brief オペレーター出力レベルとベロシティから基準レベルを設定
     *
     * Output Level、Velocity、Keyboard Level Scalingを組み合わせて
     * outlevel_を計算する。ノートごとに呼び出す。
     * ノートごとのターゲットレベル計算はcalcNoteTargetLevels()で行う。
     *
     * @param op_level オペレーター出力レベル (0-99)
     * @param velocity MIDIベロシティ (0-127)
     * @param midinote MIDIノート番号 (0-127) - Keyboard Level Scaling用
     * @param velocity_sens ベロシティ感度 (0-7、0=感度なし)
     */
    void setOutlevel(uint8_t op_level, uint8_t velocity, uint8_t midinote, uint8_t velocity_sens = 0);

    /**
     * @brief ノートごとのターゲットレベルを計算
     *
     * setOutlevel()で設定されたoutlevel_を使用して、
     * ノートごとのターゲットレベル(mem.target_level1-4)を計算する。
     * ノートオン時に呼び出す。
     *
     * @param mem エンベロープメモリ
     */
    void calcNoteTargetLevels(Memory& mem);

    /**
     * @brief ターゲットレベル計算時にoutlevelを適用
     *
     * actuallevel = scaleoutlevel(env_level) >> 1 + outlevel - 4256
     * targetlevel = actuallevel << 16
     *
     * @param env_level エンベロープレベルパラメータ (0-99)
     * @param outlevel オペレーター基準レベル (setOutlevelで設定)
     * @return EnvLevel_t ターゲットレベル (Q24形式相当)
     */
    static EnvLevel_t calcTargetLevel(uint8_t env_level, EnvLevel_t outlevel);
};
