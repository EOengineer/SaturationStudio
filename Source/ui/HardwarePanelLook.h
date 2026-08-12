#pragma once

#include <JuceHeader.h>

/** Brushed / aged metal panel painting helpers (Pultec / Neve inspired). */
namespace HardwarePanelLook
{
inline juce::Colour panelBase()   { return juce::Colour (0xff3a3530); }
inline juce::Colour panelLight()  { return juce::Colour (0xff5a534c); }
inline juce::Colour panelDark()   { return juce::Colour (0xff1c1916); }
inline juce::Colour accentBrass() { return juce::Colour (0xffc4a35a); }
inline juce::Colour engraving()   { return juce::Colour (0xffd8cbb0); }

inline void paintPanel (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    juce::ColourGradient grad (panelLight(), (float) bounds.getX(), (float) bounds.getY(),
                               panelDark(), (float) bounds.getX(), (float) bounds.getBottom(), false);
    g.setGradientFill (grad);
    g.fillAll();

    // Subtle horizontal brush lines
    g.setColour (juce::Colours::white.withAlpha (0.03f));
    for (int y = bounds.getY(); y < bounds.getBottom(); y += 3)
        g.drawHorizontalLine (y, (float) bounds.getX(), (float) bounds.getRight());

    // Vignette
    juce::ColourGradient vig (juce::Colours::transparentBlack, bounds.getCentreX(), bounds.getCentreY(),
                              juce::Colours::black.withAlpha (0.45f),
                              (float) bounds.getX(), (float) bounds.getY(), true);
    g.setGradientFill (vig);
    g.fillAll();

    // Edge bevel
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRect (bounds.toFloat().reduced (1.0f), 1.5f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRect (bounds.toFloat(), 2.0f);
}

inline void paintBrand (juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour (engraving().withAlpha (0.9f));
    g.setFont (juce::Font (juce::FontOptions ("Georgia", 28.0f, juce::Font::plain)));
    g.drawText ("SaturationStudio", area, juce::Justification::centredLeft, false);
    g.setColour (accentBrass().withAlpha (0.7f));
    g.setFont (juce::Font (juce::FontOptions ("Georgia", 12.0f, juce::Font::italic)));
    g.drawText ("ANALOG MODELED SATURATION", area.translated (0, 22),
                juce::Justification::centredLeft, false);
}
} // namespace HardwarePanelLook

/** Skirted rotary look for primary knobs. */
class HardwareLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    HardwareLookAndFeel()
    {
        setColour (juce::Slider::rotarySliderFillColourId, HardwarePanelLook::accentBrass());
        setColour (juce::Slider::rotarySliderOutlineColourId, HardwarePanelLook::panelDark());
        setColour (juce::ComboBox::backgroundColourId, HardwarePanelLook::panelDark());
        setColour (juce::ComboBox::outlineColourId, HardwarePanelLook::accentBrass().withAlpha (0.5f));
        setColour (juce::ComboBox::textColourId, HardwarePanelLook::engraving());
        setColour (juce::PopupMenu::backgroundColourId, HardwarePanelLook::panelDark());
        setColour (juce::PopupMenu::textColourId, HardwarePanelLook::engraving());
        setColour (juce::Label::textColourId, HardwarePanelLook::engraving());
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        juce::ignoreUnused (slider);
        const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
        const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Skirt / base
        g.setColour (HardwarePanelLook::panelDark());
        g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

        juce::ColourGradient body (HardwarePanelLook::panelLight(), centre.x - radius * 0.4f, centre.y - radius * 0.5f,
                                   HardwarePanelLook::panelBase(), centre.x + radius * 0.3f, centre.y + radius * 0.5f, false);
        g.setGradientFill (body);
        g.fillEllipse (centre.x - radius * 0.88f, centre.y - radius * 0.88f, radius * 1.76f, radius * 1.76f);

        g.setColour (HardwarePanelLook::accentBrass().withAlpha (0.35f));
        g.drawEllipse (centre.x - radius * 0.88f, centre.y - radius * 0.88f, radius * 1.76f, radius * 1.76f, 1.5f);

        juce::Path pointer;
        const float pointerLen = radius * 0.72f;
        pointer.startNewSubPath (centre.x, centre.y - pointerLen * 0.15f);
        pointer.lineTo (centre.x, centre.y - pointerLen);
        g.setColour (HardwarePanelLook::engraving());
        g.strokePath (pointer, juce::PathStrokeType (2.5f),
                      juce::AffineTransform::rotation (angle, centre.x, centre.y));

        g.setColour (HardwarePanelLook::accentBrass());
        g.fillEllipse (centre.x - 3.5f, centre.y - 3.5f, 7.0f, 7.0f);
    }
};
