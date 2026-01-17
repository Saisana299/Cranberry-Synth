<h1 align="center">Cranberry Synth</h1>

<p align="center">
  <strong>6-Operator FM Synthesizer on Teensy 4.1</strong>
</p>

<p align="center">
  Successor to <a href="https://github.com/Saisana299/RP-DS16">RP-DS16</a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Platform-Teensy%204.1-E91E63?style=flat-square&logo=arduino" alt="Platform"/>
  <img src="https://img.shields.io/badge/Framework-Arduino-00979D?style=flat-square&logo=arduino" alt="Framework"/>
  <img src="https://img.shields.io/badge/License-GPL--3.0-mossgreen?style=flat-square" alt="License"/>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/44.1kHz-16bit-blue?style=flat-square" alt="Audio"/>
  <img src="https://img.shields.io/badge/Polyphony-16-blue?style=flat-square" alt="Polyphony"/>
  <img src="https://img.shields.io/badge/Operators-6-blue?style=flat-square" alt="Operators"/>
</p>

---

## Features

- **6-Operator FM Synthesis** – DX7-style FM engine with configurable algorithms
- **Wavetable Oscillators** – Sine, Triangle, Saw, Square
- **16-Voice Polyphony** – 16 simultaneous notes across 16 MIDI channels
- **Real-time Effects** – Delay (300ms), Biquad LPF/HPF
- **ADSR Envelopes** – Per-operator with exponential curves
- **OLED Display** – 128×128 RGB (SSD1351)
- **MIDI Support** – Hardware MIDI IN, USB MIDI, SMF playback from SD card

---

## Hardware

### Requirements

| Component | Specification |
|:---|:---|
| MCU | Teensy 4.1 (ARM Cortex-M7 @ 600MHz) |
| Display | WaveShare 1.5" OLED (SSD1351, 128×128) |
| Audio | I2S Quad DAC (PCM5102A or compatible) |
| Input | Rotary encoder + 6 buttons |
| Storage | microSD card (for MIDI files) |

### Pin Configuration

<details>
<summary><b>Display (SPI1)</b></summary>

| Pin | Function |
|:---:|:---|
| 26 | MOSI |
| 27 | SCK |
| 38 | CS |
| 40 | DC |
| 41 | RST |

</details>

<details>
<summary><b>Audio (I2S Quad)</b></summary>

| Pin | Function |
|:---:|:---|
| 21 | BCLK |
| 23 | MCLK |
| 20 | LRCLK |
| 7 | TX1/2 (L+, L-) |
| 32 | TX3/4 (R+, R-) |

</details>

<details>
<summary><b>User Input</b></summary>

| Pin | Function |
|:---:|:---|
| 15, 17 | Encoder A, B |
| 16 | Encoder SW |
| 2, 3, 4, 5 | Direction buttons |
| 18, 19 | Cancel, Enter |
| 29, 34 | Audio/MIDI LEDs |

</details>

---

## Project Structure

```
Cranberry-Synth/
├── include/
│   ├── display/        # OLED & LED control
│   ├── handlers/       # Audio, MIDI, Input handlers
│   ├── modules/        # Synth engine (oscillator, envelope, filter, delay)
│   ├── tools/          # MIDI file player
│   ├── ui/             # UI manager & screens
│   └── utils/          # Algorithms, presets, wavetables
├── src/                # Implementation files
├── lib/MD_MIDIFile/    # MIDI file library
└── platformio.ini
```

---

## Parameters

### Operator

| Parameter | Range | Description |
|:---|:---:|:---|
| Wavetable | 0-3 | Sine, Triangle, Saw, Square |
| Level | 0-99 | Output level (non-linear) |
| Coarse | 0-31 | Frequency ratio (integer) |
| Fine | 0-99 | Frequency ratio (decimal) |
| Detune | ±7 | Detune in cents |

### Envelope (ADSR)

| Parameter | Range |
|:---|:---:|
| Attack | 0-99 |
| Decay | 0-99 |
| Sustain | 0-99 |
| Release | 0-99 |

### Effects

| Effect | Parameters |
|:---|:---|
| Delay | Time (1-300ms), Level, Feedback |
| LPF/HPF | Cutoff (20-20kHz), Resonance, Mix |

---

## Specifications

| Item | Spec |
|:---|:---|
| Sample Rate | 44,100 Hz |
| Bit Depth | 16-bit |
| Latency | ~2.9 ms |
| Operators | 6 |
| Polyphony | 16 voices |
| MIDI Channels | 16 |
| Display | 128×128 @ 15 FPS |

---

## License

GNU General Public License v3.0 - see [LICENSE](LICENSE)

---

## Acknowledgments

- [PJRC](https://www.pjrc.com/) – Teensy & Audio Library
- [Adafruit](https://www.adafruit.com/) – GFX Libraries
- [MajicDesigns](https://github.com/MajicDesigns/MD_MIDIFile) – MIDI File Library
- [Dexed](https://github.com/asb2m10/dexed) – DX7 compatible FM synthesis algorithms (envelope, feedback, key scaling)

---

<p align="center">
  Made by <a href="https://github.com/Saisana299">Saisana299</a>
</p>
