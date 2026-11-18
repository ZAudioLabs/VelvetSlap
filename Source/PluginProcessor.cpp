#include "PluginProcessor.h"
#include "PluginEditor.h"

using APF = juce::AudioParameterFloat;

juce::AudioProcessorValueTreeState::ParameterLayout VelvetSlapAudioProcessor::layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<APF> ("mix",      "Mix",      juce::NormalisableRange<float> (0.f, 1.f),  0.30f));
    l.add (std::make_unique<APF> ("ambience", "Ambience", juce::NormalisableRange<float> (0.f, 1.f),  0.35f));
    l.add (std::make_unique<APF> ("width",    "Width",    juce::NormalisableRange<float> (0.f, 1.5f), 1.0f));
    l.add (std::make_unique<APF> ("delay",    "Delay",    juce::NormalisableRange<float> (0.f, 1.f),  0.30f));
    l.add (std::make_unique<APF> ("sccomp",   "S.C. Comp",juce::NormalisableRange<float> (0.f, 1.f),  0.30f));
    l.add (std::make_unique<juce::AudioParameterBool> ("wetonly", "Wet Only", false));
    l.add (std::make_unique<juce::AudioParameterBool> ("bypass",  "Bypass",   false));
    return l;
}

VelvetSlapAudioProcessor::VelvetSlapAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", layout()) {}

void VelvetSlapAudioProcessor::prepareToPlay (double s, int bs)
{
    sr = s;
    juce::dsp::ProcessSpec spec { s, (juce::uint32) bs, 2 };
    delay.prepare (spec);
    delay.setMaximumDelayInSamples ((int) (s * 0.5));
    reverb.prepare (spec);
    juce::dsp::Reverb::Parameters p;
    p.roomSize = 0.4f; p.damping = 0.6f; p.wetLevel = 1.f; p.dryLevel = 0.f; p.width = 1.f;
    reverb.setParameters (p);
    for (auto& f : lp) { f.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (s, 5500.f); f.reset(); }
    sDelay.reset (s, 0.05);
    sDelay.setCurrentAndTargetValue ((float) (s * 0.07));
    atk = 1.f - std::exp (-1.f / (0.003f * (float) s));
    rel = 1.f - std::exp (-1.f / (0.150f * (float) s));
    fbs[0] = fbs[1] = env = 0.f;
}

void VelvetSlapAudioProcessor::processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    if (apvts.getRawParameterValue ("bypass")->load() > 0.5f) return;

    const int n = buf.getNumSamples();
    const float mix = apvts.getRawParameterValue ("mix")->load();
    const float amb = apvts.getRawParameterValue ("ambience")->load();
    const float wid = apvts.getRawParameterValue ("width")->load();
    const float del = apvts.getRawParameterValue ("delay")->load();
    const float sc  = apvts.getRawParameterValue ("sccomp")->load();
    const bool wetOnly = apvts.getRawParameterValue ("wetonly")->load() > 0.5f;

    wet.setSize (2, n, false, false, true);
    duck.resize ((size_t) n);
    sDelay.setTargetValue ((float) (sr * (0.020 + 0.160 * del)));   // 20 a 180 ms

    auto *L = buf.getWritePointer (0), *R = buf.getWritePointer (1);
    auto *wl = wet.getWritePointer (0), *wr = wet.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        const float dl = sDelay.getNextValue();
        const float a = std::abs (0.5f * (L[i] + R[i]));
        env += (a > env ? atk : rel) * (a - env);
        duck[(size_t) i] = 1.f / (1.f + sc * 10.f * env);            // sidechain: dry abaixa o wet

        delay.pushSample (0, L[i] + fbs[0] * 0.2f);
        delay.pushSample (1, R[i] + fbs[1] * 0.2f);
        fbs[0] = lp[0].processSample (delay.popSample (0, dl));
        fbs[1] = lp[1].processSample (delay.popSample (1, dl * 1.12f)); // R levemente mais longo
        wl[i] = fbs[0];
        wr[i] = fbs[1];
    }

    rev.makeCopyOf (wet, true);
    juce::dsp::AudioBlock<float> blk (rev);
    reverb.process (juce::dsp::ProcessContextReplacing<float> (blk));
    auto *rl = rev.getReadPointer (0), *rr = rev.getReadPointer (1);

    for (int i = 0; i < n; ++i)
    {
        const float g = duck[(size_t) i];
        float l = (wl[i] * (1.f - 0.3f * amb) + rl[i] * amb) * g;
        float r = (wr[i] * (1.f - 0.3f * amb) + rr[i] * amb) * g;
        const float m = 0.5f * (l + r), s = 0.5f * (l - r) * wid;    // width 0-150%
        l = m + s; r = m - s;
        L[i] = wetOnly ? l * mix : L[i] * (1.f - mix) + l * mix;
        R[i] = wetOnly ? r * mix : R[i] * (1.f - mix) + r * mix;
    }
}

juce::AudioProcessorEditor* VelvetSlapAudioProcessor::createEditor() { return new VelvetSlapEditor (*this); }

void VelvetSlapAudioProcessor::getStateInformation (juce::MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, d);
}
void VelvetSlapAudioProcessor::setStateInformation (const void* d, int sz)
{
    if (auto xml = getXmlFromBinary (d, sz)) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VelvetSlapAudioProcessor(); }
