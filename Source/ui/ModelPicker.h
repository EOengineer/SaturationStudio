#pragma once

#include <JuceHeader.h>
#include "HardwarePanelLook.h"

/** Collapsible model family picker. */
class ModelPicker final : public juce::Component
{
public:
    ModelPicker()
    {
        collapseButton.setButtonText ("Model");
        collapseButton.setClickingTogglesState (true);
        collapseButton.setToggleState (true, juce::dontSendNotification);
        collapseButton.onClick = [this]
        {
            expanded = collapseButton.getToggleState();
            modelBox.setVisible (expanded);
            if (onExpandedChanged)
                onExpandedChanged();
            resized();
        };
        addAndMakeVisible (collapseButton);
        addAndMakeVisible (modelBox);
        modelBox.addItemList ({ "Diode", "Tube" }, 1);
        modelBox.setSelectedItemIndex (0, juce::dontSendNotification);
    }

    juce::ComboBox& getCombo() noexcept { return modelBox; }
    juce::TextButton& getCollapseButton() noexcept { return collapseButton; }
    bool isExpanded() const noexcept { return expanded; }

    std::function<void()> onExpandedChanged;

    void resized() override
    {
        auto r = getLocalBounds();
        collapseButton.setBounds (r.removeFromTop (28));
        if (expanded)
            modelBox.setBounds (r.removeFromTop (28).reduced (0, 2));
    }

    int getPreferredHeight() const noexcept { return expanded ? 60 : 28; }

private:
    juce::TextButton collapseButton;
    juce::ComboBox modelBox;
    bool expanded = true;
};
