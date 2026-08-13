#pragma once

#include "DiodeCurve.h"
#include "SaturationModel.h"
#include "../../util/ParamIDs.h"
#include <vector>

/**
 * Diode family saturator: Shockley antiparallel + shunt-C clipper in the OS island.
 *
 * Drive pushes into the knee (0 dB → ~34 dB with gentle curve); makeup keeps
 * −18 dBFS reference level roughly stable. Flavors select device params.
 * Relies on 4× OS for anti-aliasing (no ADAA on the Newton transfer).
 */
class DiodeModel final : public SaturationModel
{
public:
    const char* getId() const override { return ModelIds::diode; }
    const char* getDisplayName() const override { return "Diode"; }

    void prepare (double sampleRate, int /*maxBlock*/, int numChannels) override
    {
        const int ch = numChannels > 0 ? numChannels : 1;
        capState.assign ((size_t) ch, {});
        osSampleRate = sampleRate > 0.0 ? sampleRate : 192000.0;
        setup = diode::setupForFlavor (diodeFlavor);
        setup.clipper.prepare (osSampleRate);
        updateGains();
    }

    void reset() override
    {
        for (auto& s : capState)
            s = {};
    }

    void setDrive (float drive01) override
    {
        drive = drive01;
        updateGains();
    }

    void setDiodeFlavor (int flavor) override
    {
        diodeFlavor = flavor;
        setup = diode::setupForFlavor (flavor);
        setup.clipper.prepare (osSampleRate);
        updateGains();
        reset();
    }

    int getDiodeFlavor() const noexcept { return diodeFlavor; }
    float getDrive() const noexcept { return drive; }

    void process (float* const* channels, int numChannels, int numSamples) override
    {
        if (channels == nullptr || numChannels <= 0 || numSamples <= 0)
            return;

        if ((int) capState.size() < numChannels)
            capState.resize ((size_t) numChannels, {});

        if (drive < 1.0e-6f)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                if (channels[ch] == nullptr)
                    continue;
                // Keep C state coherent with bypassed audio
                auto& st = capState[(size_t) ch];
                st.v = channels[ch][numSamples - 1];
                st.iCap = 0.0f;
            }
            return;
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = channels[ch];
            if (data == nullptr)
                continue;

            auto& st = capState[(size_t) ch];
            for (int i = 0; i < numSamples; ++i)
            {
                const float xd = data[i] * driveGain;
                const float y = setup.clipper.process (xd, st);
                data[i] = y * makeupGain;
            }
        }
    }

private:
    void updateGains() noexcept
    {
        driveGain = diode::driveGainLinear (drive);
        makeupGain = diode::makeupGainLinear (drive, setup);
    }

    int diodeFlavor = DiodeFlavorIds::silicon;
    float drive = 0.5f;
    float driveGain = 1.0f;
    float makeupGain = 1.0f;
    double osSampleRate = 192000.0;
    diode::FlavorSetup setup {};
    std::vector<devices::AntiParallelRcClipper::State> capState;
};
