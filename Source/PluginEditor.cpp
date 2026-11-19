#include "PluginEditor.h"
#include <BinaryData.h>

static const juce::Colour lime (0xffa6c92a);

void VelvetLnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                  float start, float end, juce::Slider&)
{
    const float R = juce::jmin (w, h) * 0.5f;
    const auto c = juce::Point<float> (x + w * 0.5f, y + h * 0.5f);
    const float ang = start + pos * (end - start);

    auto arc = [&] (float a0, float a1, float rad, float thick, juce::Colour col)
    {
        juce::Path p; p.addCentredArc (c.x, c.y, rad, rad, 0.f, a0, a1, true);
        g.setColour (col);
        g.strokePath (p, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };
    g.setColour (juce::Colour (0xff15170d));                          // cobre o arco original
    g.fillEllipse (c.x - R * 0.95f, c.y - R * 0.95f, R * 1.9f, R * 1.9f);
    arc (start, end, R * 0.87f, R * 0.07f, juce::Colour (0xff2a2f14));
    arc (start, ang, R * 0.87f, R * 0.07f, lime);

    const float kr = R * 0.68f;                                       // corpo do knob
    g.setColour (juce::Colours::black);
    g.fillEllipse (c.x - kr * 1.08f, c.y - kr * 1.08f, kr * 2.16f, kr * 2.16f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff7a7a7a), c.x - kr * 0.4f, c.y - kr * 0.5f,
                                             juce::Colour (0xff161616), c.x + kr, c.y + kr, true));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.f, kr * 2.f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawEllipse (c.x - kr * 0.8f, c.y - kr * 0.8f, kr * 1.6f, kr * 1.6f, 1.f);

    const juce::Point<float> d (std::sin (ang), -std::cos (ang));     // ponteiro branco
    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.drawLine ({ c + d * (kr * 0.25f), c + d * (kr * 0.95f) }, R * 0.045f);
}

void PillButton::paintButton (juce::Graphics& g, bool, bool)
{
    auto b = getLocalBounds().toFloat().reduced (2.f);
    const bool on = getToggleState();
    g.setColour (juce::Colour (0xff14150f));
    g.fillRoundedRectangle (b, 8.f);
    g.setColour (on ? lime : juce::Colour (0xff3a3a3a));
    g.drawRoundedRectangle (b, 8.f, on ? 3.f : 2.f);
    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.setFont (juce::FontOptions (b.getHeight() * 0.38f, juce::Font::bold));
    g.drawText (label, b.reduced (6.f, 0.f), juce::Justification::centred);
}

VelvetSlapEditor::VelvetSlapEditor (VelvetSlapAudioProcessor& p) : AudioProcessorEditor (&p)
{
    bg = juce::ImageCache::getFromMemory (BinaryData::bg_png, BinaryData::bg_pngSize);
    const char* ids[5] = { "mix", "ambience", "width", "delay", "sccomp" };
    for (int i = 0; i < 5; ++i)
    {
        knobs[i].setSliderStyle (juce::Slider::RotaryVerticalDrag);
        knobs[i].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        knobs[i].setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                      juce::MathConstants<float>::pi * 2.75f, true);
        knobs[i].setLookAndFeel (&lnf);
        knobs[i].onValueChange = [this] { repaint(); };
        addAndMakeVisible (knobs[i]);
        ka[i] = std::make_unique<SA> (p.apvts, ids[i], knobs[i]);
    }
    wetBtn.label = "Wet Only"; bypBtn.label = "Bypass";
    addAndMakeVisible (wetBtn); addAndMakeVisible (bypBtn);
    wa = std::make_unique<BA> (p.apvts, "wetonly", wetBtn);
    ba = std::make_unique<BA> (p.apvts, "bypass", bypBtn);

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio (1672.0 / 941.0);
    setResizeLimits (627, 353, 1672, 941);
    setSize (836, 470);
}

static juce::Colour sampleColour (const juce::Image& im, juce::Rectangle<int> r)
{
    const int pts[3][2] = { { r.getX() - 6, r.getCentreY() }, { r.getRight() + 6, r.getCentreY() }, { r.getCentreX(), r.getBottom() + 6 } };
    float rr = 0, gg = 0, bb = 0;
    for (auto& p : pts)
    {
        auto c = im.getPixelAt (juce::jlimit (0, im.getWidth() - 1, p[0]), juce::jlimit (0, im.getHeight() - 1, p[1]));
        rr += c.getFloatRed(); gg += c.getFloatGreen(); bb += c.getFloatBlue();
    }
    return juce::Colour::fromFloatRGBA (rr / 3.f, gg / 3.f, bb / 3.f, 1.f);
}

juce::String VelvetSlapEditor::readout (int i) const
{
    const auto v = (float) knobs[i].getValue();
    if (i == 3) return juce::String (juce::roundToInt (v)) + " ms";
    return juce::String (juce::roundToInt (v * 100.f)) + " %";
}

void VelvetSlapEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.drawImage (bg, getLocalBounds().toFloat());
    const float s = getWidth() / 1672.f, k = bg.getWidth() / 1672.f;
    const float cx[5] = { 210.f, 525.f, 833.f, 1143.f, 1453.f };

    // troca os rotulos "0%" e "100%" do Delay por ms
    const juce::Rectangle<float> pl (cx[3] - 125.f, 588.f, 62.f, 36.f), pr (cx[3] + 52.f, 588.f, 82.f, 36.f);
    for (auto r : { pl, pr })
    {
        g.setColour (sampleColour (bg, { (int) (r.getX() * k), (int) (r.getY() * k), (int) (r.getWidth() * k), (int) (r.getHeight() * k) }));
        g.fillRoundedRectangle (r * s, 6.f * s);
    }
    g.setColour (juce::Colour (0xffe0e0e0));
    g.setFont (juce::FontOptions (25.f * s, juce::Font::bold));
    g.drawText ("20 ms",  (pl.withTrimmedLeft (12.f) * s), juce::Justification::centredLeft);
    g.drawText ("180 ms", (pr.withTrimmedRight (8.f) * s), juce::Justification::centredRight);

    // valor atual de cada knob
    for (int i = 0; i < 5; ++i)
    {
        auto r = juce::Rectangle<float> (96.f, 32.f).withCentre ({ cx[i], 604.f }) * s;
        g.setColour (juce::Colour (0xd90c0d08));
        g.fillRoundedRectangle (r, 8.f * s);
        g.setColour (lime.withAlpha (0.55f));
        g.drawRoundedRectangle (r, 8.f * s, 1.5f);
        g.setColour (lime);
        g.setFont (juce::FontOptions (24.f * s, juce::Font::bold));
        g.drawText (readout (i), r, juce::Justification::centred);
    }
}

void VelvetSlapEditor::resized()
{
    const float s = getWidth() / 1672.f;                              // coordenadas da imagem original
    const float cx[5] = { 210.f, 525.f, 833.f, 1143.f, 1453.f };
    for (int i = 0; i < 5; ++i)
        knobs[i].setBounds (juce::Rectangle<float> (260.f * s, 260.f * s)
                                .withCentre ({ cx[i] * s, 488.f * s }).toNearestInt());
    wetBtn.setBounds (juce::Rectangle<float> (1240.f * s, 202.f * s, 183.f * s, 86.f * s).toNearestInt());
    bypBtn.setBounds (juce::Rectangle<float> (1440.f * s, 202.f * s, 190.f * s, 90.f * s).toNearestInt());
}
