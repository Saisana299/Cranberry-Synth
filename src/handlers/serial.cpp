#include "handlers/serial.hpp"
#include "modules/synth.hpp"
#include <cstring>
#include <cstdlib>

SerialHandler serial_hdl;

// =============================================
// ボタン名テーブル
// =============================================
struct BtnEntry { const char* name; uint8_t id_short; uint8_t id_long; };
static const BtnEntry BTN_TABLE[] = {
    {"UP",  BTN_UP,  BTN_UP_LONG},
    {"DN",  BTN_DN,  BTN_DN_LONG},
    {"L",   BTN_L,   BTN_L_LONG},
    {"R",   BTN_R,   BTN_R_LONG},
    {"ET",  BTN_ET,  BTN_ET_LONG},
    {"CXL", BTN_CXL, BTN_CXL_LONG},
    {"EC",  BTN_ET,  BTN_ET_LONG},  // エンコーダークリック = 決定ボタン
};
static constexpr uint8_t BTN_TABLE_SIZE = sizeof(BTN_TABLE) / sizeof(BTN_TABLE[0]);

// =============================================
// パーサーユーティリティ
// =============================================

// s が prefix で始まれば残り文字列を返す、マッチしなければ nullptr
static const char* match(const char* s, uint8_t len, const char* prefix) {
    uint8_t plen = strlen(prefix);
    if (len < plen || strncmp(s, prefix, plen) != 0) return nullptr;
    return s + plen;
}

static bool parseU8(const char* s, uint8_t& out, uint8_t lo, uint8_t hi) {
    if (!s || !*s) return false;
    int v = atoi(s);
    if (v < lo || v > hi) return false;
    out = static_cast<uint8_t>(v);
    return true;
}

static bool parseI8(const char* s, int8_t& out, int8_t lo, int8_t hi) {
    if (!s || !*s) return false;
    int v = atoi(s);
    if (v < lo || v > hi) return false;
    out = static_cast<int8_t>(v);
    return true;
}

static bool parseBool(const char* s, bool& out) {
    if (!s || !*s) return false;
    int v = atoi(s);
    if (v != 0 && v != 1) return false;
    out = (v == 1);
    return true;
}

// =============================================
// SET MASTER
// =============================================
static void handleSetMaster(const char* s, uint8_t len) {
    Synth& synth = Synth::getInstance();
    const char* arg;

    if ((arg = match(s, len, "LEVEL "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 127)) { Serial.println("ERR: LEVEL 0-127"); return; }
        synth.setMasterLevel(static_cast<Gain_t>(static_cast<int32_t>(v) * Q15_MAX / 127));
        Serial.printf("OK: MASTER LEVEL %d\n", v); return;
    }
    if ((arg = match(s, len, "TRANSPOSE "))) {
        int8_t v; if (!parseI8(arg, v, -24, 24)) { Serial.println("ERR: TRANSPOSE -24..24"); return; }
        synth.setTranspose(v);
        Serial.printf("OK: MASTER TRANSPOSE %d\n", v); return;
    }
    if ((arg = match(s, len, "ALGO "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 31)) { Serial.println("ERR: ALGO 0-31"); return; }
        synth.setAlgorithm(v);
        Serial.printf("OK: MASTER ALGO %d\n", v); return;
    }
    if ((arg = match(s, len, "FB "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 7)) { Serial.println("ERR: FB 0-7"); return; }
        synth.setFeedback(v);
        Serial.printf("OK: MASTER FB %d\n", v); return;
    }
    if ((arg = match(s, len, "BEND "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 24)) { Serial.println("ERR: BEND 0-24"); return; }
        synth.setPitchBendRange(v);
        Serial.printf("OK: MASTER BEND %d\n", v); return;
    }
    if ((arg = match(s, len, "VEL "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 3)) { Serial.println("ERR: VEL 0-3"); return; }
        synth.setVelocityCurve(v);
        Serial.printf("OK: MASTER VEL %d\n", v); return;
    }
    if ((arg = match(s, len, "PRESET "))) {
        uint8_t v; if (!parseU8(arg, v, 0, MAX_PRESETS - 1)) {
            Serial.printf("ERR: PRESET 0-%d\n", MAX_PRESETS - 1); return;
        }
        synth.loadPreset(v);
        Serial.printf("OK: MASTER PRESET %d (%s)\n", v, synth.getCurrentPresetName()); return;
    }

    Serial.println("ERR: SET MASTER LEVEL|TRANSPOSE|ALGO|FB|BEND|VEL|PRESET <value>");
}

