#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

/**
 * Soft symmetric tape-family clip (algebraic) + ADAA.
 * Odd-leaning, smooth ceiling — “soft saturate” not diode snap.
 */
namespace tape
{
struct Coeffs
{
    float a = 0.72f; // softer ceiling → clearer Drive action at −18
    float makeupBiasDb = 0.0f;
};

inline Coeffs defaultCoeffs() noexcept { return {}; }

inline float shape (float x, const Coeffs& c) noexcept
{
    return c.a * x / std::sqrt (c.a * c.a + x * x);
}

inline float antiderivative (float x, const Coeffs& c) noexcept
{
    return c.a * (std::sqrt (c.a * c.a + x * x) - c.a);
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
    constexpr float kCurve = 1.3f;
    return std::pow (10.0f, (kMaxDb * std::pow (drive01, kCurve)) / 20.0f);
}

inline float makeupGainLinear (float drive01, const Coeffs& c) noexcept
{
    drive01 = std::clamp (drive01, 0.0f, 1.0f);
    if (drive01 < 1.0e-6f)
        return std::pow (10.0f, c.makeupBiasDb / 20.0f);

    constexpr float kRefPeak = 0.177827941f;
    const float driven = kRefPeak * driveGainLinear (drive01);
    const float shaped = std::abs (shape (driven, c));
    const float ratio = shaped / std::max (kRefPeak, 1.0e-8f);
    const float comp = 1.0f / std::sqrt (std::max (ratio, 1.0e-3f));
    const float blend = 0.45f * std::pow (drive01, 0.85f);
    return (1.0f + blend * (comp - 1.0f)) * std::pow (10.0f, c.makeupBiasDb / 20.0f);
}
} // namespace tape
