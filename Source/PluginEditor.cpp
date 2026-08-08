#include "PluginEditor.h"

DustBoxLookAndFeel::DustBoxLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffd8c7a2));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
}

void DustBoxLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                           float pos, float start, float end, juce::Slider& s)
{
    const float diameter = (float) juce::jmin (w, h) - 20.0f;
    auto r = juce::Rectangle<float> (diameter, diameter)
                 .withCentre ({ (float) x + (float) w * 0.5f, (float) y + (float) h * 0.5f });
    auto radius = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
    auto centre = r.getCentre();
    auto angle = start + pos * (end - start);
    const bool heat = s.getName() == "HEAT";

    g.setColour (juce::Colour (0xff151412));
    g.fillEllipse (r);
    g.setColour (juce::Colour (0xff514b40));
    g.drawEllipse (r, 1.5f);

    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, radius + 5.0f, radius + 5.0f, 0.0f, start, angle, true);
    g.setColour (heat ? juce::Colour (0xffe26d3f) : juce::Colour (0xffc6a66b));
    g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRoundedRectangle (-2.0f, -radius + 8.0f, 4.0f, radius * 0.42f, 2.0f);
    g.setColour (juce::Colour (0xffeee1c2));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

DustBoxLoFiAudioProcessorEditor::DustBoxLoFiAudioProcessorEditor (DustBoxLoFiAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&lookAndFeel);
    setSize (720, 460);
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.setName (names[i]);
        k.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 22);
        k.setDoubleClickReturnValue (true, i == 5 ? 0.0 : (i == 4 ? 1.0 : 0.0));
        addAndMakeVisible (k);
        labels[i].setText (names[i], juce::dontSendNotification);
        labels[i].setJustificationType (juce::Justification::centred);
        labels[i].setColour (juce::Label::textColourId, juce::Colour (0xffb9aa8c));
        labels[i].setFont (juce::FontOptions (13.0f).withStyle ("Bold"));
        addAndMakeVisible (labels[i]);
        attachments[i] = std::make_unique<Attachment> (audioProcessor.parameters, ids[i], k);
    }
}

DustBoxLoFiAudioProcessorEditor::~DustBoxLoFiAudioProcessorEditor() { setLookAndFeel (nullptr); }

void DustBoxLoFiAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0d0c0b));
    juce::ColourGradient glow (juce::Colour (0x223d2b1e), 360.0f, 230.0f,
                               juce::Colour (0x00100f0d), 360.0f, 460.0f, true);
    g.setGradientFill (glow); g.fillAll();
    g.setColour (juce::Colour (0xffd8c7a2));
    g.setFont (juce::FontOptions (30.0f).withStyle ("Bold"));
    g.drawText ("DUSTBOX", 0, 22, getWidth(), 42, juce::Justification::centred);
    g.setColour (juce::Colour (0xff716858));
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("LO-FI COLOR PROCESSOR  /  ALPHA 0.2.1", 0, 61, getWidth(), 20, juce::Justification::centred);
    g.drawHorizontalLine (91, 42.0f, (float) getWidth() - 42.0f);
    g.setColour (juce::Colour (0xff4b453b));
    g.drawRoundedRectangle (18.0f, 14.0f, (float) getWidth() - 36.0f, (float) getHeight() - 28.0f, 10.0f, 1.0f);
}

void DustBoxLoFiAudioProcessorEditor::resized()
{
    const int top = 112, labelH = 24;
    const int centres[] = { 105, 275, 445, 615 };
    for (int i = 0; i < 4; ++i)
    {
        labels[(size_t)i].setBounds (centres[i] - 55, top, 110, labelH);
        const int size = i == 3 ? 126 : 110;
        knobs[(size_t)i].setBounds (centres[i] - size / 2, top + 23, size, size + 30);
    }
    labels[4].setBounds (225, 296, 110, labelH); knobs[4].setBounds (225, 319, 110, 132);
    labels[5].setBounds (385, 296, 110, labelH); knobs[5].setBounds (385, 319, 110, 132);
}
