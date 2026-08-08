#include "PluginProcessor.h"
#include <cmath>
#include <iostream>

static double renderRms (float age)
{
    DustBoxLoFiAudioProcessor p;
    p.prepareToPlay (48000.0, 256);
    for (auto id : { "warp", "dust", "heat" })
        p.parameters.getParameter (id)->setValueNotifyingHost (0.0f);
    p.parameters.getParameter ("mix")->setValueNotifyingHost (1.0f);
    p.parameters.getParameter ("age")->setValueNotifyingHost (age);

    juce::MidiBuffer midi;
    juce::AudioBuffer<float> block (2, 256);
    double energy = 0.0;
    int count = 0;
    double phase = 0.0;
    for (int b = 0; b < 120; ++b)
    {
        for (int i = 0; i < 256; ++i)
        {
            const float x = 0.35f * std::sin ((float) phase);
            phase += juce::MathConstants<double>::twoPi * 9000.0 / 48000.0;
            for (int ch = 0; ch < 2; ++ch) block.setSample (ch, i, x);
        }
        p.processBlock (block, midi);
        if (b > 30)
            for (int i = 0; i < 256; ++i) { const double x = block.getSample (0, i); energy += x * x; ++count; }
    }
    return std::sqrt (energy / count);
}

int main()
{
    const double fresh = renderRms (0.0f);
    const double aged = renderRms (1.0f);
    std::cout << "AGE 0 RMS=" << fresh << " AGE 100 RMS=" << aged
              << " ratio=" << aged / fresh << '\n';
    if (! std::isfinite (fresh) || ! std::isfinite (aged) || aged >= fresh * 0.40)
        return 1;
    return 0;
}
