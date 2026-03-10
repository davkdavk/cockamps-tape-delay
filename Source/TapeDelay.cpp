#include "TapeDelay.h"
#include <cmath>


void TapeDelay::prepare(double newSampleRate, int, int newNumChannels)
{
    sampleRate  = newSampleRate;
    numChannels = juce::jmin(2, newNumChannels);
    reset();
}


void TapeDelay::reset()
{
    hfState[0]   = hfState[1]   = 0.0f;
    pinkState[0] = pinkState[1] = 0.0f;
    wowPhase = wowPhase2 = flutterPhase = flutterPhase2 = 0.0;
}


float TapeDelay::saturate(float x, float drive) const noexcept
{
    const float driven     = x * drive;
    const float tanhSig    = std::tanh(driven);
    const float atanSig    = (2.0f / juce::MathConstants<float>::pi) * std::atan(driven * 1.3f);
    const float asymFactor = 1.0f + 0.05f * tanhSig;
    return (0.6f * tanhSig + 0.4f * atanSig) * asymFactor / drive;
}


void TapeDelay::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();


    const double wowInc1     = juce::MathConstants<double>::twoPi * 0.5  / sampleRate;
    const double wowInc2     = juce::MathConstants<double>::twoPi * 1.1  / sampleRate;
    const double flutterInc1 = juce::MathConstants<double>::twoPi * 8.0  / sampleRate;
    const double flutterInc2 = juce::MathConstants<double>::twoPi * 11.0 / sampleRate;


    const float hfCutoff = 18000.0f * std::pow(1.0f - hfLoss, 2.5f) + 200.0f;
    const float hfCoeff  = std::exp(-juce::MathConstants<float>::twoPi * hfCutoff / (float)sampleRate);
    const float drive    = 1.0f + saturation * 9.0f;


    for (int i = 0; i < numSamples; ++i)
    {
        wowPhase      += wowInc1;
        wowPhase2     += wowInc2;
        flutterPhase  += flutterInc1;
        flutterPhase2 += flutterInc2;


        if (wowPhase      > juce::MathConstants<double>::twoPi) wowPhase      -= juce::MathConstants<double>::twoPi;
        if (wowPhase2     > juce::MathConstants<double>::twoPi) wowPhase2     -= juce::MathConstants<double>::twoPi;
        if (flutterPhase  > juce::MathConstants<double>::twoPi) flutterPhase  -= juce::MathConstants<double>::twoPi;
        if (flutterPhase2 > juce::MathConstants<double>::twoPi) flutterPhase2 -= juce::MathConstants<double>::twoPi;


        const float wowAmt     = (float)(std::sin(wowPhase)     * 0.7 + std::sin(wowPhase2)     * 0.3) * wow     * 0.004f;
        const float flutterAmt = (float)(std::sin(flutterPhase) * 0.6 + std::sin(flutterPhase2) * 0.4) * flutter * 0.002f;
        const float pitchScale = 1.0f + wowAmt + flutterAmt;


        for (int ch = 0; ch < numChannels; ++ch)
        {
            const int srcCh = (pingPong && numChannels > 1) ? 1 - ch : ch;
            const float dry = buffer.getSample(srcCh, i);
            float x = dry;


            x *= pitchScale;


            if (saturation > 0.001f)
                x = saturate(x, drive);


            if (hfLoss > 0.001f)
            {
                hfState[ch] = x + hfCoeff * (hfState[ch] - x);
                x = hfState[ch];
            }


            if (noiseLevel > 0.001f)
            {
                pinkState[ch] = 0.99765f * pinkState[ch] + noiseDist(rng) * 0.099046f;
                x += (pinkState[ch] + noiseDist(rng) * 0.25f) * noiseLevel * 0.003f;
            }


            buffer.setSample(ch, i, dry * (1.0f - mix) + x * mix);
        }
    }
}
