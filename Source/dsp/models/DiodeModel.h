#pragma once

#include "DiodeCurve.h"
#include "SaturationModel.h"
#include "../../util/ParamIDs.h"
#include <vector>

/**
 * Diode family saturator: ideal-OA feedback diode clipper inside the OS island.
 *
 * Drive changes Rin (Rf/Rin gain into the diodes) — a component mod, not a
 * separate pre-gain. Makeup keeps −18 dBFS roughly level-stable. Flavors
 * select Shockley device params. Anti-aliasing: 4× OS (no ADAA).
 */
class DiodeModel final : public SaturationModel
{
public:
    const char* getId() const override { return ModelIds::diode; }
    const char* getDisplayName() const override { return "Diode"; }

    void prepare (double sampleRate, int /*maxBlock*/, int numChannels) override
    {
        const int ch = numChannels > 0 ? numChannels : 1;
        fbState.assign ((size_t) ch, {});
        osSampleRate = sampleRate > 0.0 ? sampleRate : 192000.0;
        setup = diode::setupForFlavor (diodeFlavor);
        setup.clipper.prepare (osSampleRate);
        updateDriveComponents();
    }

    void reset() override
    {
        for (auto& s : fbState)
            s = {};
    }

    void setDrive (float drive01) override
    {
        drive = drive01;
        updateDriveComponents();
    }

    void setDiodeFlavor (int flavor) override
    {
        diodeFlavor = flavor;
        setup = diode::setupForFlavor (flavor);
        setup.clipper.prepare (osSampleRate);
        updateDriveComponents();
        reset();
    }

    int getDiodeFlavor() const noexcept { return diodeFlavor; }
    float getDrive() const noexcept { return drive; }

    void process (float* const* channels, int numChannels, int numSamples) override
    {
        if (channels == nullptr || numChannels <= 0 || numSamples <= 0)
            return;

        if ((int) fbState.size() < numChannels)
            fbState.resize ((size_t) numChannels, {});

        if (drive < 1.0e-6f)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                if (channels[ch] == nullptr)
                    continue;
                auto& st = fbState[(size_t) ch];
                st.vOut = -channels[ch][numSamples - 1];
                st.iCap = 0.0f;
            }
            return;
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = channels[ch];
            if (data == nullptr)
                continue;

            auto& st = fbState[(size_t) ch];
            for (int i = 0; i < numSamples; ++i)
            {
                // No pre-gain — Drive already set Rin on the clipper
                data[i] = setup.clipper.process (data[i], st) * makeupGain;
            }
        }
    }

private:
    void updateDriveComponents() noexcept
    {
        diode::applyDriveToClipper (setup.clipper, drive);
        makeupGain = diode::makeupGainLinear (drive, setup);
    }

    int diodeFlavor = DiodeFlavorIds::silicon;
    float drive = 0.5f;
    float makeupGain = 1.0f;
    double osSampleRate = 192000.0;
    diode::FlavorSetup setup {};
    std::vector<devices::FeedbackDiodeClipper::State> fbState;
};
