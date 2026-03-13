#include "PluginProcessor.h"
#include "PluginEditor.h"

TapeDelayProcessor::TapeDelayProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
}

void TapeDelayProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    tapeEngine.prepare(sampleRate, samplesPerBlock, getTotalNumInputChannels());
    bbdEngine.prepare(sampleRate, samplesPerBlock, getTotalNumInputChannels());
}

void TapeDelayProcessor::releaseResources()
{
}

bool TapeDelayProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainIn.isDisabled() || mainOut.isDisabled())
        return false;

    if (mainIn != mainOut)
        return false;

    return mainIn == juce::AudioChannelSet::mono() || mainIn == juce::AudioChannelSet::stereo();
}

void TapeDelayProcessor::updateEngineParams()
{
    bbdEngine.delayTimeMs = juce::jmin(2400.0f, apvts.getRawParameterValue("delayTime")->load());
    bbdEngine.feedback = apvts.getRawParameterValue("feedback")->load();
    bbdEngine.mix = apvts.getRawParameterValue("mix")->load();
    bbdEngine.clockNoise = apvts.getRawParameterValue("clockNoise")->load();
    bbdEngine.compander = apvts.getRawParameterValue("compander")->load();
    bbdEngine.modDepth = apvts.getRawParameterValue("modDepth")->load();
    bbdEngine.modRate = apvts.getRawParameterValue("modRate")->load();

    tapeEngine.wow = apvts.getRawParameterValue("wow")->load();
    tapeEngine.flutter = apvts.getRawParameterValue("flutter")->load();
    tapeEngine.saturation = apvts.getRawParameterValue("saturation")->load();
    tapeEngine.mix = 1.0f;
    tapeEngine.hfLoss = apvts.getRawParameterValue("hfLoss")->load();
    tapeEngine.noiseLevel = apvts.getRawParameterValue("noiseLevel")->load();
    tapeEngine.pingPong = apvts.getRawParameterValue("pingPong")->load() > 0.5f;
    tapeEnabled = apvts.getRawParameterValue("tapeEnabled")->load() > 0.5f;
}

void TapeDelayProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    updateEngineParams();

    const bool powerEnabled = apvts.getRawParameterValue("powerEnabled")->load() > 0.5f;
    if (! powerEnabled)
    {
        buffer.clear();
        return;
    }

    bbdEngine.process(buffer);
    const bool tapeNowEnabled = apvts.getRawParameterValue("tapeEnabled")->load() > 0.5f;
    if (tapeNowEnabled && !tapePreviouslyEnabled)
        tapeEngine.reset();
    tapePreviouslyEnabled = tapeNowEnabled;
    if (tapeNowEnabled)
        tapeEngine.process(buffer);
}

juce::AudioProcessorEditor* TapeDelayProcessor::createEditor()
{
    return new TapeDelayEditor(*this);
}

void TapeDelayProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState().createXml())
        copyXmlToBinary(*state, destData);
}

void TapeDelayProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout TapeDelayProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "delayTime", "DELAY",
        juce::NormalisableRange<float>(10.0f, 2400.0f, 1.0f), 250.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "feedback", "FEEDBACK",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.001f), 0.40f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "mix", "MIX",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.50f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "wow", "WOW",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.25f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "flutter", "FLUTTER",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.20f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "saturation", "SAT",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.40f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "hfLoss", "HF LOSS",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.40f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "noiseLevel", "NOISE",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.10f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "pingPong", "PING PONG", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "tapeEnabled", "TAPE ON/OFF", true));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "powerEnabled", "POWER", true));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "clockNoise", "CLOCK",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.20f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "compander", "COMPANDER",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.30f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "modDepth", "MOD DEPTH",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.15f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "modRate", "MOD RATE",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.30f));

    return layout;
}
