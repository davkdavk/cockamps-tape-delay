#pragma once

#include <JuceHeader.h>
#include <memory>

#include "PluginProcessor.h"

class VintageLAF final : public juce::LookAndFeel_V4
{
public:
    void setUiScale(float newScale) { uiScale = newScale; }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool isHovered, bool isDown) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    juce::Font getLabelFont(juce::Label&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;

private:
    float uiScale = 1.0f;
};

class InvisibleToggleLAF final : public juce::LookAndFeel_V4
{
public:
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override {}
};

class KnobRow final : public juce::Component
{
public:
    KnobRow(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
            const juce::String& labelText, VintageLAF& laf);

    void setLabelHeight(int newHeight) { labelHeight = newHeight; }

    void resized() override;

private:
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    int labelHeight = 20;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KnobRow)
};

class TapeDelayEditor final : public juce::AudioProcessorEditor
{
public:
    explicit TapeDelayEditor(TapeDelayProcessor& processor);
    ~TapeDelayEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    TapeDelayProcessor& processor;
    VintageLAF laf;
    InvisibleToggleLAF invisibleLaf;

    KnobRow knobDelay { processor.apvts, "delayTime", "DELAY", laf };
    KnobRow knobFeedback { processor.apvts, "feedback", "FEEDBACK", laf };
    KnobRow knobMix { processor.apvts, "mix", "MIX", laf };

    KnobRow knobWow { processor.apvts, "wow", "WOW", laf };
    KnobRow knobFlutter { processor.apvts, "flutter", "FLUTTER", laf };
    KnobRow knobSaturation { processor.apvts, "saturation", "SAT", laf };
    KnobRow knobHfLoss { processor.apvts, "hfLoss", "HF LOSS", laf };
    KnobRow knobNoise { processor.apvts, "noiseLevel", "NOISE", laf };

    juce::ToggleButton pingPong;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> pingPongAttachment;

    juce::ToggleButton tapeToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tapeToggleAttachment;

    juce::ToggleButton powerToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerToggleAttachment;

    juce::TextButton tapButton;

    KnobRow knobClockNoise { processor.apvts, "clockNoise", "CLOCK", laf };
    KnobRow knobCompander { processor.apvts, "compander", "COMPANDER", laf };
    KnobRow knobModDepth { processor.apvts, "modDepth", "MOD DEPTH", laf };
    KnobRow knobModRate { processor.apvts, "modRate", "MOD RATE", laf };

    double lastTapTimeMs = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapeDelayEditor)
};
