# TapeDelayVST

VST3 tape delay plugin with two switchable engines (Tape and BBD). Targets Linux x86_64
and Windows x64. macOS and AU are intentionally unsupported.

## Build (Linux)

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DJUCE_PATH=$(realpath ../JUCE)
cmake --build build --parallel
```

## Build (Windows)

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_PATH="..\JUCE"
cmake --build build --config Release --parallel
```

Output: `build/**/TapeDelay.vst3`

## Parameters

| APVTS ID     | Range        | Engine  | Default |
|--------------|--------------|---------|---------|
| delayTime    | 10-2400 ms   | both    | 250 ms  |
| feedback     | 0.0-0.95     | both    | 0.40    |
| mix          | 0.0-1.0      | both    | 0.50    |
| wow          | 0.0-1.0      | Tape    | 0.25    |
| flutter      | 0.0-1.0      | Tape    | 0.20    |
| saturation   | 0.0-1.0      | Tape    | 0.40    |
| hfLoss       | 0.0-1.0      | Tape    | 0.40    |
| noiseLevel   | 0.0-1.0      | Tape    | 0.10    |
| pingPong     | bool         | Tape    | false   |
| clockNoise   | 0.0-1.0      | BBD     | 0.20    |
| compander    | 0.0-1.0      | BBD     | 0.30    |
| modDepth     | 0.0-1.0      | BBD     | 0.15    |
| modRate      | 0.0-1.0      | BBD     | 0.30    |
| mode         | 0=Tape 1=BBD | -       | 0       |
