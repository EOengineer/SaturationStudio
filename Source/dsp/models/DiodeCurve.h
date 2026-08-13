#pragma once

#include "../devices/FeedbackDiodeClipper.h"
#include "../devices/DiodeDevice.h"
#include "../../util/ParamIDs.h"
#include <algorithm>
#include <cmath>

/**
 * Diode-family saturator helpers: flavor → feedback clipper, Drive/makeup.
 *
 * Drive is a component change: Rin = Rf / G(drive) so closed-loop gain
 * Rf/Rin rises with Drive. G follows the same dB curve as the old pre-gain
 * map (~34 dB max, drive^1.3) so mid-knob stays usable at −18.
 * Anti-aliasing relies on the engine 4× OS island (no analytic ADAA).
 */
namespace diode
{
struct FlavorSetup
{
    devices::FeedbackDiodeClipper clipper;
    float makeupBiasDb = 0.0f;
};

inline constexpr float kRfOhms = 10000.0f;
inline constexpr float kCfFarads = 100.0e-12f;
inline constexpr float kMaxDriveDb = 34.0f;
inline constexpr float kDriveCurve = 1.3f;

inline FlavorSetup setupForFlavor (int flavor) noexcept
{
    FlavorSetup s;
    s.clipper.rf = kRfOhms;
    s.clipper.cf = kCfFarads;
    s.clipper.rin = kRfOhms; // unity until Drive applied

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

/** Closed-loop |gain| Rf/Rin from Drive (1 → ~50 ≈ 34 dB). */
inline float closedLoopGain (float drive01) noexcept
{
    drive01 = std::clamp (drive01, 0.0f, 1.0f);
    const float t = std::pow (drive01, kDriveCurve);
    const float db = kMaxDriveDb * t;
    return std::pow (10.0f, db / 20.0f);
}

/** Rin for Drive — smaller Rin → more gain into the diodes. */
inline float rinForDrive (float drive01, float rf = kRfOhms) noexcept
{
    return rf / std::max (closedLoopGain (drive01), 1.0e-3f);
}

inline void applyDriveToClipper (devices::FeedbackDiodeClipper& clip, float drive01) noexcept
{
    clip.rin = rinForDrive (drive01, clip.rf);
}

/** DC clipper transfer (Cf open), audio polarity. */
inline float shape (float x, const devices::FeedbackDiodeClipper& clip) noexcept
{
    return clip.solveDc (x);
}

/**
 * Makeup so −18 dBFS RMS sine stays roughly level-stable vs Drive.
 * Uses DC solve with Rin set for this Drive (no external pre-gain).
 */
inline float makeupGainLinear (float drive01, FlavorSetup setup) noexcept
{
    drive01 = std::clamp (drive01, 0.0f, 1.0f);
    if (drive01 < 1.0e-6f)
        return std::pow (10.0f, setup.makeupBiasDb / 20.0f);

    applyDriveToClipper (setup.clipper, drive01);

    constexpr float kRefPeak = 0.177827941f;
    const float shaped = 0.5f * (std::abs (setup.clipper.solveDc (kRefPeak))
                                 + std::abs (setup.clipper.solveDc (-kRefPeak)));
    const float ratio = shaped / std::max (kRefPeak, 1.0e-8f);
    const float comp = 1.0f / std::sqrt (std::max (ratio, 1.0e-3f));
    const float blend = 0.40f * std::pow (drive01, 0.85f);
    const float makeup = 1.0f + blend * (comp - 1.0f);
    return makeup * std::pow (10.0f, setup.makeupBiasDb / 20.0f);
}
} // namespace diode
