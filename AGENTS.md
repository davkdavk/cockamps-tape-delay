# AGENTS.md — Tape Delay VST3

---

## READ THIS FIRST — WHAT TO DO RIGHT NOW

You have just been pointed at this file. Follow these steps in order before doing
anything else. Do not ask for confirmation on any of these steps — just do them.

### Step 1 — Clone JUCE
JUCE must live one directory above this repo. Clone it now:
```bash
git clone --branch 7.0.9 --depth 1 https://github.com/juce-framework/JUCE.git ../JUCE
```

### Step 2 — Install system dependencies
```bash
sudo apt install build-essential cmake ninja-build pkg-config \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxext-dev \
    libfreetype6-dev libasound2-dev libfontconfig1-dev libgl1-mesa-dev
```

### Step 3 — Build the plugin
```bash
chmod +x build-linux.sh && ./build-linux.sh
```
Expected output: a file ending in `TapeDelay.vst3` inside the `build/` directory.
If the build fails, read the error, fix it, and rebuild before doing anything else.

### Step 4 — Install into Reaper's plugin folder
```bash
mkdir -p ~/.vst3
cp -r build/**/TapeDelay.vst3 ~/.vst3/
```

### Step 5 — Confirm and report back
Tell the user:
- Whether the build succeeded or failed
- The full path to the installed `.vst3`
- Any warnings from the compiler worth knowing about

Only after Step 5 is complete should you take further instructions from the user.

---

## Your role

You are the sole developer on this codebase. You write, refactor, debug, and build
C++ JUCE VST3 plugin code. After the initial setup above is done, work autonomously
on whatever the user asks. Do not ask for confirmation on routine edits. Only pause
if a change would be irreversible or break existing DAW sessions (e.g. changing
plugin IDs, restructuring the parameter tree incompatibly).

When in doubt about DSP correctness, prefer the conservative option and leave a
`// TODO:` comment explaining the tradeoff.

After every change that touches DSP or build files, rebuild and reinstall:
```bash
./build-linux.sh && cp -r build/**/TapeDelay.vst3 ~/.vst3/
```

---

## What this plugin is

A VST3 guitar delay plugin for Linux (and Windows). Two switchable engines:

- **Tape** — reel-to-reel tape machine: wow, flutter, saturation, HF loss, pink noise, ping-pong
- **BBD** — bucket-brigade device: bandwidth-limited filters, clock noise, compander, mod LFO

Target: **Linux (x86_64) and Windows 10/11 (x64). No macOS. No AU format.**

Framework: JUCE 7.0.9 — do not upgrade without instruction.
Build system: CMake 3.22+ via `build-linux.sh`. Never use Projucer.

---

## Directory map

```
TapeDelayVST/
├── AGENTS.md                    ← you are here
├── CMakeLists.txt               ← all build logic; do not add AU format
├── README.md                    ← keep in sync when you change features
├── build-linux.sh               ← use this to build every time
├── build-windows.ps1            ← Windows equivalent
├── .github/workflows/build.yml  ← CI for both platforms
└── Source/
    ├── TapeDelay.h / .cpp       ← Tape DSP engine
    ├── BBDDelay.h / .cpp        ← BBD DSP engine
    ├── PluginProcessor.h / .cpp ← owns both engines + all parameters
    └── PluginEditor.h / .cpp    ← UI: custom knobs, vintage look and feel
```

---

## Parameters

All parameters are in `TapeDelayProcessor::createParameterLayout()`.
Read them in `updateEngineParams()` using:
```cpp
float val = apvts.getRawParameterValue("paramID")->load();
```
Never use `getParameter()->getValue()` on the audio thread — it is not atomic.

| APVTS ID     | Range        | Engine  | Default |
|--------------|--------------|---------|---------|
| `delayTime`  | 10–2000 ms   | both    | 250 ms  |
| `feedback`   | 0.0–0.95     | both    | 0.40    |
| `mix`        | 0.0–1.0      | both    | 0.50    |
| `wow`        | 0.0–1.0      | Tape    | 0.25    |
| `flutter`    | 0.0–1.0      | Tape    | 0.20    |
| `saturation` | 0.0–1.0      | Tape    | 0.40    |
| `hfLoss`     | 0.0–1.0      | Tape    | 0.40    |
| `noiseLevel` | 0.0–1.0      | Tape    | 0.10    |
| `pingPong`   | bool         | Tape    | false   |
| `clockNoise` | 0.0–1.0      | BBD     | 0.20    |
| `compander`  | 0.0–1.0      | BBD     | 0.30    |
| `modDepth`   | 0.0–1.0      | BBD     | 0.15    |
| `modRate`    | 0.0–1.0      | BBD     | 0.30    |
| `mode`       | 0=Tape 1=BBD | —       | 0       |

