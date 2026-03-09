#pragma once

#include <JuceHeader.h>
#include <random>
#include <vector>

class BBDDelay
{
public:
    void prepare(double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);

    float delayTimeMs = 250.0f;
    float feedback = 0.40f;
    float mix = 0.50f;
    float clockNoise = 0.20f;
    float compander = 0.30f;
    float modDepth = 0.15f;
    float modRate = 0.30f;

private:
    static constexpr int kMaxDelaySamples = 192000 / 2;

    struct BiquadState
    {
        float s1 = 0.0f;
        float s2 = 0.0f;
    };

    struct BiquadCoeffs
    {
        float b0 = 0.0f;
        float b1 = 0.0f;
        float b2 = 0.0f;
        float a1 = 0.0f;
        float a2 = 0.0f;
    };

    void makeLPF(float cutoff, float q, BiquadCoeffs& coeffs) const noexcept;
    float processBiquad(float x, BiquadState& state, const BiquadCoeffs& coeffs) const noexcept;
    float readLinear(int ch, float delaySamples) const noexcept;
    float compand(float x, float& rms, float attackMs, float releaseMs) const noexcept;

    double sampleRate = 48000.0;
    int numChannels = 2;
    int writePos = 0;

    std::vector<float> delayBuf[2];
    BiquadState filterState[2][4] {};
    BiquadCoeffs lpfCoeffs[2] {};

    float encRms[2] { 0.0f, 0.0f };
    float decRms[2] { 0.0f, 0.0f };

    double modPhase = 0.0;

    std::mt19937 rng { 0x87654321u };
    std::normal_distribution<float> noiseDist { 0.0f, 1.0f };
};
