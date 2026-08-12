#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

/**
 * C¹-smooth diode-family soft clip (algebraic) + first-order ADAA helpers.
 * JUCE-free — usable from offline verifies and the plugin DiodeModel.
 *
 * Transfer (per polarity threshold a):
 *   f(x) = a * x / sqrt(a² + x²)
 *   F(x) = a * (sqrt(a² + x²) - a)   // antiderivative with F(0)=0
 *
 * Asymmetry: aPos for x≥0, aNeg for x<0 (both → f'(0)=1).
 */
namespace diode
{
struct FlavorCoeffs
{
    float aPos = 0.85f; // positive-side threshold (higher = cleaner)
    float aNeg = 0.85f; // negative-side threshold
    float makeupBiasDb = 0.0f; // static flavor loudness trim
};

inline FlavorCoeffs coeffsForFlavor (int flavor) noexcept
{
    // Tuned characters — not component datasheets.
    switch (flavor)
    {
        case 1: // Germanium — earlier, softer
            return { 0.48f, 0.48f, 0.5f };
        case 2: // LED — more headroom, then firmer grab
            return { 1.25f, 1.25f, -0.5f };
        case 3: // Asymmetric — even harmonics
            return { 0.95f, 0.55f, 0.25f };
        case 0: // Silicon
        default:
            return { 0.85f, 0.85f, 0.0f };
    }
}

inline float thresholdFor (const FlavorCoeffs& c, float x) noexcept
{
    return x >= 0.0f ? c.aPos : c.aNeg;
}

/** Waveshape f(x). */
inline float shape (float x, const FlavorCoeffs& c) noexcept
{
    const float a = thresholdFor (c, x);
    return a * x / std::sqrt (a * a + x * x);
}

/** Antiderivative F(x) with F(0)=0 (continuous across polarity change). */
inline float antiderivative (float x, const FlavorCoeffs& c) noexcept
{
    const float a = thresholdFor (c, x);
    return a * (std::sqrt (a * a + x * x) - a);
}

/**
 * First-order ADAA: y = (F(x) - F(xPrev)) / (x - xPrev), else f(x).
 */
inline float adaa (float x, float xPrev, const FlavorCoeffs& c) noexcept
{
    const float dx = x - xPrev;
    if (std::abs (dx) > 1.0e-5f)
        return (antiderivative (x, c) - antiderivative (xPrev, c)) / dx;
    return shape (x, c);
}

/** Drive → linear gain. drive∈[0,1], k≈1.4 so 0.5 is gentle. */
inline float driveGainLinear (float drive01) noexcept
{
    drive01 = std::clamp (drive01, 0.0f, 1.0f);
    constexpr float kMaxDb = 24.0f;
    constexpr float kCurve = 1.4f;
    const float t = std::pow (drive01, kCurve);
    const float db = kMaxDb * t;
    return std::pow (10.0f, db / 20.0f);
}

/**
 * Makeup so −18 dBFS RMS sine stays roughly level-stable vs Drive.
 * Uses a cheap peak-proxy through the static curve (no iteration).
 */
inline float makeupGainLinear (float drive01, const FlavorCoeffs& c) noexcept
{
    drive01 = std::clamp (drive01, 0.0f, 1.0f);
    if (drive01 < 1.0e-6f)
        return std::pow (10.0f, c.makeupBiasDb / 20.0f);

    // −18 dBFS RMS sine peak
    constexpr float kRefPeak = 0.177827941f; // 10^(-18/20)*sqrt(2)
    const float g = driveGainLinear (drive01);
    const float driven = kRefPeak * g;
    const float shaped = shape (driven, c);
    const float ratio = shaped / std::max (kRefPeak, 1.0e-8f);
    const float comp = 1.0f / std::max (ratio, 1.0e-3f);
    // Blend: full compensation at high drive, light at low (keeps mild grit audible)
    const float blend = std::pow (drive01, 0.85f);
    const float makeup = 1.0f + blend * (comp - 1.0f);
    return makeup * std::pow (10.0f, c.makeupBiasDb / 20.0f);
}

/** One sample with driven-domain ADAA state (xPrevDriven = previous driven sample). */
inline float processSampleDriven (float x, float drive01, const FlavorCoeffs& c, float& xPrevDriven) noexcept
{
    if (drive01 < 1.0e-6f)
    {
        xPrevDriven = x;
        return x;
    }

    const float gIn = driveGainLinear (drive01);
    const float gOut = makeupGainLinear (drive01, c);
    const float xd = x * gIn;
    const float y = adaa (xd, xPrevDriven, c);
    xPrevDriven = xd;
    return y * gOut;
}
} // namespace diode
