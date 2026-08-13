#pragma once

#include "../devices/FeedbackDiodeClipper.h"
#include "../devices/DiodeDevice.h"
#include "../../util/ParamIDs.h"
#include <algorithm>
#include <cmath>

/**
 * Diode-family saturator helpers: flavor → feedback clipper, Drive/makeup.
 * Nonlinear transfer is ideal-OA inverting soft-clip (Shockley FB diodes + Cf).
 * Anti-aliasing relies on the engine 4× OS island (no analytic ADAA).
 */
namespace diode
{
struct FlavorSetup
{
    devices::FeedbackDiodeClipper clipper;
    float makeupBiasDb = 0.0f;
};

inline FlavorSetup setupForFlavor (int flavor) noexcept
{
    FlavorSetup s;
    s.clipper.rin = 10000.0f;
    s.clipper.rf  = 10000.0f;
    s.clipper.cf  = 100.0e-12f;

    switch (flavor)
    {
        case DiodeFlavorIds::germanium:
            s.clipper.dPos = devices::germanium();
            s.clipper.dNeg = devices::germanium();
            s.makeupBiasDb = 0.5f;
            break;
        case DiodeFlavorIds::led:
            s.clipper.dPos = devices::ledRed();
            s.clipper.dNeg = devices::ledRed();
            s.makeupBiasDb = -0.35f;
            break;
        case DiodeFlavorIds::asymmetric:
            s.clipper.dPos = devices::siliconSignal();
            s.clipper.dNeg = devices::germanium();
            s.makeupBiasDb = 0.25f;
            break;
        case DiodeFlavorIds::silicon:
        default:
            s.clipper.dPos = devices::siliconSignal();
            s.clipper.dNeg = devices::siliconSignal();
            s.makeupBiasDb = 0.0f;
            break;
    }

    return s;
}

/** DC clipper transfer (Cf open), audio polarity. */
inline float shape (float x, const devices::FeedbackDiodeClipper& clip) noexcept
{
    return clip.solveDc (x);
}

inline float driveGainLinear (float drive01) noexcept
{
    drive01 = std::clamp (drive01, 0.0f, 1.0f);
    constexpr float kMaxDb = 34.0f;
    constexpr float kCurve = 1.3f;
    const float t = std::pow (drive01, kCurve);
    const float db = kMaxDb * t;
    return std::pow (10.0f, db / 20.0f);
}

inline float makeupGainLinear (float drive01, const FlavorSetup& setup) noexcept
{
    drive01 = std::clamp (drive01, 0.0f, 1.0f);
    if (drive01 < 1.0e-6f)
        return std::pow (10.0f, setup.makeupBiasDb / 20.0f);

    constexpr float kRefPeak = 0.177827941f;
    const float g = driveGainLinear (drive01);
    const float driven = kRefPeak * g;
    const float shaped = 0.5f * (std::abs (setup.clipper.solveDc (driven))
                                 + std::abs (setup.clipper.solveDc (-driven)));
    const float ratio = shaped / std::max (kRefPeak, 1.0e-8f);
    const float comp = 1.0f / std::sqrt (std::max (ratio, 1.0e-3f));
    const float blend = 0.40f * std::pow (drive01, 0.85f);
    const float makeup = 1.0f + blend * (comp - 1.0f);
    return makeup * std::pow (10.0f, setup.makeupBiasDb / 20.0f);
}
} // namespace diode
