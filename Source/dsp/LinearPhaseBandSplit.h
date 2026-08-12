#pragma once

#include "FirDesign.h"
#include <algorithm>
#include <cmath>
#include <vector>

/**
 * Linear-phase HP→LP band extract with complementary out-of-band via delayed − mid.
 * Header-only / JUCE-free so offline verifies can link without the plugin.
 */
class LinearPhaseBandSplit
{
public:
    static constexpr int kNumTaps = fir::kDefaultNumTaps;

    void prepare (double sampleRateIn, int /*maxBlock*/, int numChannelsIn)
    {
        sampleRate = std::max (1.0, sampleRateIn);
        numChannels = std::max (1, numChannelsIn);
        hpFilters.assign ((size_t) numChannels, {});
        lpFilters.assign ((size_t) numChannels, {});
        delays.assign ((size_t) numChannels, {});
        rebuildFilters (lowCutHz, highCutHz);
        reset();
    }

    void reset()
    {
        for (auto& f : hpFilters) f.reset();
        for (auto& f : lpFilters) f.reset();
        for (auto& d : delays) d.reset();
    }

    void setCutoffs (float lowHz, float highHz)
    {
        lowHz = std::clamp (lowHz, 20.0f, 2000.0f);
        highHz = std::clamp (highHz, 200.0f, 20000.0f);
        if (lowHz >= highHz)
            highHz = std::min (20000.0f, lowHz + 50.0f);

        if (std::abs (lowHz - lowCutHz) < 1.0e-3f && std::abs (highHz - highCutHz) < 1.0e-3f)
            return;

        lowCutHz = lowHz;
        highCutHz = highHz;
        rebuildFilters (lowCutHz, highCutHz);
        reset();
    }

    float getLowCutHz() const noexcept { return lowCutHz; }
    float getHighCutHz() const noexcept { return highCutHz; }

    int getLatencySamples() const noexcept { return latency; }

    /**
     * Process planar channels.
     * midOut / outOfBandOut must have numChannels pointers with numSamples floats each.
     */
    void process (const float* const* input,
                  float* const* midOut,
                  float* const* outOfBandOut,
                  int numSamples)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* in = input[ch];
            float* mid = midOut[ch];
            float* side = outOfBandOut[ch];
            auto& hp = hpFilters[(size_t) ch];
            auto& lp = lpFilters[(size_t) ch];
            auto& delay = delays[(size_t) ch];

            for (int i = 0; i < numSamples; ++i)
            {
                const float x = in[i];
                const float delayed = delay.processSample (x);
                float band = hp.processSample (x);
                band = lp.processSample (band);
                mid[i] = band;
                side[i] = delayed - band;
            }
        }
    }

private:
    void rebuildFilters (float lowHz, float highHz)
    {
        const auto hpCoeffs = fir::designHighpass (kNumTaps, sampleRate, (double) lowHz);
        const auto lpCoeffs = fir::designLowpass (kNumTaps, sampleRate, (double) highHz);

        for (auto& f : hpFilters)
            f.setCoefficients (hpCoeffs);
        for (auto& f : lpFilters)
            f.setCoefficients (lpCoeffs);

        latency = 0;
        if (! hpFilters.empty())
            latency += hpFilters[0].getLatencySamples();
        if (! lpFilters.empty())
            latency += lpFilters[0].getLatencySamples();

        for (auto& d : delays)
            d.setDelay (latency);
    }

    double sampleRate = 48000.0;
    int numChannels = 2;
    float lowCutHz = 20.0f;
    float highCutHz = 20000.0f;
    int latency = 0;
    std::vector<fir::FirFilter> hpFilters;
    std::vector<fir::FirFilter> lpFilters;
    std::vector<fir::DelayLine> delays;
};
