#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

DustBoxLoFiAudioProcessor::DustBoxLoFiAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    loadVinylCrackle();
}

juce::AudioProcessorValueTreeState::ParameterLayout DustBoxLoFiAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto pct = [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; };
    auto attr = [pct] { return juce::AudioParameterFloatAttributes().withStringFromValueFunction (pct); };
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("age", "AGE", juce::NormalisableRange<float> (0, 1), 0.38f, attr()));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("warp", "WARP", juce::NormalisableRange<float> (0, 1), 0.22f, attr()));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("dust", "DUST", juce::NormalisableRange<float> (0, 1), 0.18f, attr()));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("heat", "HEAT", juce::NormalisableRange<float> (0, 1), 0.34f, attr()));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mix", "MIX", juce::NormalisableRange<float> (0, 1), 1.0f, attr()));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("output", "OUTPUT", juce::NormalisableRange<float> (-18, 12, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; })));
    return { p.begin(), p.end() };
}

void DustBoxLoFiAudioProcessor::loadVinylCrackle()
{
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatReader> reader (wav.createReaderFor (
        new juce::MemoryInputStream (BinaryData::vinyl_crackle_source_original_wav,
                                     BinaryData::vinyl_crackle_source_original_wavSize, false), true));
    if (reader)
    {
        vinylBuffer.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
        reader->read (&vinylBuffer, 0, (int) reader->lengthInSamples, 0, true, true);
        vinylStep = reader->sampleRate / sampleRateHz;
    }
}

void DustBoxLoFiAudioProcessor::prepareToPlay (double sr, int blockSize)
{
    sampleRateHz = sr;
    warpDelay.setSize (2, juce::jmax (256, (int) (sr * 0.06))); warpDelay.clear();
    dryBuffer.setSize (2, blockSize);
    warpWrite = 0; vinylReadPos = wowPhase = flutterPhase = 0.0;
    vinylStep = 44100.0 / sr;
    std::fill (std::begin (lpState), std::end (lpState), 0.0f);
    std::fill (std::begin (hpState), std::end (hpState), 0.0f);
    std::fill (std::begin (hpPrev), std::end (hpPrev), 0.0f);
    std::fill (std::begin (headBumpState), std::end (headBumpState), 0.0f);
    std::fill (std::begin (ageHeldSample), std::end (ageHeldSample), 0.0f);
    std::fill (std::begin (ageHoldCounter), std::end (ageHoldCounter), 0);
    heatOversampling = std::make_unique<juce::dsp::Oversampling<float>> (2, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    heatOversampling->initProcessing ((size_t) blockSize);
    heatOversampling->reset();
    setLatencySamples ((int) std::round (heatOversampling->getLatencyInSamples()));
    for (auto* s : { &ageSmooth, &warpSmooth, &dustSmooth, &heatSmooth, &mixSmooth, &outputSmooth })
        s->reset (sr, 0.035);
    randomWarpCountdown = popCountdown = 1;
}

bool DustBoxLoFiAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && in == out;
}

float DustBoxLoFiAudioProcessor::cubic (float y0, float y1, float y2, float y3, float mu) const
{
    const float a0 = y3 - y2 - y0 + y1;
    const float a1 = y0 - y1 - a0;
    const float a2 = y2 - y0;
    return ((a0 * mu + a1) * mu + a2) * mu + y1;
}

float DustBoxLoFiAudioProcessor::readWarpDelay (int ch, float delaySamples) const
{
    const int len = warpDelay.getNumSamples();
    float pos = (float) warpWrite - delaySamples;
    while (pos < 0) pos += (float) len;
    const int i1 = (int) pos;
    const float f = pos - (float) i1;
    auto at = [&, len] (int i) { i %= len; if (i < 0) i += len; return warpDelay.getSample (ch, i); };
    return cubic (at (i1 - 1), at (i1), at (i1 + 1), at (i1 + 2), f);
}

float DustBoxLoFiAudioProcessor::getVinylSample()
{
    if (vinylBuffer.getNumSamples() == 0) return 0.0f;
    const int len = vinylBuffer.getNumSamples(), channels = vinylBuffer.getNumChannels();
    const int i0 = (int) vinylReadPos % len, i1 = (i0 + 1) % len;
    const float f = (float) (vinylReadPos - std::floor (vinylReadPos));
    float out = 0.0f;
    for (int ch = 0; ch < channels; ++ch) out += juce::jmap (f, vinylBuffer.getSample (ch, i0), vinylBuffer.getSample (ch, i1));
    vinylReadPos += vinylStep; while (vinylReadPos >= len) vinylReadPos -= len;
    return out / (float) juce::jmax (1, channels);
}

float DustBoxLoFiAudioProcessor::saturate (float x, float heat) const
{
    const float drive = 1.0f + heat * heat * 14.0f;
    const float bias = 0.045f * heat;
    const float tape = std::tanh ((x + bias) * drive) / std::tanh (drive) - std::tanh (bias * drive) / std::tanh (drive);
    const float softClip = x / (1.0f + std::abs (x));
    return juce::jmap (heat * 0.38f, tape, softClip * 1.8f);
}

void DustBoxLoFiAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals guard;
    const int channels = juce::jmin (2, buffer.getNumChannels()), n = buffer.getNumSamples();
    dryBuffer.setSize (channels, n, false, false, true);
    for (int ch = 0; ch < channels; ++ch) dryBuffer.copyFrom (ch, 0, buffer, ch, 0, n);

    ageSmooth.setTargetValue (parameters.getRawParameterValue ("age")->load());
    warpSmooth.setTargetValue (parameters.getRawParameterValue ("warp")->load());
    dustSmooth.setTargetValue (parameters.getRawParameterValue ("dust")->load());
    heatSmooth.setTargetValue (parameters.getRawParameterValue ("heat")->load());
    mixSmooth.setTargetValue (parameters.getRawParameterValue ("mix")->load());
    outputSmooth.setTargetValue (juce::Decibels::decibelsToGain (parameters.getRawParameterValue ("output")->load()));

    for (int i = 0; i < n; ++i)
    {
        const float age = ageSmooth.getNextValue(), warp = warpSmooth.getNextValue(), dust = dustSmooth.getNextValue();
        heatSmooth.getNextValue();
        if (--randomWarpCountdown <= 0) { randomWarpTarget = random.nextFloat() * 2.0f - 1.0f; randomWarpCountdown = (int) (sampleRateHz * (0.08 + random.nextFloat() * 0.22)); }
        randomWarp += 0.0007f * (randomWarpTarget - randomWarp);
        wowPhase += (0.18 + 0.38 * warp) / sampleRateHz; if (wowPhase >= 1.0) wowPhase -= 1.0;
        flutterPhase += (4.4 + 2.8 * warp) / sampleRateHz; if (flutterPhase >= 1.0) flutterPhase -= 1.0;
        const float mod = (float) std::sin (wowPhase * juce::MathConstants<double>::twoPi)
                        + 0.24f * (float) std::sin (flutterPhase * juce::MathConstants<double>::twoPi) + randomWarp * 0.32f;
        const float baseDelay = (float) sampleRateHz * 0.018f;
        const float depth = (float) sampleRateHz * (0.00004f + warp * warp * 0.0017f);
        const float vinyl = getVinylSample();
        if (--popCountdown <= 0) { popEnvelope = 1.0f; popCountdown = (int) (sampleRateHz * (0.08f + random.nextFloat() * (2.2f - 1.8f * dust))); }
        const float click = (random.nextFloat() * 2.0f - 1.0f) * popEnvelope; popEnvelope *= 0.72f;
        noiseState = 0.985f * noiseState + 0.015f * (random.nextFloat() * 2.0f - 1.0f);
        const float texture = dust * dust * (vinyl * 0.115f + click * 0.09f + noiseState * 0.028f);
        const float aged = std::pow (age, 2.25f);
        const float lpCoeff = std::exp (-2.0f * juce::MathConstants<float>::pi * juce::jmap (aged, 18000.0f, 1100.0f) / (float) sampleRateHz);
        const float hpCoeff = std::exp (-2.0f * juce::MathConstants<float>::pi * juce::jmap (aged, 16.0f, 135.0f) / (float) sampleRateHz);
        const int holdLength = 1 + (int) std::round (aged * 3.0f);
        const float quantSteps = std::pow (2.0f, 15.0f - aged * 6.5f);

        for (int ch = 0; ch < channels; ++ch)
        {
            const float input = buffer.getSample (ch, i);
            warpDelay.setSample (ch, warpWrite, input);
            float x = readWarpDelay (ch, baseDelay + depth * mod * (ch == 0 ? 1.0f : 0.91f));
            if (ageHoldCounter[ch] <= 0)
            {
                ageHeldSample[ch] = std::round (x * quantSteps) / quantSteps;
                ageHoldCounter[ch] = holdLength;
            }
            --ageHoldCounter[ch];
            x = juce::jmap (aged, x, ageHeldSample[ch]);
            const float hp = hpCoeff * (hpState[ch] + x - hpPrev[ch]); hpPrev[ch] = x; hpState[ch] = hp;
            headBumpState[ch] += 0.018f * (hp - headBumpState[ch]);
            x = hp + age * 0.12f * headBumpState[ch] + texture;
            lpState[ch] = (1.0f - lpCoeff) * x + lpCoeff * lpState[ch];
            buffer.setSample (ch, i, lpState[ch]);
        }
        warpWrite = (warpWrite + 1) % warpDelay.getNumSamples();

    }

    if (heatOversampling)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        auto up = heatOversampling->processSamplesUp (block);
        const float heat = heatSmooth.getCurrentValue();
        const float autoGain = juce::Decibels::decibelsToGain (-heat * 7.0f);
        for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
            for (size_t i = 0; i < up.getNumSamples(); ++i)
                up.setSample ((int) ch, (int) i, saturate (up.getSample ((int) ch, (int) i), heat) * autoGain);
        heatOversampling->processSamplesDown (block);
    }

    for (int i = 0; i < n; ++i)
    {
        const float mix = mixSmooth.getNextValue(), gain = outputSmooth.getNextValue();
        for (int ch = 0; ch < channels; ++ch)
        {
            const float wet = buffer.getSample (ch, i), dry = dryBuffer.getSample (ch, i);
            buffer.setSample (ch, i, juce::jlimit (-1.0f, 1.0f, (dry + (wet - dry) * mix) * gain));
        }
    }
}

juce::AudioProcessorEditor* DustBoxLoFiAudioProcessor::createEditor() { return new DustBoxLoFiAudioProcessorEditor (*this); }
void DustBoxLoFiAudioProcessor::getStateInformation (juce::MemoryBlock& d) { if (auto xml = parameters.copyState().createXml()) copyXmlToBinary (*xml, d); }
void DustBoxLoFiAudioProcessor::setStateInformation (const void* d, int n) { if (auto xml = getXmlFromBinary (d, n)) if (xml->hasTagName (parameters.state.getType())) parameters.replaceState (juce::ValueTree::fromXml (*xml)); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DustBoxLoFiAudioProcessor(); }
