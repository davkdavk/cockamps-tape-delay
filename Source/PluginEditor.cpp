#include "PluginEditor.h"

static constexpr float kUiScale = 0.6f;

static juce::Colour colourFromHex(juce::uint32 value)
{
    return juce::Colour((juce::uint32) value);
}

void VintageLAF::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                  float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                  juce::Slider&)
{
    const auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height);
    const float inset = 6.0f * uiScale;
    const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f - inset;
    const auto centre = bounds.getCentre();

    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const auto body = juce::Colour(0xCC1A1208);
    const auto accent = colourFromHex(0xFFE8B85A);

    g.setColour(body);
    g.fillEllipse(centre.getX() - radius, centre.getY() - radius,
                  radius * 2.0f, radius * 2.0f);

    g.setColour(colourFromHex(0xFF5A4820));
    g.drawEllipse(centre.getX() - radius, centre.getY() - radius, radius * 2.0f, radius * 2.0f, 1.5f * uiScale);

    const float arcRadius = radius + inset;
    juce::Path arc;
    arc.addArc(centre.getX() - arcRadius, centre.getY() - arcRadius,
               arcRadius * 2.0f, arcRadius * 2.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(colourFromHex(0xFF2A1E10));
    g.strokePath(arc, juce::PathStrokeType(2.5f * uiScale));

    juce::Path arcValue;
    arcValue.addArc(centre.getX() - arcRadius, centre.getY() - arcRadius,
                    arcRadius * 2.0f, arcRadius * 2.0f, rotaryStartAngle, angle, true);
    g.setColour(accent);
    g.strokePath(arcValue, juce::PathStrokeType(3.0f * uiScale));

    const float pointerAngle = angle - (juce::MathConstants<float>::pi * 0.5f);
    juce::Line<float> pointer(centre.getX(), centre.getY(),
                               centre.getX() + radius * std::cos(pointerAngle),
                               centre.getY() + radius * std::sin(pointerAngle));
    g.setColour(colourFromHex(0xFFFFD87F));
    g.drawLine(pointer, 2.0f * uiScale);
}

void VintageLAF::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                  bool isHovered, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    const auto base = colourFromHex(0xFF1E160A);
    const auto border = colourFromHex(0xFF5A4820);
    const auto accent = colourFromHex(0xFFE8B85A);

    g.setColour(base);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(border);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    if (button.getToggleState())
    {
        g.setColour(accent.withAlpha(isHovered ? 0.9f : 0.75f));
        g.fillRoundedRectangle(bounds.reduced(4.0f), 3.0f);
    }

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font("Courier New", 14.0f * uiScale, juce::Font::bold));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds(),
                     juce::Justification::centred, 1);
}

void VintageLAF::drawComboBox(juce::Graphics&, int, int, bool,
                              int, int, int, int, juce::ComboBox&)
{
}

juce::Font VintageLAF::getLabelFont(juce::Label&)
{
    return juce::Font("Courier New", 13.0f * uiScale * 0.4f, juce::Font::bold);
}

juce::Font VintageLAF::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font("Courier New", 13.0f * uiScale, juce::Font::bold);
}

KnobRow::KnobRow(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                 const juce::String& labelText, VintageLAF& laf)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                               juce::MathConstants<float>::pi * 2.75f,
                               true);
    slider.setLookAndFeel(&laf);

    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    label.setFont(juce::Font("Courier New", 9.0f, juce::Font::bold));

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);

    addAndMakeVisible(slider);
    addAndMakeVisible(label);
}

void KnobRow::resized()
{
    auto bounds = getLocalBounds();
    auto labelArea = bounds.removeFromBottom(20).translated(0, -5);
    label.setBounds(labelArea);
    slider.setBounds(bounds);
}

