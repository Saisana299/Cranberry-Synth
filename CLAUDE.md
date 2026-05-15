# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Cranberry Synth is a 6-operator FM synthesizer firmware for the **Teensy 4.1** microcontroller (ARM Cortex-M7 @ 600MHz). It produces real-time FM synthesis with 16-voice polyphony, a 128×128 OLED display UI, full MIDI support, and an effects chain (delay, filter, chorus, reverb).

## Build System

This project uses **PlatformIO** (not CMake or Make).

```bash
pio run -e teensy41                      # Build firmware
pio run -e teensy41 --target upload      # Build and upload to device
pio device monitor --speed 115200        # Open serial monitor
```

There are currently no implemented tests (the `test/` directory contains only a placeholder README).

## Architecture

### Signal Flow

```
MIDI input → Voice allocation (Synth) → Operator graph (FM algorithm)
           → Effects chain (Delay → Filter → Chorus → Reverb)
           → I2S output (AudioHandler)
```

Passthrough mode bypasses FM synthesis and routes the PCM1802 ADC input through the same effects chain.

### Priority System (lower = higher priority)

| Priority | Work |
|----------|------|
| 0 | Audio sample generation (`synth.update()`) + MIDI/USB processing during SPI |
| 1 | I2S AD/DA audio I/O (`AudioHandler`) |
| 2 | MIDI serial input detection |
| 3 | Physical encoder/button input |
| 4 | OLED UI rendering (15 FPS) |
| 5 | MIDI file player (SD card) |
| 6 | Serial/USB communication |
| 7 | LED control |

### Key Modules

**Core engine** (`include/modules/`, `src/modules/`):
- `synth` — Voice allocator; owns 16 voices, selects FM algorithm, manages feedback and global modulation
- `oscillator` — Wavetable oscillator (sine, triangle, saw, square) using a phase accumulator
- `envelope` — 4-rate/4-level EG with rate scaling and keyboard level scaling
- `lfo` — 6 waveforms, pitch/amplitude modulation, key sync

**Effects** (same directories):
- `delay`, `filter` (biquad LPF/HPF), `chorus` (stereo), `reverb` (Freeverb — 8 comb + 4 allpass)
- `passthrough` — wraps the effects chain for audio-input mode

**Handlers** (`include/handlers/`, `src/handlers/`):
- `audio` — Teensy Audio Library integration (I2S)
- `midi` — Hardware serial and USB MIDI
- `physical` — Rotary encoder and 6 buttons
- `serial` — USB/serial communication

**UI** (`include/ui/`, `include/display/`):
- `UIManager` (`ui.hpp`) — Screen-stack navigator, renders at 15 FPS
- Each screen in `include/ui/screens/` handles a specific parameter group (operator, envelope, LFO, effects, etc.)
- `gfx.hpp` — Thin wrapper around the Adafruit SSD1351 driver for the 128×128 RGB OLED

**Utilities** (`include/utils/`):
- `types.hpp` — Fixed-point type aliases: `Q23`, `Q15`, `Q31`, `Phase_t`
- `state.hpp` — Global state struct (all synth parameters, CPU usage, current mode)
- `preset.hpp` — 10 built-in presets + random patch generation
- `algorithm.hpp` — 32 FM operator routing algorithms
- `math.hpp` — Fixed-point arithmetic helpers

### Fixed-Point Arithmetic

All audio math uses fixed-point (no floating-point in the DSP hot path). The primary internal format is **Q23** (24-bit signed, 8 integer + 23 fractional bits). Use helpers in `include/utils/math.hpp` for multiplication and conversion rather than raw shifts.

### Memory and Performance Notes

- Build flags `-O3 -ffast-math -fomit-frame-pointer` and LTO (`-D TEENSY_OPT_FASTEST_LTO`) are critical for meeting real-time deadlines at 44.1 kHz / 128-sample blocks (~2.9 ms latency).
- Avoid dynamic allocation (`new`/`malloc`) in the audio path; all voice and buffer memory is statically allocated.
- `extra_script.py` configures PlatformIO's LTO partitioning — do not remove it.
