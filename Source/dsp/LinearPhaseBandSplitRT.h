#pragma once

#include "FirDesign.h"
#include <JuceHeader.h>
#include <atomic>
#include <vector>

/**
 * Realtime linear-phase band split using juce::dsp::Convolution (FFT / partitioned).
 *
 * Cutoff changes are applied on the message thread via AsyncUpdater so the audio
 * thread never designs FIRs, allocates IRs, or re-prepares convolvers.
 */
class LinearPhaseBandSplitRT final : private juce::AsyncUpdater
{
public:
    static constexpr int kNumTaps = fir::kDefaultNumTaps;

    LinearPhaseBandSplitRT()
        : hpConv (juce::dsp::Convolution::Latency { 0 }),
          lpConv (juce::dsp::Convolution::Latency { 0 })
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
        hpConv.prepare (spec);
        lpConv.prepare (spec);
        // Second prepare flushes the queued IR so process() starts with the new coeffs.
        hpConv.prepare (spec);
        lpConv.prepare (spec);

        updateLatency();
        for (auto& d : delays)
        {
            d.setDelay (latency);
            d.reset();
        }

        hpConv.reset();
        lpConv.reset();
    }

    void reset()
    {
        hpConv.reset();
        lpConv.reset();
        for (auto& d : delays)
            d.reset();
    }

    /**
     * Wait-free from the audio thread: stores targets and schedules a message-thread rebuild.
     * During prepare (before audio runs) rebuilds synchronously.
     */
    void setCutoffs (float lowHz, float highHz)
    {
        lowHz = juce::jlimit (20.0f, 2000.0f, lowHz);
        highHz = juce::jlimit (200.0f, 20000.0f, highHz);
        if (lowHz >= highHz)
            highHz = juce::jmin (20000.0f, lowHz + 50.0f);

        pendingLow.store (lowHz, std::memory_order_relaxed);
        pendingHigh.store (highHz, std::memory_order_relaxed);

        // Ignore sub-Hz chatter from parameter polling
        if (std::abs (lowHz - appliedLow) < 0.5f && std::abs (highHz - appliedHigh) < 0.5f)
            return;

        if (! prepared)
        {
            appliedLow = lowHz;
            appliedHigh = highHz;
            return;
        }

        // Coalesced: many knob moves → one handleAsyncUpdate with latest pending values
        triggerAsyncUpdate();
    }

    float getLowCutHz() const noexcept { return pendingLow.load (std::memory_order_relaxed); }
    float getHighCutHz() const noexcept { return pendingHigh.load (std::memory_order_relaxed); }
    int getLatencySamples() const noexcept { return latency; }

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
            hpConv.process (ctx);
            lpConv.process (ctx);
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

        // loadImpulseResponse is wait-free / background-processed; do not prepare/reset
        // here — that was blowing Logic's CPU when twisting EQ knobs.
        rebuildFilters (low, high);
    }

    void rebuildFilters (float lowHz, float highHz)
    {
        const auto hpCoeffs = fir::designHighpass (kNumTaps, sampleRate, (double) lowHz);
        const auto lpCoeffs = fir::designLowpass (kNumTaps, sampleRate, (double) highHz);

        loadMonoOrStereoIr (hpConv, hpCoeffs);
        loadMonoOrStereoIr (lpConv, lpCoeffs);
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
        const int firGroup = 2 * ((kNumTaps - 1) / 2);
        latency = firGroup + hpConv.getLatency() + lpConv.getLatency();
    }

    juce::dsp::Convolution hpConv;
    juce::dsp::Convolution lpConv;
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
