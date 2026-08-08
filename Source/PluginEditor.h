#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class DustBoxLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    DustBoxLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
};

class DustBoxLoFiAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit DustBoxLoFiAudioProcessorEditor (DustBoxLoFiAudioProcessor&);
    ~DustBoxLoFiAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    DustBoxLoFiAudioProcessor& audioProcessor;
    DustBoxLookAndFeel lookAndFeel;
    std::array<juce::Slider, 6> knobs;
    std::array<juce::Label, 6> labels;
    std::array<std::unique_ptr<Attachment>, 6> attachments;
    const std::array<juce::String, 6> ids { "age", "warp", "dust", "heat", "mix", "output" };
    const std::array<juce::String, 6> names { "AGE", "WARP", "DUST", "HEAT", "MIX", "OUTPUT" };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DustBoxLoFiAudioProcessorEditor)
};
