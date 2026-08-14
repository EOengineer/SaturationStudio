#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/HardwarePanelLook.h"
#include "ui/SpectrumMeter.h"
#include "ui/ModelPicker.h"
#include "ui/ParamHost.h"

class SaturationStudioAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                   private juce::Timer
{
public:
    explicit SaturationStudioAudioProcessorEditor (SaturationStudioAudioProcessor&);
    ~SaturationStudioAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupKnob (juce::Slider& s, juce::Label& label, const juce::String& text);
    void refreshFlavorHost();

    SaturationStudioAudioProcessor& processor;
    HardwareLookAndFeel lookAndFeel;

    SpectrumMeter spectrum;
    ModelPicker modelPicker;
    ParamHost paramHost;

    juce::Slider lowCutSlider, highCutSlider, driveSlider, bandSlider, mixSlider;
    juce::Label lowCutLabel, highCutLabel, driveLabel, bandLabel, mixLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> lowCutAttachment, highCutAttachment, driveAttachment,
                                      bandAttachment, mixAttachment;
    std::unique_ptr<ComboAttachment> modelAttachment, diodeFlavorAttachment, tubeFlavorAttachment, preampFlavorAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaturationStudioAudioProcessorEditor)
};
