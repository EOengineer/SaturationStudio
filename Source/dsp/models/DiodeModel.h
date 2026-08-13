#pragma once

#include "DiodeCurve.h"
#include "SaturationModel.h"
#include "../../util/ParamIDs.h"
#include <vector>

/**
 * Diode family saturator: C¹ soft-clip + first-order ADAA inside the OS island.
 *
 * Drive pushes into the knee (0 dB → ~34 dB with gentle curve); makeup keeps
 * −18 dBFS reference level roughly stable. Flavors are tuned characters.
 */
class DiodeModel final : public SaturationModel
{
public:
    const char* getId() const override { return ModelIds::diode; }
    const char* getDisplayName() const override { return "Diode"; }

    void prepare (double /*sampleRate*/, int /*maxBlock*/, int numChannels) override
    {
        const int ch = numChannels > 0 ? numChannels : 1;
        prevDriven.assign ((size_t) ch, 0.0f);
        coeffs = diode::coeffsForFlavor (diodeFlavor);
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

    void setDiodeFlavor (int flavor) override
    {
        diodeFlavor = flavor;
        coeffs = diode::coeffsForFlavor (flavor);
        updateGains();
    }

    int getDiodeFlavor() const noexcept { return diodeFlavor; }
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
                const float y = diode::adaa (xd, prev, coeffs);
                prev = xd;
                data[i] = y * makeupGain;
            }
            prevDriven[(size_t) ch] = prev;
        }
    }

private:
    void updateGains() noexcept
    {
        driveGain = diode::driveGainLinear (drive);
        makeupGain = diode::makeupGainLinear (drive, coeffs);
    }

    int diodeFlavor = DiodeFlavorIds::silicon;
    float drive = 0.5f;
    float driveGain = 1.0f;
    float makeupGain = 1.0f;
    diode::FlavorCoeffs coeffs {};
    std::vector<float> prevDriven;
};
