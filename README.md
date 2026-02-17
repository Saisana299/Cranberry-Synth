<h1 align="center">Cranberry Synth</h1>

<img src="https://storage.googleapis.com/zenn-user-upload/c3e1dd264522-20260217.jpg">

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

- **6-Operator FM Synthesis** – 32 algorithms with configurable feedback
- **Wavetable Oscillators** – Sine, Triangle, Saw, Square (512 samples each)
- **16-Voice Polyphony** – 16 simultaneous notes across 16 MIDI channels
- **4-Rate / 4-Level Envelopes** – Per-operator with rate scaling and keyboard level scaling
- **LFO** – 6 waveforms, pitch/amplitude modulation, per-operator AMS, key sync
- **Real-time Effects** – Delay (300ms), Biquad LPF/HPF
- **Passthrough Mode** – External audio input (PCM1802 ADC) with effects processing
- **OLED Display** – 128×128 16-bit RGB (SSD1351) @ 15 FPS
- **MIDI Support** – Hardware MIDI IN (Serial7), USB MIDI, SMF playback from SD card

---

## Hardware

### Requirements

| Component | Specification |
|:---|:---|
| MCU | Teensy 4.1 (ARM Cortex-M7 @ 600MHz) |
| Display | WaveShare 1.5" RGB OLED (SSD1351, 128×128) |
| Audio Output | I2S Quad DAC (PCM5102A or compatible, differential 4ch) |
| Audio Input | PCM1802 ADC (for passthrough mode) |
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
| 8 | RX (I2S Input) |
| 7 | TX1/2 (L+, L-) |
| 32 | TX3/4 (R+, R-) |

</details>

<details>
<summary><b>User Input</b></summary>

| Pin | Function |
|:---:|:---|
| 15 | Encoder A |
| 17 | Encoder B |
| 16 | Encoder SW |
| 37 | UP |
| 36 | DOWN |
| 35 | LEFT |
| 34 | RIGHT |
| 22 | ENTER |
| 14 | CANCEL |

</details>

<details>
<summary><b>LEDs</b></summary>

| Pin | Function |
|:---:|:---|
| 6 | MIDI LED |
| 9 | Audio LED |
| 30 | Power LED |
| 31 | Status LED |

</details>

---

## Project Structure

```
Cranberry-Synth/
├── include/
│   ├── display/        # OLED & LED control
│   ├── handlers/       # Audio, MIDI, Input handlers
│   ├── modules/        # Synth engine (oscillator, envelope, filter, delay, LFO)
│   ├── tools/          # MIDI file player
│   ├── ui/             # UI manager & screens
│   └── utils/          # Algorithms, presets, wavetables
├── src/                # Implementation files
├── lib/MD_MIDIFile/    # MIDI file library
└── platformio.ini
```

---

## Parameters

### Master

| Parameter | Range | Description |
|:---|:---:|:---|
| Level | 0-100 | Master output level (default: 71 ≈ -3dB) |
| Transpose | -24 – +24 | Transpose in semitones |
| Feedback | 0-7 | Operator feedback amount |

### Operator

| Parameter | Range | Description |
|:---|:---:|:---|
| Enabled | ON/OFF | Operator enable/disable |
| Wavetable | 0-3 | Sine, Triangle, Saw, Square |
| Level | 0-99 | Output level (non-linear) |
| Mode | RATIO/FIXED | Frequency mode |
| Coarse | 0-31 | Frequency ratio (0 = 0.5×) |
| Fine | 0-99 | Fine tuning (ratio × (1 + fine/100)) |
| Detune | -7 – +7 | Detune in cents |
| AMS | 0-3 | Amplitude modulation sensitivity |
| Velocity Sens | 0-7 | Velocity sensitivity |

### Envelope (4-Rate / 4-Level)

| Parameter | Range | Description |
|:---|:---:|:---|
| Rate 1 | 0-99 | Attack rate |
| Rate 2 | 0-99 | Decay 1 rate |
| Rate 3 | 0-99 | Decay 2 rate |
| Rate 4 | 0-99 | Release rate |
| Level 1 | 0-99 | Attack target level |
| Level 2 | 0-99 | Decay 1 target level |
| Level 3 | 0-99 | Sustain level |
| Level 4 | 0-99 | Release target level |
| Rate Scaling | 0-7 | Higher notes → faster envelopes |

### Keyboard Level Scaling

| Parameter | Range | Description |
|:---|:---:|:---|
| Break Point | 0-99 | Reference note (MIDI = value + 17) |
| Left Depth | 0-99 | Scaling depth below break point |
| Right Depth | 0-99 | Scaling depth above break point |
| Left Curve | 0-3 | -LN, -EX, +EX, +LN |
| Right Curve | 0-3 | -LN, -EX, +EX, +LN |

### LFO

| Parameter | Range | Description |
|:---|:---:|:---|
| Wave | 0-5 | Triangle, SawDown, SawUp, Square, Sine, Sample&Hold |
| Speed | 0-99 | LFO rate (~0.06–50 Hz) |
| Delay | 0-99 | Fade-in time (0–5 sec) |
| PM Depth | 0-99 | Pitch modulation depth |
| AM Depth | 0-99 | Amplitude modulation depth |
| P.M. Sens | 0-7 | Pitch modulation sensitivity |
| Key Sync | ON/OFF | Reset LFO phase on note-on |
| OSC Key Sync | ON/OFF | Reset oscillator phase on note-on |

### Effects

| Effect | Parameters |
|:---|:---|
| Delay | Time (1-300ms), Level (0-100%), Feedback (0-100%) |
| LPF | Cutoff (20-20kHz), Resonance (0.1-10.0), Mix (0-100%) |
| HPF | Cutoff (100-20kHz), Resonance (0.1-10.0), Mix (0-100%) |

---

## Specifications

| Item | Spec |
|:---|:---|
| Sample Rate | 44,100 Hz |
| Bit Depth | 16-bit output / 24-bit internal (Q23) |
| Latency | ~2.9 ms (128 samples) |
| Operators | 6 |
| Algorithms | 32 |
| Polyphony | 16 voices |
| MIDI Channels | 16 |
| LFO Waveforms | 6 |
| Display | 128×128 RGB @ 15 FPS |

---

## License

GNU General Public License v3.0 - see [LICENSE](LICENSE)

---

## Acknowledgments

- [PJRC](https://www.pjrc.com/) – Teensy & Audio Library
- [Adafruit](https://www.adafruit.com/) – GFX Libraries
- [MajicDesigns](https://github.com/MajicDesigns/MD_MIDIFile) – MIDI File Library
- [Dexed](https://github.com/asb2m10/dexed) – Reference for FM synthesis algorithms (envelope, feedback, key scaling, LFO)

---

<p align="center">
  Made by <a href="https://github.com/Saisana299">Saisana299</a>
</p>
