#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/LevelReference.h"
#include "util/ParamIDs.h"

SaturationStudioAudioProcessor::SaturationStudioAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

SaturationStudioAudioProcessor::~SaturationStudioAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout SaturationStudioAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::lowCutHz, 1 }, "Low Cut",
        juce::NormalisableRange<float> (20.0f, 2000.0f, 0.01f, 0.3f), 20.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::highCutHz, 1 }, "High Cut",
        juce::NormalisableRange<float> (200.0f, 20000.0f, 0.01f, 0.3f), 20000.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::drive, 1 }, "Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::mix, 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::outputDb, 1 }, "Band",
        juce::NormalisableRange<float> (LevelReference::kBandOutputMinDb,
                                        LevelReference::kBandOutputMaxDb, 0.01f),
        0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::satModel, 1 }, "Model",
        juce::StringArray { "Diode", "Tube", "Tape", "Transformer", "Preamp" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::diodeFlavor, 1 }, "Diode Flavor",
        juce::StringArray { "Silicon", "Germanium", "LED", "Asymmetric" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::preampFlavor, 1 }, "Preamp Flavor",
        juce::StringArray { "Neve 1073", "API 512" }, 0));

    return { params.begin(), params.end() };
}

void SaturationStudioAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) getTotalNumOutputChannels();

    engine.prepare (spec);
    analyzer.prepare (sampleRate);
    syncEngineFromParams();
    lastReportedLatency = -1;
    updateLatencyIfNeeded();
}

void SaturationStudioAudioProcessor::updateLatencyIfNeeded()
{
    const int latency = engine.getLatencySamples();
    if (latency != lastReportedLatency)
    {
        lastReportedLatency = latency;
        setLatencySamples (latency);
    }
}

void SaturationStudioAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SaturationStudioAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    const auto& mainIn  = layouts.getMainInputChannelSet();
    if (mainOut != mainIn)
        return false;
    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}
#endif

void SaturationStudioAudioProcessor::syncEngineFromParams()
{
    auto* low = apvts.getRawParameterValue (ParamIDs::lowCutHz);
    auto* high = apvts.getRawParameterValue (ParamIDs::highCutHz);
    auto* drive = apvts.getRawParameterValue (ParamIDs::drive);
    auto* mix = apvts.getRawParameterValue (ParamIDs::mix);
    auto* bandOut = apvts.getRawParameterValue (ParamIDs::outputDb);
    auto* model = apvts.getRawParameterValue (ParamIDs::satModel);
    auto* diode = apvts.getRawParameterValue (ParamIDs::diodeFlavor);
    auto* preamp = apvts.getRawParameterValue (ParamIDs::preampFlavor);

    float lowHz = low != nullptr ? low->load() : 20.0f;
    float highHz = high != nullptr ? high->load() : 20000.0f;
    if (lowHz >= highHz)
        highHz = std::min (20000.0f, lowHz + 50.0f);

    engine.setCutoffs (lowHz, highHz);
    engine.setDrive (drive != nullptr ? drive->load() : 0.5f);
    engine.setMix (mix != nullptr ? mix->load() : 1.0f);
    engine.setOutputDb (bandOut != nullptr ? bandOut->load() : 0.0f);
    engine.setModelFamily (model != nullptr ? (int) model->load() : 0);
    engine.setDiodeFlavor (diode != nullptr ? (int) diode->load() : 0);
    engine.setPreampFlavor (preamp != nullptr ? (int) preamp->load() : 0);
}

void SaturationStudioAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    syncEngineFromParams();
    updateLatencyIfNeeded();

    engine.process (buffer);

    analyzer.pushBlock (buffer);
    const float heat = juce::jlimit (0.0f, 1.0f,
        (apvts.getRawParameterValue (ParamIDs::drive)->load()) * (engine.getMidBandRms() * 4.0f));
    analyzer.setHeat (heat);
}

juce::AudioProcessorEditor* SaturationStudioAudioProcessor::createEditor()
{
    return new SaturationStudioAudioProcessorEditor (*this);
}

bool SaturationStudioAudioProcessor::hasEditor() const { return true; }
const juce::String SaturationStudioAudioProcessor::getName() const { return JucePlugin_Name; }
bool SaturationStudioAudioProcessor::acceptsMidi() const { return false; }
bool SaturationStudioAudioProcessor::producesMidi() const { return false; }
bool SaturationStudioAudioProcessor::isMidiEffect() const { return false; }
double SaturationStudioAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int SaturationStudioAudioProcessor::getNumPrograms() { return 1; }
int SaturationStudioAudioProcessor::getCurrentProgram() { return 0; }
void SaturationStudioAudioProcessor::setCurrentProgram (int) {}
const juce::String SaturationStudioAudioProcessor::getProgramName (int) { return {}; }
void SaturationStudioAudioProcessor::changeProgramName (int, const juce::String&) {}

void SaturationStudioAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SaturationStudioAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SaturationStudioAudioProcessor();
}