// =============================================
// SET OP <1-6>
// =============================================
static void handleSetOp(const char* s, uint8_t len) {
    Synth& synth = Synth::getInstance();

    if (len < 3 || s[0] < '1' || s[0] > '6' || s[1] != ' ') {
        Serial.println("ERR: SET OP <1-6> <param> <value>"); return;
    }
    uint8_t opIdx = s[0] - '1';
    const char* param = s + 2;
    uint8_t paramLen = len - 2;

    Oscillator& osc = const_cast<Oscillator&>(synth.getOperatorOsc(opIdx));
    Envelope&   env = const_cast<Envelope&>(synth.getOperatorEnv(opIdx));
    const char* arg;

    if ((arg = match(param, paramLen, "LEVEL "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: LEVEL 0-99"); return; }
        osc.setLevelNonLinear(v);
        Serial.printf("OK: OP %d LEVEL %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "WAVE "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 3)) { Serial.println("ERR: WAVE 0=sine 1=tri 2=saw 3=sqr"); return; }
        osc.setWavetable(v);
        Serial.printf("OK: OP %d WAVE %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "COARSE "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 31)) { Serial.println("ERR: COARSE 0-31"); return; }
        osc.setCoarse(static_cast<float>(v));
        Serial.printf("OK: OP %d COARSE %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "FINE "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: FINE 0-99"); return; }
        osc.setFine(static_cast<float>(v));
        Serial.printf("OK: OP %d FINE %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "DETUNE "))) {
        int8_t v; if (!parseI8(arg, v, -50, 50)) { Serial.println("ERR: DETUNE -50..50 (DX7互換: -7..7)"); return; }
        osc.setDetune(v);
        Serial.printf("OK: OP %d DETUNE %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "FIXED "))) {
        bool v; if (!parseBool(arg, v)) { Serial.println("ERR: FIXED 0/1"); return; }
        osc.setFixed(v);
        Serial.printf("OK: OP %d FIXED %d\n", opIdx + 1, (int)v); return;
    }
    if ((arg = match(param, paramLen, "ENABLE "))) {
        bool v; if (!parseBool(arg, v)) { Serial.println("ERR: ENABLE 0/1"); return; }
        v ? osc.enable() : osc.disable();
        Serial.printf("OK: OP %d ENABLE %d\n", opIdx + 1, (int)v); return;
    }
    if ((arg = match(param, paramLen, "AMS "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 3)) { Serial.println("ERR: AMS 0-3"); return; }
        synth.setOperatorAms(opIdx, v);
        Serial.printf("OK: OP %d AMS %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "RS "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 7)) { Serial.println("ERR: RS 0-7"); return; }
        env.setRateScaling(v);
        Serial.printf("OK: OP %d RS %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "VS "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 7)) { Serial.println("ERR: VS 0-7"); return; }
        env.setVelocitySens(v);
        Serial.printf("OK: OP %d VS %d\n", opIdx + 1, v); return;
    }
    // EG Rate / Level
    if ((arg = match(param, paramLen, "R1 "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: R1 0-99"); return; }
        env.setRate1(v); Serial.printf("OK: OP %d R1 %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "R2 "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: R2 0-99"); return; }
        env.setRate2(v); Serial.printf("OK: OP %d R2 %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "R3 "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: R3 0-99"); return; }
        env.setRate3(v); Serial.printf("OK: OP %d R3 %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "R4 "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: R4 0-99"); return; }
        env.setRate4(v); Serial.printf("OK: OP %d R4 %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "L1 "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: L1 0-99"); return; }
        env.setLevel1(v); Serial.printf("OK: OP %d L1 %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "L2 "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: L2 0-99"); return; }
        env.setLevel2(v); Serial.printf("OK: OP %d L2 %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "L3 "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: L3 0-99"); return; }
        env.setLevel3(v); Serial.printf("OK: OP %d L3 %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "L4 "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: L4 0-99"); return; }
        env.setLevel4(v); Serial.printf("OK: OP %d L4 %d\n", opIdx + 1, v); return;
    }
    // KLS (Keyboard Level Scaling)
    if ((arg = match(param, paramLen, "KBP "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: KBP 0-99 (39=C3)"); return; }
        env.setBreakPoint(v); Serial.printf("OK: OP %d KBP %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "KLD "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: KLD 0-99"); return; }
        env.setLeftDepth(v); Serial.printf("OK: OP %d KLD %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "KRD "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: KRD 0-99"); return; }
        env.setRightDepth(v); Serial.printf("OK: OP %d KRD %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "KLC "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 3)) { Serial.println("ERR: KLC 0-3 (0=-LN 1=-EX 2=+EX 3=+LN)"); return; }
        env.setLeftCurve(v); Serial.printf("OK: OP %d KLC %d\n", opIdx + 1, v); return;
    }
    if ((arg = match(param, paramLen, "KRC "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 3)) { Serial.println("ERR: KRC 0-3 (0=-LN 1=-EX 2=+EX 3=+LN)"); return; }
        env.setRightCurve(v); Serial.printf("OK: OP %d KRC %d\n", opIdx + 1, v); return;
    }

    Serial.println("ERR: SET OP <1-6>: LEVEL|WAVE|COARSE|FINE|DETUNE|FIXED|ENABLE|AMS|RS|VS");
    Serial.println("                   R1-R4|L1-L4|KBP|KLD|KRD|KLC|KRC <value>");
}

// =============================================
// SET LFO
// =============================================
static void handleSetLfo(const char* s, uint8_t len) {
    Lfo& lfo = Synth::getInstance().getLfo();
    const char* arg;

    if ((arg = match(s, len, "WAVE "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 5)) { Serial.println("ERR: WAVE 0=tri 1=sawdn 2=sawup 3=sqr 4=sine 5=s&h"); return; }
        lfo.setWave(v); Serial.printf("OK: LFO WAVE %d\n", v); return;
    }
    if ((arg = match(s, len, "SPEED "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: SPEED 0-99"); return; }
        lfo.setSpeed(v); Serial.printf("OK: LFO SPEED %d\n", v); return;
    }
    if ((arg = match(s, len, "DELAY "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: DELAY 0-99"); return; }
        lfo.setDelay(v); Serial.printf("OK: LFO DELAY %d\n", v); return;
    }
    if ((arg = match(s, len, "PMD "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: PMD 0-99"); return; }
        lfo.setPmDepth(v); Serial.printf("OK: LFO PMD %d\n", v); return;
    }
    if ((arg = match(s, len, "AMD "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: AMD 0-99"); return; }
        lfo.setAmDepth(v); Serial.printf("OK: LFO AMD %d\n", v); return;
    }
    if ((arg = match(s, len, "PMS "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 7)) { Serial.println("ERR: PMS 0-7"); return; }
        lfo.setPitchModSens(v); Serial.printf("OK: LFO PMS %d\n", v); return;
    }
    if ((arg = match(s, len, "SYNC "))) {
        bool v; if (!parseBool(arg, v)) { Serial.println("ERR: SYNC 0/1"); return; }
        lfo.setKeySync(v); Serial.printf("OK: LFO SYNC %d\n", (int)v); return;
    }

    Serial.println("ERR: SET LFO WAVE|SPEED|DELAY|PMD|AMD|PMS|SYNC <value>");
}

// =============================================
// SET DELAY / LPF / HPF / CHORUS / REVERB
// =============================================
static void handleSetDelay(const char* s, uint8_t len) {
    Synth& synth = Synth::getInstance();
    const char* arg;

    if ((arg = match(s, len, "ENABLE "))) {
        bool v; if (!parseBool(arg, v)) { Serial.println("ERR: ENABLE 0/1"); return; }
        synth.setDelayEnabled(v); Serial.printf("OK: DELAY ENABLE %d\n", (int)v); return;
    }
    if ((arg = match(s, len, "TIME "))) {
        int t = atoi(arg);
        if (t < 1 || t > 300) { Serial.println("ERR: TIME 1-300 (ms)"); return; }
        synth.getDelay().setTime(t); Serial.printf("OK: DELAY TIME %d\n", t); return;
    }
    if ((arg = match(s, len, "LEVEL "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: LEVEL 0-99"); return; }
        synth.getDelay().setLevel(EffectPreset::toQ15(v));
        Serial.printf("OK: DELAY LEVEL %d\n", v); return;
    }
    if ((arg = match(s, len, "FB "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: FB 0-99"); return; }
        synth.getDelay().setFeedback(EffectPreset::toQ15(v));
        Serial.printf("OK: DELAY FB %d\n", v); return;
    }

    Serial.println("ERR: SET DELAY ENABLE|TIME|LEVEL|FB <value>");
}

static void handleSetLpf(const char* s, uint8_t len) {
    Synth& synth = Synth::getInstance();
    Filter& f = synth.getFilter();
    const char* arg;

    if ((arg = match(s, len, "ENABLE "))) {
        bool v; if (!parseBool(arg, v)) { Serial.println("ERR: ENABLE 0/1"); return; }
        synth.setLpfEnabled(v); Serial.printf("OK: LPF ENABLE %d\n", (int)v); return;
    }
    if ((arg = match(s, len, "CUTOFF "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: CUTOFF 0-99"); return; }
        f.setLowPass(EffectPreset::cutoffToHz(v), f.getLpfResonance());
        Serial.printf("OK: LPF CUTOFF %d\n", v); return;
    }
    if ((arg = match(s, len, "RES "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: RES 0-99"); return; }
        f.setLowPass(f.getLpfCutoff(), EffectPreset::resonanceToQ(v));
        Serial.printf("OK: LPF RES %d\n", v); return;
    }
    if ((arg = match(s, len, "MIX "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: MIX 0-99"); return; }
        f.setLpfMix(EffectPreset::toQ15(v)); Serial.printf("OK: LPF MIX %d\n", v); return;
    }

    Serial.println("ERR: SET LPF ENABLE|CUTOFF|RES|MIX <value>");
}

static void handleSetHpf(const char* s, uint8_t len) {
    Synth& synth = Synth::getInstance();
    Filter& f = synth.getFilter();
    const char* arg;

    if ((arg = match(s, len, "ENABLE "))) {
        bool v; if (!parseBool(arg, v)) { Serial.println("ERR: ENABLE 0/1"); return; }
        synth.setHpfEnabled(v); Serial.printf("OK: HPF ENABLE %d\n", (int)v); return;
    }
    if ((arg = match(s, len, "CUTOFF "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: CUTOFF 0-99"); return; }
        f.setHighPass(EffectPreset::cutoffToHz(v), f.getHpfResonance());
        Serial.printf("OK: HPF CUTOFF %d\n", v); return;
    }
    if ((arg = match(s, len, "RES "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: RES 0-99"); return; }
        f.setHighPass(f.getHpfCutoff(), EffectPreset::resonanceToQ(v));
        Serial.printf("OK: HPF RES %d\n", v); return;
    }
    if ((arg = match(s, len, "MIX "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: MIX 0-99"); return; }
        f.setHpfMix(EffectPreset::toQ15(v)); Serial.printf("OK: HPF MIX %d\n", v); return;
    }

    Serial.println("ERR: SET HPF ENABLE|CUTOFF|RES|MIX <value>");
}

static void handleSetChorus(const char* s, uint8_t len) {
    Synth& synth = Synth::getInstance();
    const char* arg;

    if ((arg = match(s, len, "ENABLE "))) {
        bool v; if (!parseBool(arg, v)) { Serial.println("ERR: ENABLE 0/1"); return; }
        synth.setChorusEnabled(v); Serial.printf("OK: CHORUS ENABLE %d\n", (int)v); return;
    }
    if ((arg = match(s, len, "RATE "))) {
        uint8_t v; if (!parseU8(arg, v, 1, 99)) { Serial.println("ERR: RATE 1-99"); return; }
        synth.getChorus().setRate(v); Serial.printf("OK: CHORUS RATE %d\n", v); return;
    }
    if ((arg = match(s, len, "DEPTH "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: DEPTH 0-99"); return; }
        synth.getChorus().setDepth(v); Serial.printf("OK: CHORUS DEPTH %d\n", v); return;
    }
    if ((arg = match(s, len, "MIX "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: MIX 0-99"); return; }
        synth.getChorus().setMix(EffectPreset::toQ15(v));
        Serial.printf("OK: CHORUS MIX %d\n", v); return;
    }

    Serial.println("ERR: SET CHORUS ENABLE|RATE|DEPTH|MIX <value>");
}

static void handleSetReverb(const char* s, uint8_t len) {
    Synth& synth = Synth::getInstance();
    const char* arg;

    if ((arg = match(s, len, "ENABLE "))) {
        bool v; if (!parseBool(arg, v)) { Serial.println("ERR: ENABLE 0/1"); return; }
        synth.setReverbEnabled(v); Serial.printf("OK: REVERB ENABLE %d\n", (int)v); return;
    }
    if ((arg = match(s, len, "ROOM "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: ROOM 0-99"); return; }
        synth.getReverb().setRoomSize(v); Serial.printf("OK: REVERB ROOM %d\n", v); return;
    }
    if ((arg = match(s, len, "DAMP "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: DAMP 0-99"); return; }
        synth.getReverb().setDamping(v); Serial.printf("OK: REVERB DAMP %d\n", v); return;
    }
    if ((arg = match(s, len, "MIX "))) {
        uint8_t v; if (!parseU8(arg, v, 0, 99)) { Serial.println("ERR: MIX 0-99"); return; }
        synth.getReverb().setMix(EffectPreset::toQ15(v));
        Serial.printf("OK: REVERB MIX %d\n", v); return;
    }

    Serial.println("ERR: SET REVERB ENABLE|ROOM|DAMP|MIX <value>");
}

// =============================================
// GET コマンド
// =============================================
static void handleGetMaster() {
    Synth& synth = Synth::getInstance();
    Serial.printf("MASTER:\n");
    Serial.printf("  PRESET    %d (%s)\n", synth.getCurrentPresetId(), synth.getCurrentPresetName());
    Serial.printf("  ALGO      %d\n", synth.getCurrentAlgorithmId());
    Serial.printf("  FB        %d\n", synth.getFeedbackAmount());
    Serial.printf("  LEVEL     %d\n", static_cast<int>(static_cast<int32_t>(synth.getMasterLevel()) * 127 / Q15_MAX));
    Serial.printf("  TRANSPOSE %d\n", synth.getTranspose());
    Serial.printf("  BEND      %d\n", synth.getPitchBendRange());
    Serial.printf("  VEL       %d\n", static_cast<uint8_t>(synth.getVelocityCurve()));
}

static void handleGetOp(const char* s) {
    Synth& synth = Synth::getInstance();
    if (!s || s[0] < '1' || s[0] > '6') { Serial.println("ERR: GET OP <1-6>"); return; }
    uint8_t opIdx = s[0] - '1';
    const Oscillator& osc = synth.getOperatorOsc(opIdx);
    const Envelope&   env = synth.getOperatorEnv(opIdx);

    Serial.printf("OP %d:\n", opIdx + 1);
    Serial.printf("  ENABLE  %d  WAVE %d  LEVEL %d  COARSE %.0f  FINE %.0f  DETUNE %d  FIXED %d\n",
        (int)osc.isEnabled(), osc.getWavetableId(), osc.getLevel(),
        osc.getCoarse(), osc.getFine(), osc.getDetune(), (int)osc.isFixed());
    Serial.printf("  AMS %d  RS %d  VS %d\n",
        synth.getOperatorAms(opIdx), env.getRateScaling(), env.getVelocitySens());
    Serial.printf("  EG  R1=%d R2=%d R3=%d R4=%d  L1=%d L2=%d L3=%d L4=%d\n",
        env.getRate1(), env.getRate2(), env.getRate3(), env.getRate4(),
        env.getLevel1(), env.getLevel2(), env.getLevel3(), env.getLevel4());
    Serial.printf("  KLS BP=%d LD=%d RD=%d LC=%d RC=%d\n",
        env.getBreakPoint(), env.getLeftDepth(), env.getRightDepth(),
        (uint8_t)env.getLeftCurve(), (uint8_t)env.getRightCurve());
}

static void handleGetLfo() {
    const Lfo& lfo = Synth::getInstance().getLfo();
    Serial.printf("LFO:\n");
    Serial.printf("  WAVE %d  SPEED %d  DELAY %d  PMD %d  AMD %d  PMS %d  SYNC %d\n",
        lfo.getWave(), lfo.getSpeed(), lfo.getDelay(),
        lfo.getPmDepth(), lfo.getAmDepth(), lfo.getPitchModSens(), (int)lfo.getKeySync());
}

static void handleGetFx() {
    Synth& synth = Synth::getInstance();
    Serial.printf("FX:\n");
    Serial.printf("  DELAY  EN=%d TIME=%d LEVEL=%d FB=%d\n",
        (int)synth.isDelayEnabled(), (int)synth.getDelayTime(),
        EffectPreset::fromQ15(synth.getDelayLevel()),
        EffectPreset::fromQ15(synth.getDelayFeedback()));
    Serial.printf("  LPF    EN=%d CUTOFF=%d RES=%d MIX=%d\n",
        (int)synth.isLpfEnabled(),
        EffectPreset::hzToCutoff(synth.getLpfCutoff()),
        EffectPreset::qToResonance(synth.getLpfResonance()),
        EffectPreset::fromQ15(synth.getLpfMix()));
    Serial.printf("  HPF    EN=%d CUTOFF=%d RES=%d MIX=%d\n",
        (int)synth.isHpfEnabled(),
        EffectPreset::hzToCutoff(synth.getHpfCutoff()),
        EffectPreset::qToResonance(synth.getHpfResonance()),
        EffectPreset::fromQ15(synth.getHpfMix()));
    Serial.printf("  CHORUS EN=%d RATE=%d DEPTH=%d MIX=%d\n",
        (int)synth.isChorusEnabled(), synth.getChorusRate(),
        synth.getChorusDepth(), EffectPreset::fromQ15(synth.getChorusMix()));
    Serial.printf("  REVERB EN=%d ROOM=%d DAMP=%d MIX=%d\n",
        (int)synth.isReverbEnabled(), synth.getReverbRoomSize(),
        synth.getReverbDamping(), EffectPreset::fromQ15(synth.getReverbMix()));
}

// =============================================
// HELP
// =============================================
static void printHelp() {
    Serial.println("--- BTN / ENC ---");
    Serial.println("  BTN UP|DN|L|R|ET|CXL|EC [LONG]");
    Serial.println("  ENC <+/-delta>");
    Serial.println("--- SET ---");
    Serial.println("  SET MASTER LEVEL|TRANSPOSE|ALGO|FB|BEND|VEL|PRESET <value>");
    Serial.println("  SET OP <1-6> LEVEL|WAVE|COARSE|FINE|DETUNE|FIXED|ENABLE|AMS|RS|VS <value>");
    Serial.println("  SET OP <1-6> R1-R4|L1-L4 <value>");
    Serial.println("  SET OP <1-6> KBP|KLD|KRD|KLC|KRC <value>");
    Serial.println("  SET LFO WAVE|SPEED|DELAY|PMD|AMD|PMS|SYNC <value>");
    Serial.println("  SET DELAY ENABLE|TIME|LEVEL|FB <value>");
    Serial.println("  SET LPF|HPF ENABLE|CUTOFF|RES|MIX <value>");
    Serial.println("  SET CHORUS ENABLE|RATE|DEPTH|MIX <value>");
    Serial.println("  SET REVERB ENABLE|ROOM|DAMP|MIX <value>");
    Serial.println("--- GET ---");
    Serial.println("  GET MASTER");
    Serial.println("  GET OP <1-6>");
    Serial.println("  GET LFO");
    Serial.println("  GET FX");
}

// =============================================
// メインディスパッチャ
// =============================================
void SerialHandler::executeCommand(const uint8_t* cmd, uint8_t len) {
    const char* s = reinterpret_cast<const char*>(cmd);
    const char* arg;

    // BTN
    if ((arg = match(s, len, "BTN "))) {
        uint8_t argLen = len - 4;
        for (uint8_t i = 0; i < BTN_TABLE_SIZE; ++i) {
            const BtnEntry& b = BTN_TABLE[i];
            uint8_t nameLen = strlen(b.name);
            if (argLen >= nameLen && strncmp(arg, b.name, nameLen) == 0) {
                if (argLen > nameLen && arg[nameLen] != ' ') continue;
                bool isLong = (argLen == nameLen + 5 && strncmp(arg + nameLen, " LONG", 5) == 0);
                if (!state_) { Serial.println("ERR: Not initialized"); return; }
                state_->setBtnState(isLong ? b.id_long : b.id_short);
                Serial.printf("OK: BTN %s %s\n", b.name, isLong ? "LONG" : "SHORT");
                return;
            }
        }
        Serial.println("ERR: Unknown button. Buttons: UP DN L R ET CXL EC");
        return;
    }

    // ENC
    if ((arg = match(s, len, "ENC "))) {
        int delta = atoi(arg);
        if (delta == 0) { Serial.println("ERR: ENC <non-zero integer>"); return; }
        if (!state_) { Serial.println("ERR: Not initialized"); return; }
        state_->addEncoderDelta(static_cast<int16_t>(delta));
        Serial.printf("OK: ENC %+d\n", delta);
        return;
    }

    // SET
    if ((arg = match(s, len, "SET "))) {
        uint8_t argLen = len - 4;
        const char* sub;
        if ((sub = match(arg, argLen, "MASTER ")))       handleSetMaster(sub, argLen - 7);
        else if ((sub = match(arg, argLen, "OP ")))      handleSetOp(sub, argLen - 3);
        else if ((sub = match(arg, argLen, "LFO ")))     handleSetLfo(sub, argLen - 4);
        else if ((sub = match(arg, argLen, "DELAY ")))   handleSetDelay(sub, argLen - 6);
        else if ((sub = match(arg, argLen, "LPF ")))     handleSetLpf(sub, argLen - 4);
        else if ((sub = match(arg, argLen, "HPF ")))     handleSetHpf(sub, argLen - 4);
        else if ((sub = match(arg, argLen, "CHORUS ")))  handleSetChorus(sub, argLen - 7);
        else if ((sub = match(arg, argLen, "REVERB ")))  handleSetReverb(sub, argLen - 7);
        else { Serial.println("ERR: SET MASTER|OP|LFO|DELAY|LPF|HPF|CHORUS|REVERB ..."); return; }
        if (state_) state_->setParamChanged();
        return;
    }

    // GET
    if ((arg = match(s, len, "GET "))) {
        uint8_t argLen = len - 4;
        if (match(arg, argLen, "MASTER"))     handleGetMaster();
        else if (match(arg, argLen, "OP "))   handleGetOp(arg + 3);
        else if (match(arg, argLen, "LFO"))   handleGetLfo();
        else if (match(arg, argLen, "FX"))    handleGetFx();
        else Serial.println("ERR: GET MASTER|OP <1-6>|LFO|FX");
        return;
    }

    // HELP
    if (match(s, len, "HELP") || match(s, len, "help") || match(s, len, "?")) {
        printHelp(); return;
    }

    Serial.println("ERR: Unknown command. Type HELP for usage.");
}
