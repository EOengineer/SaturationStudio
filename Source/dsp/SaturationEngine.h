#pragma once

#include "LinearPhaseBandSplitRT.h"
#include "Oversampler.h"
#include "models/ModelRegistry.h"
#include <JuceHeader.h>
#include <cmath>
#include <memory>
#include <vector>

/**
 * Owns band split, oversampler island, and active saturation model.
 *
 * Path:
 *   split (BP pre) → mid
 *     → dry mid / side delayed by (OS + BP post)
 *     → mid → OS↑ → model → OS↓ → BP post
 *   → out = mix(wet, dryMid) + side
 *
 * Post-BP keeps clipper harmonics inside the selected band so they don’t comb
 * against the bypassed out-of-band side (the “filters sound phasey at Mix=1” case).
 */
class SaturationEngine
{
public:
    SaturationEngine()
    {
        model = ModelRegistry::createById (ModelRegistry::defaultFamilyId());
    }

    void prepare (const juce::dsp::ProcessSpec& spec, size_t oversampleFactor = Oversampler::kDefaultFactor)
    {
        sampleRate = spec.sampleRate;
        maxBlock = (int) spec.maximumBlockSize;
        numChannels = juce::jmax (1, (int) spec.numChannels);

        juce::dsp::ProcessSpec engineSpec = spec;
        engineSpec.numChannels = (juce::uint32) numChannels;

        split.prepare (engineSpec);
        split.setCutoffs (lowCutHz, highCutHz);

        oversampler.prepare (engineSpec, oversampleFactor);
        osLatency = oversampler.getLatencySamples();
        postBpLatency = split.getPostLatencySamples();

        midBuffer.setSize (numChannels, maxBlock, false, true, true);
        sideBuffer.setSize (numChannels, maxBlock, false, true, true);
        dryMidBuffer.setSize (numChannels, maxBlock, false, true, true);
        inputScratch.setSize (numChannels, maxBlock, false, true, true);
        osPtrs.resize ((size_t) numChannels);

        resizeMatchDelays();

        if (model != nullptr)
            model->prepare (sampleRate * (double) oversampler.getFactor(),
                            maxBlock * (int) oversampler.getFactor(),
                            numChannels);

        reportedLatency = split.getLatencySamples() + osLatency + postBpLatency;
        reset();
    }

    void reset()
    {
        split.reset();
        oversampler.reset();
        if (model != nullptr)
            model->reset();
        for (auto& d : matchDelaysSide)
            d.reset();
        for (auto& d : matchDelaysDryMid)
            d.reset();
        midBandRms = 0.0f;
    }

    void setCutoffs (float lowHz, float highHz)
    {
        if (lowHz >= highHz)
            highHz = std::min (20000.0f, lowHz + 50.0f);
        lowCutHz = lowHz;
        highCutHz = highHz;
        split.setCutoffs (lowCutHz, highCutHz);
        postBpLatency = split.getPostLatencySamples();
        resizeMatchDelays();
        reportedLatency = split.getLatencySamples() + osLatency + postBpLatency;
    }

    void setDrive (float d)
    {
        drive = juce::jlimit (0.0f, 1.0f, d);
        if (model != nullptr)
            model->setDrive (drive);
    }

    void setMix (float m) { mix = juce::jlimit (0.0f, 1.0f, m); }
    void setOutputDb (float db) { outputDb = juce::jlimit (-24.0f, 24.0f, db); }

    void setModelFamily (int familyIndex)
    {
        familyIndex = juce::jlimit (0, (int) ModelRegistry::families().size() - 1, familyIndex);
        if (familyIndex == currentFamily && model != nullptr)
            return;

        currentFamily = familyIndex;
        model = ModelRegistry::create (currentFamily);
        if (model != nullptr)
        {
            model->prepare (sampleRate * (double) oversampler.getFactor(),
                            maxBlock * (int) oversampler.getFactor(),
                            numChannels);
            model->setDrive (drive);
            model->setDiodeFlavor (diodeFlavor);
            model->setTubeFlavor (tubeFlavor);
            model->setPreampFlavor (preampFlavor);
        }
    }

    void setDiodeFlavor (int flavor)
    {
        diodeFlavor = flavor;
        if (model != nullptr)
            model->setDiodeFlavor (flavor);
    }

    void setTubeFlavor (int flavor)
    {
        tubeFlavor = flavor;
        if (model != nullptr)
            model->setTubeFlavor (flavor);
    }

    void setPreampFlavor (int flavor)
    {
        preampFlavor = flavor;
        if (model != nullptr)
            model->setPreampFlavor (flavor);
    }

    int getLatencySamples() const noexcept { return reportedLatency; }
    float getLowCutHz() const noexcept { return split.getLowCutHz(); }
    float getHighCutHz() const noexcept { return split.getHighCutHz(); }
    float getMidBandRms() const noexcept { return midBandRms; }
    int getCurrentFamily() const noexcept { return currentFamily; }
    SaturationModel* getModel() noexcept { return model.get(); }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int n = buffer.getNumSamples();
        const int hostChans = buffer.getNumChannels();
        if (n <= 0 || hostChans <= 0 || model == nullptr || maxBlock <= 0)
            return;

