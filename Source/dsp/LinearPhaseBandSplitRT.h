#pragma once

#include "FirDesign.h"
#include <JuceHeader.h>
#include <atomic>
#include <vector>

/**
 * Realtime linear-phase band split using juce::dsp::Convolution (FFT / partitioned).
 *
 * Pre:  mid = BP(x), side = delayed(x) − mid
 * Post: same BP IR on a second convolver — applied to wet mid after saturation so
 *       clipper harmonics outside the band don’t comb against the bypassed side.
 *
 * One IR load updates both convolvers so cutoff changes stay matched.
 */
class LinearPhaseBandSplitRT final : private juce::AsyncUpdater
{
public:
    static constexpr int kNumTaps = fir::kDefaultNumTaps; // per prototype HP/LP

    static int bandpassGroupDelay() noexcept
    {
        return 2 * ((kNumTaps - 1) / 2); // cascade of two odd-N linear-phase FIRs
    }

    LinearPhaseBandSplitRT()
        : bpPre (juce::dsp::Convolution::Latency { 0 }),
          bpPost (juce::dsp::Convolution::Latency { 0 })
    {
    }

    ~LinearPhaseBandSplitRT() override
    {
        cancelPendingUpdate();
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        cancelPendingUpdate();

        sampleRate = spec.sampleRate;
        maxBlock = (int) spec.maximumBlockSize;
        numChannels = (int) spec.numChannels;
        prepared = true;

        workBuffer.setSize (numChannels, maxBlock, false, true, true);

        delays.clear();
        delays.resize ((size_t) numChannels);

        const float low = pendingLow.load (std::memory_order_relaxed);
        const float high = pendingHigh.load (std::memory_order_relaxed);
        appliedLow = low;
        appliedHigh = high;

        rebuildFilters (low, high);
        bpPre.prepare (spec);
        bpPost.prepare (spec);
        // Second prepare flushes queued IRs.
        bpPre.prepare (spec);
        bpPost.prepare (spec);

        updateLatency();
        for (auto& d : delays)
        {
            d.setDelay (latency);
            d.reset();
        }

        bpPre.reset();
        bpPost.reset();
    }

    void reset()
    {
        bpPre.reset();
        bpPost.reset();
        for (auto& d : delays)
            d.reset();
    }

    void setCutoffs (float lowHz, float highHz)
    {
        lowHz = juce::jlimit (20.0f, 2000.0f, lowHz);
        highHz = juce::jlimit (200.0f, 20000.0f, highHz);
        if (lowHz >= highHz)
            highHz = juce::jmin (20000.0f, lowHz + 50.0f);

        pendingLow.store (lowHz, std::memory_order_relaxed);
        pendingHigh.store (highHz, std::memory_order_relaxed);

        if (std::abs (lowHz - appliedLow) < 0.5f && std::abs (highHz - appliedHigh) < 0.5f)
            return;

        if (! prepared)
        {
            appliedLow = lowHz;
            appliedHigh = highHz;
            return;
        }

        triggerAsyncUpdate();
    }

    float getLowCutHz() const noexcept { return pendingLow.load (std::memory_order_relaxed); }
    float getHighCutHz() const noexcept { return pendingHigh.load (std::memory_order_relaxed); }

    /** Pre-split complementary delay (BP group + engine latency). */
    int getLatencySamples() const noexcept { return latency; }

    /** Extra delay of processPostBandpass (same BP group delay). */
    int getPostLatencySamples() const noexcept { return bandpassGroupDelay() + bpPost.getLatency(); }

    void process (const juce::AudioBuffer<float>& input,
                  juce::AudioBuffer<float>& midOut,
                  juce::AudioBuffer<float>& outOfBandOut,
                  int numSamples)
    {
        const int chans = juce::jmin (numChannels, input.getNumChannels());
        if (numSamples <= 0 || chans <= 0 || ! prepared)
            return;

        for (int c = 0; c < chans; ++c)
            workBuffer.copyFrom (c, 0, input, c, 0, numSamples);

        {
            juce::dsp::AudioBlock<float> block (workBuffer);
            juce::dsp::AudioBlock<float> sub = block.getSubBlock (0, (size_t) numSamples);
            juce::dsp::ProcessContextReplacing<float> ctx (sub);
            bpPre.process (ctx);
        }

        for (int c = 0; c < chans; ++c)
        {
            midOut.copyFrom (c, 0, workBuffer, c, 0, numSamples);

            const float* in = input.getReadPointer (c);
            float* mid = midOut.getWritePointer (c);
            float* side = outOfBandOut.getWritePointer (c);
            auto& delay = delays[(size_t) c];

            for (int i = 0; i < numSamples; ++i)
            {
                const float delayed = delay.processSample (in[i]);
                side[i] = delayed - mid[i];
            }
        }
    }

    /** Same BP as the split, for wet mid after saturation (in-place). */
    void processPostBandpass (juce::AudioBuffer<float>& io, int numSamples)
    {
        const int chans = juce::jmin (numChannels, io.getNumChannels());
        if (numSamples <= 0 || chans <= 0 || ! prepared)
            return;

        juce::dsp::AudioBlock<float> block (io);
        juce::dsp::AudioBlock<float> sub = block.getSubBlock (0, (size_t) numSamples);
        juce::dsp::ProcessContextReplacing<float> ctx (sub);
        bpPost.process (ctx);
    }

private:
    void handleAsyncUpdate() override
    {
        if (! prepared)
            return;

        const float low = pendingLow.load (std::memory_order_relaxed);
        const float high = pendingHigh.load (std::memory_order_relaxed);

        if (std::abs (low - appliedLow) < 0.5f && std::abs (high - appliedHigh) < 0.5f)
            return;

        appliedLow = low;
        appliedHigh = high;
        rebuildFilters (low, high);
    }

    void rebuildFilters (float lowHz, float highHz)
    {
        const auto hpCoeffs = fir::designHighpass (kNumTaps, sampleRate, (double) lowHz);
        const auto lpCoeffs = fir::designLowpass (kNumTaps, sampleRate, (double) highHz);
        const auto bpCoeffs = fir::cascade (hpCoeffs, lpCoeffs);
        loadMonoOrStereoIr (bpPre, bpCoeffs);
        loadMonoOrStereoIr (bpPost, bpCoeffs);
    }

    void loadMonoOrStereoIr (juce::dsp::Convolution& conv, const std::vector<float>& coeffs)
    {
        const int taps = (int) coeffs.size();
        juce::AudioBuffer<float> ir (numChannels, taps);
        for (int c = 0; c < numChannels; ++c)
            ir.copyFrom (c, 0, coeffs.data(), taps);

        const auto stereo = numChannels > 1 ? juce::dsp::Convolution::Stereo::yes
                                            : juce::dsp::Convolution::Stereo::no;
        conv.loadImpulseResponse (std::move (ir),
                                  sampleRate,
                                  stereo,
                                  juce::dsp::Convolution::Trim::no,
                                  juce::dsp::Convolution::Normalise::no);
    }

    void updateLatency()
    {
        latency = bandpassGroupDelay() + bpPre.getLatency();
    }

    juce::dsp::Convolution bpPre;
    juce::dsp::Convolution bpPost;
    juce::AudioBuffer<float> workBuffer;
    std::vector<fir::DelayLine> delays;

    double sampleRate = 48000.0;
    int maxBlock = 512;
    int numChannels = 2;
    int latency = 0;
    bool prepared = false;

    std::atomic<float> pendingLow { 20.0f };
    std::atomic<float> pendingHigh { 20000.0f };
    float appliedLow = 20.0f;
    float appliedHigh = 20000.0f;
};
