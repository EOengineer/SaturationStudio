#pragma once

#include "TransformerCurve.h"
#include "SaturationModel.h"
#include "../../util/ParamIDs.h"
#include <algorithm>
#include <vector>

/** Transformer family: asymmetric algebraic clip + ADAA (no hysteresis state yet). */
class TransformerModel final : public SaturationModel
{
public:
    const char* getId() const override { return ModelIds::transformer; }
    const char* getDisplayName() const override { return "Transformer"; }

    void prepare (double, int, int numChannels) override
    {
        prevDriven.assign ((size_t) std::max (1, numChannels), 0.0f);
        coeffs = transformer::defaultCoeffs();
        updateGains();
    }

    void reset() override { std::fill (prevDriven.begin(), prevDriven.end(), 0.0f); }

    void setDrive (float drive01) override
    {
        drive = drive01;
        updateGains();
    }

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
                data[i] = transformer::adaa (xd, prev, coeffs) * makeupGain;
                prev = xd;
            }
            prevDriven[(size_t) ch] = prev;
        }
    }

private:
    void updateGains() noexcept
    {
        driveGain = transformer::driveGainLinear (drive);
        makeupGain = transformer::makeupGainLinear (drive, coeffs);
    }

    float drive = 0.5f, driveGain = 1.0f, makeupGain = 1.0f;
    transformer::Coeffs coeffs {};
    std::vector<float> prevDriven;
};
