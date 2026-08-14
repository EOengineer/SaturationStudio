#pragma once

#include <JuceHeader.h>
#include "HardwarePanelLook.h"

/** Hosts flavor / per-model controls that change with the selected family. */
class ParamHost final : public juce::Component
{
public:
    ParamHost()
    {
        flavorLabel.setText ("Flavor", juce::dontSendNotification);
        flavorLabel.attachToComponent (&flavorBox, true);
        addAndMakeVisible (flavorLabel);
        addAndMakeVisible (flavorBox);
        hintLabel.setJustificationType (juce::Justification::centredLeft);
        hintLabel.setColour (juce::Label::textColourId, HardwarePanelLook::engraving().withAlpha (0.65f));
        addAndMakeVisible (hintLabel);
    }

    juce::ComboBox& getFlavorBox() noexcept { return flavorBox; }

    void showDiodeFlavors()
    {
        flavorBox.clear (juce::dontSendNotification);
        flavorBox.addItemList ({ "Silicon", "Germanium", "LED", "Asymmetric" }, 1);
        flavorBox.setEnabled (true);
        flavorBox.setVisible (true);
        flavorLabel.setVisible (true);
        hintLabel.setText ("Diode saturator — Drive sets Rin + 4× OS", juce::dontSendNotification);
    }

    void showPreampFlavors()
    {
        flavorBox.clear (juce::dontSendNotification);
        flavorBox.addItemList ({ "Neve 1073", "API 512" }, 1);
        flavorBox.setEnabled (true);
        flavorBox.setVisible (true);
        flavorLabel.setVisible (true);
        hintLabel.setText ("Preamp saturator — 1073 round / API punch; ADAA + 4× OS",
                           juce::dontSendNotification);
    }

    void showTubeFlavors()
    {
        flavorBox.clear (juce::dontSendNotification);
        flavorBox.addItemList ({ "12AX7", "5751", "12AU7" }, 1);
        flavorBox.setEnabled (true);
        flavorBox.setVisible (true);
        flavorLabel.setVisible (true);
        hintLabel.setText ("Tube saturator — 12AX7 / 5751 / 12AU7 waveshape (mu feel); ADAA + 4× OS",
                           juce::dontSendNotification);
    }

    void showTapeLive()
    {
        flavorBox.setVisible (false);
        flavorLabel.setVisible (false);
        hintLabel.setText ("Tape saturator — soft symmetric clip, odd-leaning; ADAA + 4× OS",
                           juce::dontSendNotification);
    }

    void showTransformerLive()
    {
        flavorBox.setVisible (false);
        flavorLabel.setVisible (false);
        hintLabel.setText ("Transformer saturator — asymmetric iron feel (no hysteresis yet); ADAA + 4× OS",
                           juce::dontSendNotification);
    }

    void showComingSoon (const juce::String& /*familyName*/)
    {
        // Kept for API compat; all five families are live — never show stub copy.
        flavorBox.setVisible (false);
        flavorLabel.setVisible (false);
        hintLabel.setText ({}, juce::dontSendNotification);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        if (flavorBox.isVisible())
            flavorBox.setBounds (r.removeFromTop (28).withTrimmedLeft (70));
        hintLabel.setBounds (r.removeFromTop (24));
    }

private:
    juce::Label flavorLabel;
    juce::ComboBox flavorBox;
    juce::Label hintLabel;
};
