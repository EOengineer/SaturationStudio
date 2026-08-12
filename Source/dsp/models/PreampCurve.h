#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

/**
 * Console preamp-family transfers + ADAA.
 * Flavors are tuned characters (not circuit recreations):
 *   Neve 1073 — rounder, even-leaning
 *   API 512   — punchier, harder knee
 */
namespace preamp
{
struct Coeffs
{
    float aPos = 0.85f;
    float aNeg = 0.55f;
    float sharpness = 1.0f; // 1 = plain tanh; >1 denser
    float makeupBiasDb = 0.0f;
};

inline Coeffs coeffsForFlavor (int flavor) noexcept
{
    switch (flavor)
    {
        case 1: // API 512 — punchier / harder
            return { 0.62f, 0.50f, 1.55f, -0.25f };
        case 0: // Neve 1073 — rounder / even
        default:
            return { 0.92f, 0.40f, 1.20f, 0.15f };
    }
}

inline float thresholdFor (const Coeffs& c, float x) noexcept
{
    return x >= 0.0f ? c.aPos : c.aNeg;
}

inline float shape (float x, const Coeffs& c) noexcept
{
    const float a = thresholdFor (c, x);
    const float s = std::max (1.0f, c.sharpness);
    return (a / s) * std::tanh (s * x / a);
}

inline float antiderivative (float x, const Coeffs& c) noexcept
{
    const float a = thresholdFor (c, x);
    const float s = std::max (1.0f, c.sharpness);
    const float u = s * x / a;
    const float au = std::abs (u);
    float logCosh;
    if (au < 10.0f)
        logCosh = std::log (std::cosh (u));
    else
        logCosh = au + std::log1p (std::exp (-2.0f * au)) - 0.69314718056f;
    const float as = a / s;
    return as * as * logCosh;
}

inline float adaa (float x, float xPrev, const Coeffs& c) noexcept
{
    const float dx = x - xPrev;
    if (std::abs (dx) > 1.0e-5f)
        return (antiderivative (x, c) - antiderivative (xPrev, c)) / dx;
    return shape (x, c);
}

inline float driveGainLinear (float drive01) noexcept
{
    drive01 = std::clamp (drive01, 0.0f, 1.0f);
    constexpr float kMaxDb = 34.0f;
    constexpr float kCurve = 1.35f;
    return std::pow (10.0f, (kMaxDb * std::pow (drive01, kCurve)) / 20.0f);
}

inline float makeupGainLinear (float drive01, const Coeffs& c) noexcept
{
    drive01 = std::clamp (drive01, 0.0f, 1.0f);
    if (drive01 < 1.0e-6f)
        return std::pow (10.0f, c.makeupBiasDb / 20.0f);

    constexpr float kRefPeak = 0.177827941f;
    const float driven = kRefPeak * driveGainLinear (drive01);
    const float shaped = 0.5f * (std::abs (shape (driven, c)) + std::abs (shape (-driven, c)));
    const float ratio = shaped / std::max (kRefPeak, 1.0e-8f);
    const float comp = 1.0f / std::sqrt (std::max (ratio, 1.0e-3f));
    const float blend = 0.40f * std::pow (drive01, 0.85f);
    return (1.0f + blend * (comp - 1.0f)) * std::pow (10.0f, c.makeupBiasDb / 20.0f);
}
} // namespace preamp
