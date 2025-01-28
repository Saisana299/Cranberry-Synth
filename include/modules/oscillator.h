#pragma once

#include "handlers/audio.h"
#include "modules/envelope.h"
#include "utils/math.h"
#include "utils/wavetable.h"

class Oscillator {
public:
    struct Memory {
        float phase;
        float delta;
        float vel_vol;
    };

    Oscillator() {
        init();
    }

    void setFrequency(Memory& mem, uint8_t note);
    void setFrequency(Memory& mem, float freq);
    void setVolume(Memory& mem, uint8_t velocity);
    void setPhase(Memory& mem, float phase);
    int16_t getSample(Memory& mem);
    void update(Memory& mem, Memory* mod_mem = nullptr, Envelope::Memory* mod_env_mem = nullptr);
    void reset(Memory& mem);
    void enable();
    void disable();
    bool isActive();
    void setModulation(uint8_t id, Oscillator* mod_osc, Envelope* mod_env);
    void setLoopback(bool loopback);

private:
    int16_t* wavetable;
    size_t wavetable_size;
    bool enabled;
    bool loopback;
    Oscillator* mod_osc;
    Envelope* mod_env;
    uint8_t mod_id;
    void init();
};