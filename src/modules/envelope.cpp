#include "modules/envelope.hpp"

/** @brief エンベロープを初期位置に戻す（ノートオン時） */
void Envelope::reset(Memory& mem) {
    // 新規ノートの場合（無音 or Idle状態）は初期化
    if (mem.level_ <= ENV_LEVEL_MIN || mem.state == EnvelopeState::Idle) {
        mem.level_ = 0;
        mem.current_level = 0;
        mem.state = EnvelopeState::Phase1;
    } else {
        // リトリガーの場合：現在のlevel_を維持
        // 現在の音量がtarget_level1より高い場合はPhase2から開始
        if (mem.level_ >= mem.target_level1) {
            mem.state = EnvelopeState::Phase2;
        } else {
            mem.state = EnvelopeState::Phase1;
        }
    }
    // current_levelをlevel_に合わせて更新
    updateCurrentLevel(mem);
}

/** @brief エンベロープをリリースフェーズに移行（ノートオフ時） */
void Envelope::release(Memory& mem) {
    if(mem.state != EnvelopeState::Phase4 && mem.state != EnvelopeState::Idle) {
        mem.state = EnvelopeState::Phase4;
    }
}

/** @brief エンベロープを完全にIdle状態にリセット（ノートスロット解放時） */
void Envelope::clear(Memory& mem) {
    mem.state = EnvelopeState::Idle;
    mem.level_ = 0;
    mem.current_level = 0;
}

/**
 * @brief エンベロープ状態を更新します
 *
 * Phase1: 現在レベル → L1 (Rate1) - アタック (rising)
 * Phase2: L1 → L2 (Rate2) - ディケイ1
 * Phase3: L2 → L3 (Rate3) - ディケイ2/サステイン
 * Phase4: 現在レベル → L4 (Rate4) - リリース
 *
 * level_は対数スケール、大きいほど音が大きい
 * rising時は対数カーブ、falling時は線形カーブ
 */
FASTRUN void Envelope::update(Memory& mem) {
    const int8_t rs_delta = mem.rate_scaling_delta;

    // qrate計算とinc計算
    // 係数: 41
    auto calcInc = [rs_delta](uint8_t rate_param, int coef = 41) -> int32_t {
        int qrate = (static_cast<int>(rate_param) * coef) >> 6;
        qrate += rs_delta;
        if (qrate < 0) qrate = 0;
        if (qrate > 63) qrate = 63;
        constexpr int LG_N = 6;
        return (4 + (qrate & 3)) << (2 + LG_N + (qrate >> 2));
    };

    // ノートごとのターゲットレベル
    const EnvLevel_t tgt1 = mem.target_level1;
    const EnvLevel_t tgt2 = mem.target_level2;
    const EnvLevel_t tgt3 = mem.target_level3;
    const EnvLevel_t tgt4 = mem.target_level4;

    switch(mem.state) {
        case EnvelopeState::Phase1: {
            // Phase1: アタック (rising: 0 → target_level1)
            // 対数カーブ (jumptarget経由)
            if (mem.level_ < tgt1) {
                // rising
                if (mem.level_ < ENV_JUMPTARGET) {
                    mem.level_ = ENV_JUMPTARGET;
                }
                int32_t inc = calcInc(rate1_param);
                // level += ((17 << 24) - level) >> 24) * inc
                mem.level_ += (((17 << 24) - mem.level_) >> 24) * inc;
                if (mem.level_ >= tgt1) {
                    mem.level_ = tgt1;
                    mem.state = EnvelopeState::Phase2;
                }
            } else {
                mem.state = EnvelopeState::Phase2;
            }
            break;
        }

        case EnvelopeState::Phase2: {
            // Phase2: ディケイ1 (target_level1 → target_level2)
            int32_t inc = calcInc(rate2_param);
            if (mem.level_ > tgt2) {
                // falling (レベルを下げる)
                mem.level_ -= inc;
                if (mem.level_ <= tgt2) {
                    mem.level_ = tgt2;
                    mem.state = EnvelopeState::Phase3;
                }
            } else if (mem.level_ < tgt2) {
                // rising (レベルを上げる)
                if (mem.level_ < ENV_JUMPTARGET) {
                    mem.level_ = ENV_JUMPTARGET;
                }
                mem.level_ += (((17 << 24) - mem.level_) >> 24) * inc;
                if (mem.level_ >= tgt2) {
                    mem.level_ = tgt2;
                    mem.state = EnvelopeState::Phase3;
                }
            } else {
                mem.state = EnvelopeState::Phase3;
            }
            break;
        }

        case EnvelopeState::Phase3: {
            // Phase3: ディケイ2/サステイン (target_level2 → target_level3)
            int32_t inc = calcInc(rate3_param);
            if (mem.level_ > tgt3) {
                mem.level_ -= inc;
                if (mem.level_ <= tgt3) {
                    mem.level_ = tgt3;
                }
            } else if (mem.level_ < tgt3) {
                if (mem.level_ < ENV_JUMPTARGET) {
                    mem.level_ = ENV_JUMPTARGET;
                }
                mem.level_ += (((17 << 24) - mem.level_) >> 24) * inc;
                if (mem.level_ >= tgt3) {
                    mem.level_ = tgt3;
                }
            }
            // L3に到達してもPhase3のまま（サステイン維持）
            break;
        }

        case EnvelopeState::Phase4: {
            // Phase4: リリース (現在レベル → target_level4)
            int32_t inc = calcInc(rate4_param);
            if (mem.level_ > tgt4) {
                mem.level_ -= inc;
                if (mem.level_ <= tgt4) {
                    mem.level_ = tgt4;
                    mem.state = EnvelopeState::Idle;
                }
            } else if (mem.level_ < tgt4) {
                if (mem.level_ < ENV_JUMPTARGET) {
                    mem.level_ = ENV_JUMPTARGET;
                }
                mem.level_ += (((17 << 24) - mem.level_) >> 24) * inc;
                if (mem.level_ >= tgt4) {
                    mem.level_ = tgt4;
                    mem.state = EnvelopeState::Idle;
                }
            } else {
                mem.state = EnvelopeState::Idle;
            }
            break;
        }

        case EnvelopeState::Idle:
        default:
            mem.level_ = tgt4;
            break;
    }

    // 音量レベルを更新 (Exp2 lookup)
    updateCurrentLevel(mem);
}