TapeDelayEditor::TapeDelayEditor(TapeDelayProcessor& proc)
    : AudioProcessorEditor(&proc)
    , processor(proc)
{
    laf.setUiScale(kUiScale);
    setSize((int) std::round(1456.0f * kUiScale), (int) std::round(816.0f * kUiScale));

    pingPong.setButtonText("PING PONG");
    pingPong.setLookAndFeel(&laf);
    pingPong.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    pingPongAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, "pingPong", pingPong);

    tapeToggle.setButtonText("TAPE ON");
    tapeToggle.setLookAndFeel(&laf);
    tapeToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    tapeToggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, "tapeEnabled", tapeToggle);

    const int labelHeight = (int) std::round(20.0f * kUiScale);
    knobDelay.setLabelHeight(labelHeight);
    knobFeedback.setLabelHeight(labelHeight);
    knobMix.setLabelHeight(labelHeight);
    knobClockNoise.setLabelHeight(labelHeight);
    knobCompander.setLabelHeight(labelHeight);
    knobModDepth.setLabelHeight(labelHeight);
    knobModRate.setLabelHeight(labelHeight);
    knobWow.setLabelHeight(labelHeight);
    knobFlutter.setLabelHeight(labelHeight);
    knobSaturation.setLabelHeight(labelHeight);
    knobHfLoss.setLabelHeight(labelHeight);
    knobNoise.setLabelHeight(labelHeight);

    addAndMakeVisible(knobDelay);
    addAndMakeVisible(knobFeedback);
    addAndMakeVisible(knobMix);
    addAndMakeVisible(knobWow);
    addAndMakeVisible(knobFlutter);
    addAndMakeVisible(knobSaturation);
    addAndMakeVisible(knobHfLoss);
    addAndMakeVisible(knobNoise);
    addAndMakeVisible(pingPong);
    addAndMakeVisible(tapeToggle);
    addAndMakeVisible(knobClockNoise);
    addAndMakeVisible(knobCompander);
    addAndMakeVisible(knobModDepth);
    addAndMakeVisible(knobModRate);
}

TapeDelayEditor::~TapeDelayEditor()
{
    pingPong.setLookAndFeel(nullptr);
    tapeToggle.setLookAndFeel(nullptr);
}

void TapeDelayEditor::paint(juce::Graphics& g)
{
    auto image = juce::ImageCache::getFromMemory(BinaryData::CockDelayBG_png, BinaryData::CockDelayBG_pngSize);
    g.drawImage(image, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
}

void TapeDelayEditor::resized()
{
    auto scale = [](float value) { return (int) std::round(value * kUiScale); };
    const int knobW = scale(100.0f);
    const int knobH = scale(110.0f);

    knobDelay.setBounds(scale(320.0f), scale(270.0f), knobW, knobH);
    knobFeedback.setBounds(scale(460.0f), scale(270.0f), knobW, knobH);
    knobMix.setBounds(scale(600.0f), scale(270.0f), knobW, knobH);
    knobClockNoise.setBounds(scale(740.0f), scale(270.0f), knobW, knobH);
    knobCompander.setBounds(scale(880.0f), scale(270.0f), knobW, knobH);
    knobModDepth.setBounds(scale(1020.0f), scale(270.0f), knobW, knobH);

    knobModRate.setBounds(scale(320.0f), scale(420.0f), knobW, knobH);
    knobWow.setBounds(scale(460.0f), scale(420.0f), knobW, knobH);
    knobFlutter.setBounds(scale(600.0f), scale(420.0f), knobW, knobH);
    knobSaturation.setBounds(scale(740.0f), scale(420.0f), knobW, knobH);
    knobHfLoss.setBounds(scale(880.0f), scale(420.0f), knobW, knobH);
    knobNoise.setBounds(scale(1020.0f), scale(420.0f), knobW, knobH);

    pingPong.setBounds(scale(320.0f), scale(540.0f), scale(150.0f), scale(36.0f));
    tapeToggle.setBounds(scale(490.0f), scale(540.0f), scale(150.0f), scale(36.0f));
}
