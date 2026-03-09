#include "BBDDelay.h"

#include <algorithm>
#include <cmath>

void BBDDelay::prepare(double newSampleRate, int, int newNumChannels)
{
    sampleRate = newSampleRate;
    numChannels = juce::jmin(2, newNumChannels);
    writePos = 0;

    for (int ch = 0; ch < 2; ++ch)
        delayBuf[ch].assign(kMaxDelaySamples, 0.0f);

    makeLPF(3400.0f, 0.85f, lpfCoeffs[0]);
    makeLPF(3400.0f, 0.65f, lpfCoeffs[1]);

    reset();
}

void BBDDelay::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        encRms[ch] = 0.0f;
        decRms[ch] = 0.0f;
        for (int i = 0; i < 4; ++i)
            filterState[ch][i] = {};

        if (!delayBuf[ch].empty())
            std::fill(delayBuf[ch].begin(), delayBuf[ch].end(), 0.0f);
    }

    modPhase = 0.0;
}

void BBDDelay::makeLPF(float cutoff, float q, BiquadCoeffs& coeffs) const noexcept
{
    const float omega = 2.0f * juce::MathConstants<float>::pi * cutoff / (float) sampleRate;
    const float sinO = std::sin(omega);
    const float cosO = std::cos(omega);
    const float alpha = sinO / (2.0f * q);

    const float b0 = (1.0f - cosO) * 0.5f;
    const float b1 = 1.0f - cosO;
    const float b2 = (1.0f - cosO) * 0.5f;
    const float a0 = 1.0f + alpha;
    const float a1 = -2.0f * cosO;
    const float a2 = 1.0f - alpha;

    coeffs.b0 = b0 / a0;
    coeffs.b1 = b1 / a0;
    coeffs.b2 = b2 / a0;
    coeffs.a1 = a1 / a0;
    coeffs.a2 = a2 / a0;
}

float BBDDelay::processBiquad(float x, BiquadState& state, const BiquadCoeffs& coeffs) const noexcept
{
    const float y = coeffs.b0 * x + state.s1;
    state.s1 = coeffs.b1 * x - coeffs.a1 * y + state.s2;
    state.s2 = coeffs.b2 * x - coeffs.a2 * y;
    return y;
}

float BBDDelay::readLinear(int ch, float delaySamples) const noexcept
{
    const int bufSize = (int) delayBuf[ch].size();
    const int intDelay = (int) delaySamples;
    const float frac = delaySamples - (float) intDelay;

    int idx0 = writePos - intDelay;
    while (idx0 < 0)
        idx0 += bufSize;
    idx0 %= bufSize;

    int idx1 = idx0 + 1;
    if (idx1 >= bufSize)
        idx1 -= bufSize;

    const float y0 = delayBuf[ch][idx0];
    const float y1 = delayBuf[ch][idx1];
    return y0 + frac * (y1 - y0);
}

float BBDDelay::compand(float x, float& rms, float attackMs, float releaseMs) const noexcept
{
    const float x2 = x * x;
    const float timeMs = (x2 > rms) ? attackMs : releaseMs;
    const float coeff = std::exp(-1.0f / (0.001f * timeMs * (float) sampleRate));
    rms = coeff * rms + (1.0f - coeff) * x2;

    const float env = std::sqrt(rms + 1.0e-8f);
    return 1.0f / juce::jmax(env, 1.0e-4f);
}

void BBDDelay::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int bufSize = (int) delayBuf[0].size();

    const float baseDelaySamples = juce::jlimit(10.0f, 500.0f, delayTimeMs) * (float) sampleRate / 1000.0f;
    const float depthSamples = baseDelaySamples * modDepth * 0.02f;
    const float modRateHz = 0.1f + modRate * 4.9f;
    const double modInc = juce::MathConstants<double>::twoPi * modRateHz / sampleRate;

    const float wet = mix;
    const float dry = 1.0f - mix;

    for (int i = 0; i < numSamples; ++i)
    {
        const float mod = std::sin((float) modPhase);
        const float delaySamples = baseDelaySamples + depthSamples * mod;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float input = buffer.getSample(ch, i);

            float filtered = processBiquad(input, filterState[ch][0], lpfCoeffs[0]);
            filtered = processBiquad(filtered, filterState[ch][1], lpfCoeffs[0]);

            const float encGain = compand(filtered, encRms[ch], 5.0f, 50.0f);
            const float encoded = filtered + compander * (filtered * encGain - filtered);

            float delayed = readLinear(ch, delaySamples);
            delayed = processBiquad(delayed, filterState[ch][2], lpfCoeffs[1]);
            delayed = processBiquad(delayed, filterState[ch][3], lpfCoeffs[1]);

            const float decGain = compand(delayed, decRms[ch], 20.0f, 200.0f);
            const float expanded = delayed + compander * (delayed / juce::jmax(decGain, 1.0e-4f) - delayed);

            const float out = dry * input + wet * expanded;
            buffer.setSample(ch, i, out);

            const float noise = noiseDist(rng) * (clockNoise * 0.008f);
            const float feedbackSample = expanded * feedback;
            delayBuf[ch][writePos] = encoded + feedbackSample + noise;
        }

        ++writePos;
        if (writePos >= bufSize)
            writePos = 0;

        modPhase += modInc;
        if (modPhase >= juce::MathConstants<double>::twoPi)
            modPhase -= juce::MathConstants<double>::twoPi;
    }
}
