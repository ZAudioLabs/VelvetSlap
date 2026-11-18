#pragma once
#include <JuceHeader.h>

class VelvetSlapAudioProcessor : public juce::AudioProcessor
{
public:
    VelvetSlapAudioProcessor();
    void prepareToPlay (double sr, int bs) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
          && l.getMainInputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Velvet Slap"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay { 192000 };
    juce::dsp::Reverb reverb;
    juce::dsp::IIR::Filter<float> lp[2];
    juce::SmoothedValue<float> sDelay;
    juce::AudioBuffer<float> wet, rev;
    std::vector<float> duck;
    float fbs[2] { 0, 0 }, env = 0, atk = 0.01f, rel = 0.001f;
    double sr = 44100;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VelvetSlapAudioProcessor)
};