        const int nClamped = juce::jmin (n, maxBlock);

        inputScratch.clear();
        for (int c = 0; c < numChannels; ++c)
        {
            if (c < hostChans)
                inputScratch.copyFrom (c, 0, buffer, c, 0, nClamped);
            else if (hostChans > 0)
                inputScratch.copyFrom (c, 0, buffer, 0, 0, nClamped);
        }

        midBuffer.clear();
        sideBuffer.clear();
        dryMidBuffer.clear();

        split.process (inputScratch, midBuffer, sideBuffer, nClamped);

        for (int c = 0; c < numChannels; ++c)
            dryMidBuffer.copyFrom (c, 0, midBuffer, c, 0, nClamped);

        {
            double acc = 0.0;
            int count = 0;
            for (int c = 0; c < numChannels; ++c)
            {
                const float* m = midBuffer.getReadPointer (c);
                for (int i = 0; i < nClamped; ++i)
                {
                    acc += (double) m[i] * (double) m[i];
                    ++count;
                }
            }
            const float rms = count > 0 ? (float) std::sqrt (acc / (double) count) : 0.0f;
            midBandRms = midBandRms * 0.9f + rms * 0.1f;
        }

        // Align dry mid + side to wet path: OS latency + post-BP latency
        for (int c = 0; c < numChannels; ++c)
        {
            float* side = sideBuffer.getWritePointer (c);
            float* dryMid = dryMidBuffer.getWritePointer (c);
            for (int i = 0; i < nClamped; ++i)
            {
                side[i] = matchDelaysSide[(size_t) c].processSample (side[i]);
                dryMid[i] = matchDelaysDryMid[(size_t) c].processSample (dryMid[i]);
            }
        }

        {
            juce::dsp::AudioBlock<float> midBlock (midBuffer);
            juce::dsp::AudioBlock<float> midSub = midBlock.getSubBlock (0, (size_t) nClamped);
            auto osBlock = oversampler.processSamplesUp (midSub);
            const int osChans = (int) osBlock.getNumChannels();
            const int osN = (int) osBlock.getNumSamples();
            if ((int) osPtrs.size() < osChans)
                osPtrs.resize ((size_t) osChans);
            for (int c = 0; c < osChans; ++c)
                osPtrs[(size_t) c] = osBlock.getChannelPointer ((size_t) c);
            model->process (osPtrs.data(), osChans, osN);
            oversampler.processSamplesDown (midSub);
        }

        // Keep saturated mid in-band before summing with side
        split.processPostBandpass (midBuffer, nClamped);

        const float wet = mix;
        const float outGain = juce::Decibels::decibelsToGain (outputDb);

        for (int c = 0; c < hostChans; ++c)
        {
            const int src = juce::jmin (c, numChannels - 1);
            float* out = buffer.getWritePointer (c);
            const float* mid = midBuffer.getReadPointer (src);
            const float* dryMid = dryMidBuffer.getReadPointer (src);
            const float* side = sideBuffer.getReadPointer (src);
            for (int i = 0; i < nClamped; ++i)
            {
                // delay_full + mix * (wet - dry)  with delay_full = dryMid + side
                const float midMix = mid[i] * wet + dryMid[i] * (1.0f - wet);
                out[i] = (midMix + side[i]) * outGain;
            }
        }
    }

private:
    void resizeMatchDelays()
    {
        const int match = osLatency + postBpLatency;
        const bool chansChanged = (int) matchDelaysSide.size() != numChannels
                               || (int) matchDelaysDryMid.size() != numChannels;
        if (chansChanged)
        {
            matchDelaysSide.resize ((size_t) numChannels);
            matchDelaysDryMid.resize ((size_t) numChannels);
        }

        // setDelay() clears history — only call when length actually changes.
        // syncEngineFromParams() hits setCutoffs every block; wiping here made
        // dryMid/side silent (delay >> block size) so Mix only faded wet.
        for (auto& d : matchDelaysSide)
            if (chansChanged || d.getDelay() != match)
                d.setDelay (match);
        for (auto& d : matchDelaysDryMid)
            if (chansChanged || d.getDelay() != match)
                d.setDelay (match);
    }

    LinearPhaseBandSplitRT split;
    Oversampler oversampler;
    std::unique_ptr<SaturationModel> model;
    juce::AudioBuffer<float> midBuffer, sideBuffer, dryMidBuffer, inputScratch;
    std::vector<fir::DelayLine> matchDelaysSide;
    std::vector<fir::DelayLine> matchDelaysDryMid;
    std::vector<float*> osPtrs;

    double sampleRate = 48000.0;
    int maxBlock = 512;
    int numChannels = 2;
    int osLatency = 0;
    int postBpLatency = 0;
    int reportedLatency = 0;
    int currentFamily = 0;
    int diodeFlavor = 0;
    int tubeFlavor = 0;
    int preampFlavor = 0;
    float lowCutHz = 20.0f;
    float highCutHz = 20000.0f;
    float drive = 0.5f;
    float mix = 1.0f;
    float outputDb = 0.0f;
    float midBandRms = 0.0f;
};
