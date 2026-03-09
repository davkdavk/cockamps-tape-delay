#include "TapeDelay.h"

#include <algorithm>
#include <cmath>

void TapeDelay::prepare(double newSampleRate, int, int newNumChannels)
{
    sampleRate = newSampleRate;
    numChannels = juce::jmin(2, newNumChannels);
    writePos = 0;

    for (int ch = 0; ch < 2; ++ch)
        delayBuffer[ch].assign(kMaxDelaySamples, 0.0f);

    reset();
}

void TapeDelay::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        hfState[ch] = 0.0f;
        pinkState[ch] = 0.0f;
        if (!delayBuffer[ch].empty())
            std::fill(delayBuffer[ch].begin(), delayBuffer[ch].end(), 0.0f);
    }

    wowPhase = 0.0;
    wowPhase2 = 0.0;
    flutterPhase = 0.0;
    flutterPhase2 = 0.0;
}

float TapeDelay::readHermite(int ch, float delaySamples) const noexcept
{
    const int bufSize = (int) delayBuffer[ch].size();
    const int intDelay = (int) delaySamples;
    const float frac = delaySamples - (float) intDelay;

    auto readAt = [&](int offset) -> float
    {
        int idx = writePos - intDelay + offset;
        while (idx < 0)
            idx += bufSize;
        idx %= bufSize;
        return delayBuffer[ch][idx];
    };

    const float y0 = readAt(-1);
    const float y1 = readAt(0);
    const float y2 = readAt(1);
    const float y3 = readAt(2);

    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

float TapeDelay::saturate(float x, float drive) const noexcept
{
    const float driven = x * drive;
    const float tanhSig = std::tanh(driven);
    const float atanSig = (2.0f / juce::MathConstants<float>::pi) * std::atan(driven * 1.3f);
    const float asymFactor = 1.0f + 0.05f * tanhSig;
    return (0.6f * tanhSig + 0.4f * atanSig) * asymFactor / drive;
}

float TapeDelay::pinkNoise(int ch) noexcept
{
    const float white = noiseDist(rng);
    pinkState[ch] = 0.99765f * pinkState[ch] + white * 0.099046f;
    return pinkState[ch] + white * 0.25f;
}

void TapeDelay::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int bufSize = (int) delayBuffer[0].size();

    const float baseDelaySamples = juce::jlimit(1.0f, 2000.0f, delayTimeMs) * (float) sampleRate / 1000.0f;
    const float wowDepth = baseDelaySamples * 0.015f * wow;
    const float flutterDepth = baseDelaySamples * 0.004f * flutter;
    const float drive = 1.0f + saturation * 9.0f;
    const float wet = mix;
    const float dry = 1.0f - mix;

    const float cutoff = 18000.0f * std::pow(1.0f - hfLoss, 2.5f) + 200.0f;
    const float hfCoeff = std::exp(-2.0f * juce::MathConstants<float>::pi * cutoff / (float) sampleRate);

    const double wowRate1 = 0.5 + wow * 2.0;
    const double wowRate2 = 1.1 + wow * 1.8;
    const double flutterRate1 = 8.0 + flutter * 4.0;
    const double flutterRate2 = 11.0 + flutter * 3.0;

    const double wowInc1 = (juce::MathConstants<double>::twoPi * wowRate1) / sampleRate;
    const double wowInc2 = (juce::MathConstants<double>::twoPi * wowRate2) / sampleRate;
    const double flutterInc1 = (juce::MathConstants<double>::twoPi * flutterRate1) / sampleRate;
    const double flutterInc2 = (juce::MathConstants<double>::twoPi * flutterRate2) / sampleRate;

    for (int i = 0; i < numSamples; ++i)
    {
        const float wowLfo = 0.7f * std::sin((float) wowPhase) + 0.3f * std::sin((float) wowPhase2);
        const float flutterLfo = 0.6f * std::sin((float) flutterPhase) + 0.4f * std::sin((float) flutterPhase2);
        const float modDelay = baseDelaySamples + wowDepth * wowLfo + flutterDepth * flutterLfo;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const int readCh = (pingPong && numChannels > 1) ? 1 - ch : ch;
            float delayed = readHermite(readCh, modDelay);

            hfState[ch] = delayed + hfCoeff * (hfState[ch] - delayed);
            delayed = hfState[ch];

            float fb = saturate(delayed, drive);
            fb += noiseLevel * 0.003f * pinkNoise(ch);

            const float input = buffer.getSample(ch, i);
            const float writeSample = input + fb * feedback;

            delayBuffer[ch][writePos] = writeSample;
            const float out = dry * input + wet * delayed;
            buffer.setSample(ch, i, out);
        }

        ++writePos;
        if (writePos >= bufSize)
            writePos = 0;

        wowPhase += wowInc1;
        wowPhase2 += wowInc2;
        flutterPhase += flutterInc1;
        flutterPhase2 += flutterInc2;

        if (wowPhase >= juce::MathConstants<double>::twoPi)
            wowPhase -= juce::MathConstants<double>::twoPi;
        if (wowPhase2 >= juce::MathConstants<double>::twoPi)
            wowPhase2 -= juce::MathConstants<double>::twoPi;
        if (flutterPhase >= juce::MathConstants<double>::twoPi)
            flutterPhase -= juce::MathConstants<double>::twoPi;
        if (flutterPhase2 >= juce::MathConstants<double>::twoPi)
            flutterPhase2 -= juce::MathConstants<double>::twoPi;
    }
}
