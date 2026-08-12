#include "PluginEditor.h"
#include "util/ParamIDs.h"

SaturationStudioAudioProcessorEditor::SaturationStudioAudioProcessorEditor (SaturationStudioAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    setSize (780, 520);

    spectrum.setAnalyzer (&processor.getAnalyzer());
    addAndMakeVisible (spectrum);

    setupKnob (lowCutSlider, lowCutLabel, "Low Cut");
    setupKnob (highCutSlider, highCutLabel, "High Cut");
    setupKnob (driveSlider, driveLabel, "Drive");
    setupKnob (bandSlider, bandLabel, "Band");
    setupKnob (mixSlider, mixLabel, "Mix");

    lowCutAttachment = std::make_unique<SliderAttachment> (processor.getAPVTS(), ParamIDs::lowCutHz, lowCutSlider);
    highCutAttachment = std::make_unique<SliderAttachment> (processor.getAPVTS(), ParamIDs::highCutHz, highCutSlider);
    driveAttachment = std::make_unique<SliderAttachment> (processor.getAPVTS(), ParamIDs::drive, driveSlider);
    bandAttachment = std::make_unique<SliderAttachment> (processor.getAPVTS(), ParamIDs::outputDb, bandSlider);
    mixAttachment = std::make_unique<SliderAttachment> (processor.getAPVTS(), ParamIDs::mix, mixSlider);

    addAndMakeVisible (modelPicker);
    modelAttachment = std::make_unique<ComboAttachment> (processor.getAPVTS(), ParamIDs::satModel, modelPicker.getCombo());
    modelPicker.getCombo().onChange = [this] { refreshFlavorHost(); };
    modelPicker.onExpandedChanged = [this] { resized(); };

    addAndMakeVisible (paramHost);
    refreshFlavorHost();

    startTimerHz (30);
}

SaturationStudioAudioProcessorEditor::~SaturationStudioAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void SaturationStudioAudioProcessorEditor::setupKnob (juce::Slider& s, juce::Label& label, const juce::String& text)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
    addAndMakeVisible (s);
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}

void SaturationStudioAudioProcessorEditor::refreshFlavorHost()
{
    diodeFlavorAttachment.reset();
    preampFlavorAttachment.reset();

    const int family = modelPicker.getCombo().getSelectedItemIndex();
    auto& box = paramHost.getFlavorBox();

    if (family == 0)
    {
        paramHost.showDiodeFlavors();
        diodeFlavorAttachment = std::make_unique<ComboAttachment> (
            processor.getAPVTS(), ParamIDs::diodeFlavor, box);
    }
    else if (family == 4)
    {
        paramHost.showPreampFlavors();
        preampFlavorAttachment = std::make_unique<ComboAttachment> (
            processor.getAPVTS(), ParamIDs::preampFlavor, box);
    }
    else
    {
        const auto name = modelPicker.getCombo().getText();
        paramHost.showComingSoon (name);
    }

    paramHost.resized();
}

void SaturationStudioAudioProcessorEditor::timerCallback()
{
    if (auto* low = processor.getAPVTS().getRawParameterValue (ParamIDs::lowCutHz))
        if (auto* high = processor.getAPVTS().getRawParameterValue (ParamIDs::highCutHz))
            spectrum.setBandRange (low->load(), high->load());
    spectrum.timerUpdate();
}

void SaturationStudioAudioProcessorEditor::paint (juce::Graphics& g)
{
    HardwarePanelLook::paintPanel (g, getLocalBounds());
    HardwarePanelLook::paintBrand (g, getLocalBounds().removeFromTop (56).reduced (20, 8));
}

void SaturationStudioAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (16);
    r.removeFromTop (56); // brand

    spectrum.setBounds (r.removeFromTop (150));
    r.removeFromTop (12);

    auto modelArea = r.removeFromLeft (200);
    modelPicker.setBounds (modelArea.removeFromTop (modelPicker.getPreferredHeight()));
    modelArea.removeFromTop (8);
    paramHost.setBounds (modelArea.removeFromTop (70));

    r.removeFromLeft (12);
    auto knobs = r.removeFromTop (220);
    const int knobW = knobs.getWidth() / 5;
    auto place = [&] (juce::Slider& s, juce::Label& lab)
    {
        auto cell = knobs.removeFromLeft (knobW).reduced (4);
        lab.setBounds (cell.removeFromTop (22));
        s.setBounds (cell);
    };
    place (lowCutSlider, lowCutLabel);
    place (highCutSlider, highCutLabel);
    place (driveSlider, driveLabel);
    place (mixSlider, mixLabel);
    place (bandSlider, bandLabel);
}