// ===== Rate設定 =====

/**
 * @brief Rate1（アタックレート）を設定
 * @param rate_0_99 0=最も遅い, 99=即座に到達
 */
void Envelope::setRate1(uint8_t rate_0_99) {
    rate1_param = clamp_param(rate_0_99);
}

/**
 * @brief Rate2（ディケイ1レート）を設定
 * @param rate_0_99 0=最も遅い, 99=即座に到達
 */
void Envelope::setRate2(uint8_t rate_0_99) {
    rate2_param = clamp_param(rate_0_99);
}

/**
 * @brief Rate3（ディケイ2/サステインレート）を設定
 * @param rate_0_99 0=最も遅い, 99=即座に到達
 */
void Envelope::setRate3(uint8_t rate_0_99) {
    rate3_param = clamp_param(rate_0_99);
}

/**
 * @brief Rate4（リリースレート）を設定
 * @param rate_0_99 0=最も遅い, 99=即座に到達
 */
void Envelope::setRate4(uint8_t rate_0_99) {
    rate4_param = clamp_param(rate_0_99);
}

// ===== Level設定 =====

/**
 * @brief Level1（アタック到達レベル）を設定
 * @param level_0_99 0=無音, 99=最大音量
 */
void Envelope::setLevel1(uint8_t level_0_99) {
    level1_param = clamp_param(level_0_99);
    // ターゲットレベルはsetOutlevel()で計算される
}

/**
 * @brief Level2（ディケイ1到達レベル）を設定
 * @param level_0_99 0=無音, 99=最大音量
 */
void Envelope::setLevel2(uint8_t level_0_99) {
    level2_param = clamp_param(level_0_99);
}

/**
 * @brief Level3（サステインレベル）を設定
 * @param level_0_99 0=無音, 99=最大音量
 */
void Envelope::setLevel3(uint8_t level_0_99) {
    level3_param = clamp_param(level_0_99);
}

/**
 * @brief Level4（リリース到達レベル）を設定
 * @param level_0_99 0=無音, 99=最大音量
 */
void Envelope::setLevel4(uint8_t level_0_99) {
    level4_param = clamp_param(level_0_99);
}

// ===== Rate Scaling =====

/**
 * @brief Rate Scaling sensitivity を設定
 * @param sensitivity 0-7 (0=効果なし, 7=最大効果)
 */
void Envelope::setRateScaling(uint8_t sensitivity) {
    rate_scaling_param = (sensitivity > 7) ? 7 : sensitivity;
}

/**
 * @brief ベロシティ感度を設定
 * @param sens 0-7 (0=感度なし, 7=最大感度)
 */
void Envelope::setVelocitySens(uint8_t sens) {
    velocity_sens_ = (sens > 7) ? 7 : sens;
}

/**
 * @brief Rate Scaling計算
 *
 * @param midinote MIDIノート番号 (0-127)
 * @param sensitivity Rate Scaling sensitivity (0-7)
 * @return int8_t rate増分 (0-27)
 */
