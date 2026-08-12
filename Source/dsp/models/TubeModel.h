#pragma once

#include "TubeCurve.h"
#include "SaturationModel.h"
#include "../../util/ParamIDs.h"
#include <vector>

/**
 * Tube family saturator: asymmetric tanh transfer + first-order ADAA.
 * Same Drive / −18 makeup contract as Diode; even-heavy vs Silicon diode.
 */
class TubeModel final : public SaturationModel
{
public:
    const char* getId() const override { return ModelIds::tube; }
    const char* getDisplayName() const override { return "Tube"; }

    void prepare (double /*sampleRate*/, int /*maxBlock*/, int numChannels) override
    {
        const int ch = numChannels > 0 ? numChannels : 1;
        prevDriven.assign ((size_t) ch, 0.0f);
        coeffs = tube::defaultCoeffs();
        updateGains();
    }

    void reset() override
    {
        std::fill (prevDriven.begin(), prevDriven.end(), 0.0f);
    }

    void setDrive (float drive01) override
    {
        drive = drive01;
        updateGains();
    }

    float getDrive() const noexcept { return drive; }

    void process (float* const* channels, int numChannels, int numSamples) override
    {
        if (channels == nullptr || numChannels <= 0 || numSamples <= 0)
            return;

        if ((int) prevDriven.size() < numChannels)
            prevDriven.resize ((size_t) numChannels, 0.0f);

        if (drive < 1.0e-6f)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                if (channels[ch] == nullptr)
                    continue;
                prevDriven[(size_t) ch] = channels[ch][numSamples - 1];
            }
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
                const float x = data[i];
                const float xd = x * driveGain;
                const float y = tube::adaa (xd, prev, coeffs);
                prev = xd;
                data[i] = y * makeupGain;
            }
            prevDriven[(size_t) ch] = prev;
        }
    }

private:
    void updateGains() noexcept
    {
        driveGain = tube::driveGainLinear (drive);
        makeupGain = tube::makeupGainLinear (drive, coeffs);
    }

    float drive = 0.5f;
    float driveGain = 1.0f;
    float makeupGain = 1.0f;
    tube::Coeffs coeffs {};
    std::vector<float> prevDriven;
};
