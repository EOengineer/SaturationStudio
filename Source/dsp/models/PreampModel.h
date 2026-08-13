#pragma once

#include "PreampCurve.h"
#include "SaturationModel.h"
#include "../../util/ParamIDs.h"
#include <algorithm>
#include <vector>

/** Preamp family: Neve 1073 / API 512 character waveshapers + ADAA. */
class PreampModel final : public SaturationModel
{
public:
    const char* getId() const override { return ModelIds::preamp; }
    const char* getDisplayName() const override { return "Preamp"; }

    void prepare (double, int, int numChannels) override
    {
        prevDriven.assign ((size_t) std::max (1, numChannels), 0.0f);
        coeffs = preamp::coeffsForFlavor (preampFlavor);
        updateGains();
    }

    void reset() override { std::fill (prevDriven.begin(), prevDriven.end(), 0.0f); }

    void setDrive (float drive01) override
    {
        drive = drive01;
        updateGains();
    }

    void setPreampFlavor (int flavor) override
    {
        preampFlavor = flavor;
        coeffs = preamp::coeffsForFlavor (flavor);
        updateGains();
    }

    int getPreampFlavor() const noexcept { return preampFlavor; }

    void process (float* const* channels, int numChannels, int numSamples) override
    {
        if (channels == nullptr || numChannels <= 0 || numSamples <= 0)
            return;
        if ((int) prevDriven.size() < numChannels)
            prevDriven.resize ((size_t) numChannels, 0.0f);

        if (drive < 1.0e-6f)
        {
            for (int ch = 0; ch < numChannels; ++ch)
                if (channels[ch] != nullptr)
                    prevDriven[(size_t) ch] = channels[ch][numSamples - 1];
            return;
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = channels[ch];
            if (data == nullptr)
                continue;
            float prev = prevDriven[(size_t) ch];
            for (int i = 0; i < numSamples; ++i)
            {
                const float xd = data[i] * driveGain;
                data[i] = preamp::adaa (xd, prev, coeffs) * makeupGain;
                prev = xd;
            }
            prevDriven[(size_t) ch] = prev;
        }
    }

private:
    void updateGains() noexcept
    {
        driveGain = preamp::driveGainLinear (drive);
        makeupGain = preamp::makeupGainLinear (drive, coeffs);
    }

    int preampFlavor = PreampFlavorIds::neve1073;
    float drive = 0.5f, driveGain = 1.0f, makeupGain = 1.0f;
    preamp::Coeffs coeffs {};
    std::vector<float> prevDriven;
};