---

## DSP: TapeDelay

**Key internals:**
- Circular stereo buffer `delayBuffer[2]`, size `kMaxDelaySamples = 192000 * 2`
- Single shared `writePos` for both channels — increments at the END of the sample loop
- `readHermite()` — 4-point Catmull-Rom interpolation for sub-sample reads
- Wow: two-partial sine LFO (~0.5 Hz + ~1.1 Hz), depth = baseDelay * 0.015 * wow
- Flutter: two-partial sine LFO (~8 Hz + ~11 Hz), depth = baseDelay * 0.004 * flutter
- LFO phases stored as `double` — do not change to float (pitch drift over time)
- Saturation: `(0.6*tanh(x*drive) + 0.4*atan(x*drive*1.3)) * asymFactor / drive`
  applied on the feedback path only, not the dry input
- HF loss: 1-pole IIR, cutoff = `18000 * (1-hfLoss)^2.5 + 200` Hz, state in `hfState[2]`
- Noise: Paul Kellet pink filter, scaled by `noiseLevel * 0.003`, injected on feedback

## DSP: BBDDelay

**Key internals:**
- Circular stereo buffer `delayBuf[2]`, size `kMaxDelaySamples = 192000 / 2` (500 ms max)
- Delay time clamped to 500 ms in `updateEngineParams()`
- `readLinear()` — linear interpolation (Hermite not needed due to heavy bandlimiting)
- 4 biquad stages per channel: 2 input LPF + 2 output LPF at ~3.4 kHz
- `makeLPF()` called once in `prepare()` — never call it inside the sample loop
- Compander: RMS encode (attack 5 ms / release 50 ms) + decode (20 ms / 200 ms)
- Clock noise: white noise * `clockNoise * 0.008` written into delay buffer
- Mod LFO: sine at `0.1 + modRate*4.9` Hz, depth = `baseDelay * modDepth * 0.02`

---

## How to add a parameter (full steps)

1. `TapeDelay.h` or `BBDDelay.h` — add public float field with default
2. `PluginProcessor.cpp` → `createParameterLayout()` — register it
3. `PluginProcessor.cpp` → `updateEngineParams()` — push it to the engine
4. `TapeDelay.cpp` or `BBDDelay.cpp` → `process()` — use it in DSP
5. `PluginEditor.h` — add a `KnobRow` member
6. `PluginEditor.cpp` — `addAndMakeVisible`, `setBounds` in `resized()`,
   `setVisible(isTape)` or `setVisible(!isTape)` in `updateVisibility()`

---

## Hard rules — never break these

| Rule | Consequence if broken |
|---|---|
| Keep `-fvisibility=hidden` in CMakeLists.txt Linux block | Symbol collisions crash Reaper when loading multiple plugins |
| Keep `NOMINMAX` in Windows block | `windows.h` breaks `std::min`/`std::max` |
| Keep `FORMATS VST3` only — no AU | AU requires macOS SDK, breaks Linux/Windows build |
| Never change `PLUGIN_CODE Tdvs` or `PLUGIN_MANUFACTURER_CODE Ystu` | Reaper treats it as a new plugin, loses all session data |
| Never call `makeLPF()` inside the per-sample loop | `sin()`/`exp()` per sample kills CPU |
| Never advance `writePos` inside the per-channel loop | Stereo channels drift apart |
| Never use `std::cout` or `printf` | Use `DBG()` only — strips to no-op in release |
| Never add `juce::dsp::` deps inside engine classes | Breaks engine testability |

---

## UI reference

- Fixed window: 640 × 380 px — change `setSize()` and all `setBounds()` together
- Common controls (Delay, Feedback, Mix): x=8–236
- Tape controls (Wow, Flutter, Sat, HF Loss, Noise, PingPong): x=238–390
- BBD controls (ClockNoise, Compander, ModDepth, ModRate): x=392–632
- Mode polling: 100 ms `juce::Timer` calls `updateVisibility()` — intentional, no listener needed
- Colour accent: gold `0xFFE8B85A`, background `0xFF0C0906`, font: Courier New bold