int8_t Envelope::calcRateScalingDelta(uint8_t midinote, uint8_t sensitivity) {
    if (sensitivity == 0) return 0;

    // x = clamp(0, 31, midinote / 3 - 7)
    int16_t x = (midinote / 3) - 7;
    if (x < 0) x = 0;
    if (x > 31) x = 31;

    // qratedelta = (sensitivity * x) >> 3
    int8_t delta = static_cast<int8_t>((sensitivity * x) >> 3);
    return delta;
}

/**
 * @brief ノートごとのRate Scaling増分を設定
 *
 * このメソッドはノートオン時に呼び出し、ノート番号に応じた
 * rate scaling増分をMemoryに設定する
 *
 * @param mem エンベロープメモリ
 * @param midinote MIDIノート番号
 */
void Envelope::applyRateScaling(Memory& mem, uint8_t midinote) {
    mem.rate_scaling_delta = calcRateScalingDelta(midinote, rate_scaling_param);
}

// ===== Keyboard Level Scaling =====

/**
 * @brief ブレークポイントを設定
 * @param break_point 0-99 (0=A-1, 39=C3, 99=C8)
 */
void Envelope::setBreakPoint(uint8_t break_point) {
    kbd_break_point = (break_point > 99) ? 99 : break_point;
}

/**
 * @brief 左側スケーリング深さを設定
 * @param depth 0-99
 */
void Envelope::setLeftDepth(uint8_t depth) {
    kbd_left_depth = (depth > 99) ? 99 : depth;
}

/**
 * @brief 右側スケーリング深さを設定
 * @param depth 0-99
 */
void Envelope::setRightDepth(uint8_t depth) {
    kbd_right_depth = (depth > 99) ? 99 : depth;
}

/**
 * @brief 左側スケーリングカーブを設定
 * @param curve KeyScaleCurve
 */
void Envelope::setLeftCurve(KeyScaleCurve curve) {
    kbd_left_curve = curve;
}

/**
 * @brief 右側スケーリングカーブを設定
 * @param curve KeyScaleCurve
 */
void Envelope::setRightCurve(KeyScaleCurve curve) {
    kbd_right_curve = curve;
}

/**
 * @brief 左側スケーリングカーブを設定 (数値)
 * @param curve 0-3 (0=-LN, 1=-EX, 2=+EX, 3=+LN)
 */
void Envelope::setLeftCurve(uint8_t curve) {
    if (curve > 3) curve = 3;
    kbd_left_curve = static_cast<KeyScaleCurve>(curve);
}

/**
 * @brief 右側スケーリングカーブを設定 (数値)
 * @param curve 0-3 (0=-LN, 1=-EX, 2=+EX, 3=+LN)
 */
void Envelope::setRightCurve(uint8_t curve) {
    if (curve > 3) curve = 3;
    kbd_right_curve = static_cast<KeyScaleCurve>(curve);
}

/**
 * @brief スケーリングカーブ計算
 *
 * groupとdepthとカーブタイプからスケーリング値を計算
 *
 * @param group ブレークポイントからの距離グループ (0-31+)
 * @param depth スケーリング深さ (0-99)
 * @param curve カーブタイプ
 * @return int スケーリング値 (正=音量増加, 負=音量減少)
 */
int Envelope::scaleCurve(int group, int depth, KeyScaleCurve curve) {
    int scale;

    if (curve == KeyScaleCurve::NegLin || curve == KeyScaleCurve::PosLin) {
        // 線形カーブ
        scale = (group * depth * 329) >> 12;
    } else {
        // 指数カーブ
        constexpr int n_scale_data = sizeof(EXP_SCALE_DATA);
        int raw_exp = EXP_SCALE_DATA[std::min(group, n_scale_data - 1)];
        scale = (raw_exp * depth * 329) >> 15;
    }

    // カーブ方向: NegLin(0), NegExp(1) は負方向
    if (curve == KeyScaleCurve::NegLin || curve == KeyScaleCurve::NegExp) {
        scale = -scale;
    }

    return scale;
}

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
int Envelope::scaleLevel(int midinote, int break_pt, int left_depth, int right_depth,
                        KeyScaleCurve left_curve, KeyScaleCurve right_curve) {
    // offset = midinote - break_pt - 21
    // break_pt=0 が A-1 (MIDI note 21)、break_pt=39 が C3 (MIDI note 60) に対応
    int offset = midinote - break_pt - 21;

    if (offset >= 0) {
        // ブレークポイントより高いノート → 右側スケーリング
        return scaleCurve((offset + 1) / 3, right_depth, right_curve);
    } else {
        // ブレークポイントより低いノート → 左側スケーリング
        return scaleCurve(-(offset - 1) / 3, left_depth, left_curve);
    }
}

