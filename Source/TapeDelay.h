#pragma once

#include <JuceHeader.h>
#include <random>
#include <vector>

class TapeDelay
{
public:
    void prepare(double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);

    float delayTimeMs = 250.0f;
    float feedback = 0.40f;
    float mix = 0.50f;
    float wow = 0.25f;
    float flutter = 0.20f;
    float saturation = 0.40f;
    float hfLoss = 0.40f;
    float noiseLevel = 0.10f;
    bool pingPong = false;

private:
    static constexpr int kMaxDelaySamples = 192000 * 2;

    float readHermite(int ch, float delaySamples) const noexcept;
    float saturate(float x, float drive) const noexcept;
    float pinkNoise(int ch) noexcept;

    double sampleRate = 48000.0;
    int numChannels = 2;
    int writePos = 0;

    std::vector<float> delayBuffer[2];
    float hfState[2] { 0.0f, 0.0f };
    float pinkState[2] { 0.0f, 0.0f };

    double wowPhase = 0.0;
    double wowPhase2 = 0.0;
    double flutterPhase = 0.0;
    double flutterPhase2 = 0.0;

    std::mt19937 rng { 0x12345678u };
    std::normal_distribution<float> noiseDist { 0.0f, 1.0f };
};
