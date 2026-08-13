#pragma once

#include "../devices/AntiParallelClipper.h"
#include "../devices/DiodeDevice.h"
#include "../../util/ParamIDs.h"
#include <algorithm>
#include <cmath>

/**
 * Diode-family saturator helpers: flavor → clipper devices, Drive/makeup maps.
 * Nonlinear transfer is the static antiparallel clipper (Shockley + Rs), not
 * an algebraic soft-clip. Anti-aliasing relies on the engine 4× OS island
 * (no analytic ADAA for the Newton transfer).
 */
namespace diode
{
struct FlavorSetup
{
    devices::AntiParallelClipper clipper;
    float makeupBiasDb = 0.0f;
};

inline FlavorSetup setupForFlavor (int flavor) noexcept
{
    FlavorSetup s;
    s.clipper.rs = 1000.0f;

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
            // Neg side conducts earlier (Ge) → even harmonics
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

/** Clipper transfer (cold Newton). */
inline float shape (float x, const devices::AntiParallelClipper& clip) noexcept
{
    return clip.solve (x);
}

/** Drive → linear gain. Matches hotter Tape/Transformer family (~34 dB). */
inline float driveGainLinear (float drive01) noexcept
{
    drive01 = std::clamp (drive01, 0.0f, 1.0f);
    constexpr float kMaxDb = 34.0f;
    constexpr float kCurve = 1.3f;
    const float t = std::pow (drive01, kCurve);
    const float db = kMaxDb * t;
    return std::pow (10.0f, db / 20.0f);
}

/**
 * Makeup so −18 dBFS RMS sine stays roughly level-stable vs Drive.
 * Partial compensation so Drive still feels denser, not gain-matched dead.
 */
inline float makeupGainLinear (float drive01, const FlavorSetup& setup) noexcept
{
    drive01 = std::clamp (drive01, 0.0f, 1.0f);
    if (drive01 < 1.0e-6f)
        return std::pow (10.0f, setup.makeupBiasDb / 20.0f);

    constexpr float kRefPeak = 0.177827941f; // 10^(-18/20)*sqrt(2)
    const float g = driveGainLinear (drive01);
    const float driven = kRefPeak * g;
    const float shaped = 0.5f * (std::abs (setup.clipper.solve (driven))
                                 + std::abs (setup.clipper.solve (-driven)));
    const float ratio = shaped / std::max (kRefPeak, 1.0e-8f);
    const float comp = 1.0f / std::sqrt (std::max (ratio, 1.0e-3f));
    const float blend = 0.40f * std::pow (drive01, 0.85f);
    const float makeup = 1.0f + blend * (comp - 1.0f);
    return makeup * std::pow (10.0f, setup.makeupBiasDb / 20.0f);
}
} // namespace diode