/**
 * @brief Keyboard Level Scalingを適用したスケーリング値を計算
 *
 * 現在のKLSパラメータを使用してレベルスケーリングを計算
 *
 * @param midinote MIDIノート番号 (0-127)
 * @return int レベルスケーリング値
 */
int Envelope::calcKeyboardLevelScaling(uint8_t midinote) const {
    return scaleLevel(midinote, kbd_break_point, kbd_left_depth, kbd_right_depth,
                     kbd_left_curve, kbd_right_curve);
}

// ===== Outlevel =====

/**
 * @brief オペレーター出力レベルとベロシティから基準レベルを設定
 *
 * Output Level、Velocity、Keyboard Level Scalingを組み合わせて
 * outlevel_を計算する。ノートごとに呼び出す。
 *
 * @param op_level オペレーター出力レベル (0-99)
 * @param velocity MIDIベロシティ (0-127)
 * @param midinote MIDIノート番号 (0-127) - Keyboard Level Scaling用
 * @param velocity_sens ベロシティ感度 (0-7、0=感度なし)
 */
void Envelope::setOutlevel(uint8_t op_level, uint8_t velocity, uint8_t midinote, uint8_t velocity_sens) {
    // 1. Output Level → scaleoutlevel (0-127)
    int outlevel = scaleoutlevel(op_level);

    // 2. Keyboard Level Scaling を適用
    int level_scaling = calcKeyboardLevelScaling(midinote);
    outlevel += level_scaling;

    // 3. 0-127 にクランプ
    if (outlevel > 127) outlevel = 127;
    if (outlevel < 0) outlevel = 0;

    // 4. 内部精度へ拡張 (<< 5 = ×32)
    outlevel = outlevel << 5;

    // 5. ベロシティスケーリング
    if (velocity_sens > 0) {
        // velocity 127 = 減衰なし, velocity 0 = 最大減衰
        int sens_squared = velocity_sens * velocity_sens;
        int vel_attenuation = ((127 - velocity) * sens_squared) >> 4;
        outlevel = std::max(0, outlevel - vel_attenuation);
    }

    // クラスメンバーに保存
    outlevel_ = outlevel;
}

/**
 * @brief ノートごとのターゲットレベルを計算
 *
 * setOutlevel()で設定されたoutlevel_を使用して、
 * ノートごとのターゲットレベル(mem.target_level1-4)を計算する。
 * ノートオン時に呼び出す。
 *
 * @param mem エンベロープメモリ
 */
void Envelope::calcNoteTargetLevels(Memory& mem) {
    mem.target_level1 = calcTargetLevel(level1_param, outlevel_);
    mem.target_level2 = calcTargetLevel(level2_param, outlevel_);
    mem.target_level3 = calcTargetLevel(level3_param, outlevel_);
    mem.target_level4 = calcTargetLevel(level4_param, outlevel_);
}

/**
 * @brief ターゲットレベル計算
 *
 * actuallevel = scaleoutlevel(env_level) >> 1 + outlevel - 4256
 * targetlevel = actuallevel << 16
 *
 * @param env_level エンベロープレベルパラメータ (0-99)
 * @param outlevel オペレーター基準レベル (setOutlevelで設定)
 * @return EnvLevel_t ターゲットレベル
 */
EnvLevel_t Envelope::calcTargetLevel(uint8_t env_level, EnvLevel_t outlevel) {
    // actuallevel計算
    EnvLevel_t actuallevel = scaleoutlevel(env_level) >> 1;  // 0-63程度

    // (actuallevel << 6) + outlevel - 4256
    actuallevel = (actuallevel << 6) + outlevel - 4256;

    // 最小値制限 (16)
    if (actuallevel < 16) actuallevel = 16;

    // targetlevel = actuallevel << 16 (そのまま！変換しない！)
    return actuallevel << 16;
}

/**
 * @brief 対数レベルから線形レベルへ変換 (Exp2 lookup)
 *
 * @param mem エンベロープメモリ
 */
void Envelope::updateCurrentLevel(Memory& mem) {
    // level_in - (14 * (1 << 24)) でスケーリング
    // 14オクターブ分のオフセットを引く
    int32_t level_in = mem.level_ - (14 << 24);

    // Exp2 lookup で線形に変換 (Q24 → Q24)
    int32_t linear = exp2_lookup(level_in);

    // Q24のままクリップせずに保持
    // exp2_lookupの出力は最大約 1<<24 なのでENVGAIN_MAXでクリップ
    if (linear > ENVGAIN_MAX) linear = ENVGAIN_MAX;
    if (linear < 0) linear = 0;

    mem.current_level = static_cast<EnvGain_t>(linear);
}
