#pragma once

#include "../devices/TriodeStage.h"
#include "../devices/TubeDevice.h"
#include "SaturationModel.h"
#include "../../util/ParamIDs.h"
#include <algorithm>
#include <cmath>
#include <vector>

/**
 * Tube family saturator: live common-cathode 3×3 Newton TriodeStage stamping TubeDevice.
 * Flavors select Koren factories (12AX7 / 5751 / 12AU7). Drive → Vdrive inside the stage.
 * Waveshape TubeCurve is parked (not used here). Same Drive=0 identity + −18 makeup contract.
 */
class TubeModel final : public SaturationModel
{
public:
    const char* getId() const override { return ModelIds::tube; }
    const char* getDisplayName() const override { return "Tube"; }

    void prepare (double sampleRate, int /*maxBlock*/, int numChannels) override
    {
        osSampleRate = sampleRate > 0.0 ? sampleRate : 192000.0;
        const int ch = numChannels > 0 ? numChannels : 1;
        stages.resize ((size_t) ch);
        for (auto& s : stages)
        {
            s.prepare ((float) osSampleRate, deviceForFlavor (tubeFlavor));
            s.setDrive (drive);
        }
        updateMakeup();
    }

    void reset() override
    {
        for (auto& s : stages)
            s.reset();
    }

    void setDrive (float drive01) override
    {
        drive = drive01;
        for (auto& s : stages)
            s.setDrive (drive);
        updateMakeup();
    }

    void setTubeFlavor (int flavor) override
    {
        tubeFlavor = flavor;
        const auto dev = deviceForFlavor (flavor);
        for (auto& s : stages)
        {
            s.setTube (dev);
            s.setDrive (drive);
        }
        updateMakeup();
    }

    int getTubeFlavor() const noexcept { return tubeFlavor; }
    float getDrive() const noexcept { return drive; }

    void process (float* const* channels, int numChannels, int numSamples) override
    {
        if (channels == nullptr || numChannels <= 0 || numSamples <= 0)
            return;

        if ((int) stages.size() < numChannels)
        {
            const size_t old = stages.size();
            stages.resize ((size_t) numChannels);
            for (size_t i = old; i < stages.size(); ++i)
            {
                stages[i].prepare ((float) osSampleRate, deviceForFlavor (tubeFlavor));
                stages[i].setDrive (drive);
            }
        }

        if (drive < 1.0e-6f)
            return; // identity bypass

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = channels[ch];
            if (data == nullptr)
                continue;

            auto& stage = stages[(size_t) ch];
            for (int i = 0; i < numSamples; ++i)
                data[i] = stage.processSample (data[i]) * makeupGain;
        }
    }

private:
    static devices::TubeDevice deviceForFlavor (int flavor) noexcept
    {
        switch (flavor)
        {
            case TubeFlavorIds::type5751: return devices::type5751();
            case TubeFlavorIds::au7:      return devices::twelveAu7();
            case TubeFlavorIds::ax7:
            default:                      return devices::twelveAx7();
        }
    }

    void updateMakeup() noexcept
    {
        // Stabilize −18 loudness as Drive digs into clip (stage owns saturation depth).
        // Fixed +6.5 dB output nudge so Tube sits closer to Diode loudness at −18.
        constexpr float kOutputNudge = 2.113489f; // +6.5 dB
        const float d = std::clamp (drive, 0.0f, 1.0f);
        if (d < 1.0e-6f)
        {
            makeupGain = 1.0f;
            return;
        }
        // Match TriodeStage grid map (kMaxGridGain=86.4, kDriveCurve=2.55).
        const float gridProxy = std::max (1.0e-3f, std::pow (d, 2.55f) * 86.4f);
        const float comp = 1.0f / std::sqrt (std::max (gridProxy / 7.0f, 1.0e-3f));
        const float blend = 0.50f * std::pow (d, 0.75f);
        makeupGain = (1.0f + blend * (comp - 1.0f)) * kOutputNudge;
        makeupGain = std::clamp (makeupGain, 0.35f, 3.5f);
    }

    int tubeFlavor = TubeFlavorIds::ax7;
    float drive = 0.5f;
    float makeupGain = 1.0f;
    double osSampleRate = 192000.0;
    std::vector<devices::TriodeStage> stages;
};
