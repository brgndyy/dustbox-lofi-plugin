#pragma once
#include <JuceHeader.h>

class DustBoxLoFiAudioProcessor final : public juce::AudioProcessor
{
public:
    DustBoxLoFiAudioProcessor();
    ~DustBoxLoFiAudioProcessor() override = default;
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void loadVinylCrackle();
    float getVinylSample();
    float readWarpDelay (int channel, float delaySamples) const;
    float cubic (float y0, float y1, float y2, float y3, float mu) const;
    float saturate (float x, float heat) const;

    double sampleRateHz = 44100.0;
    juce::AudioBuffer<float> vinylBuffer, warpDelay, dryBuffer;
    int warpWrite = 0;
    double vinylReadPos = 0.0, vinylStep = 1.0;
    double wowPhase = 0.0, flutterPhase = 0.0;
    float randomWarp = 0.0f, randomWarpTarget = 0.0f;
    int randomWarpCountdown = 1, popCountdown = 1;
    float popEnvelope = 0.0f, noiseState = 0.0f;
    float lpState[2] {}, hpState[2] {}, hpPrev[2] {}, headBumpState[2] {};
    float ageHeldSample[2] {};
    int ageHoldCounter[2] {};
    juce::Random random;

    std::unique_ptr<juce::dsp::Oversampling<float>> heatOversampling;
    juce::SmoothedValue<float> ageSmooth, warpSmooth, dustSmooth, heatSmooth, mixSmooth, outputSmooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DustBoxLoFiAudioProcessor)
};
