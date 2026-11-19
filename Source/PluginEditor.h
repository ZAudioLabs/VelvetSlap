#pragma once
#include "PluginProcessor.h"

// Knob desenhado por cima do fundo (cobre os knobs da imagem e anima o arco)
struct VelvetLnF : juce::LookAndFeel_V4
{
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override;
};

// Botao com visual do layout (borda verde neon quando ativo)
struct PillButton : juce::ToggleButton
{
    juce::String label;
    void paintButton (juce::Graphics&, bool, bool) override;
};

class VelvetSlapEditor : public juce::AudioProcessorEditor
{
public:
    explicit VelvetSlapEditor (VelvetSlapAudioProcessor&);
    ~VelvetSlapEditor() override { for (auto& k : knobs) k.setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    juce::String readout (int i) const;
private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    VelvetLnF lnf;
    juce::Image bg;
    juce::Slider knobs[5];
    std::unique_ptr<SA> ka[5];
    PillButton wetBtn, bypBtn;
    std::unique_ptr<BA> wa, ba;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VelvetSlapEditor)
};
