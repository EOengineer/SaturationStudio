#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

/**
 * C¹ tube-family soft transfer + first-order ADAA helpers (JUCE-free).
 *
 * Sharpened asymmetric tanh:
 *   f(x) = (a/s) * tanh(s * x / a)
 *   f'(0) = 1, asymptote → ±a/s
 *   F(x) = (a/s)² * log(cosh(s * x / a))
 *
 * Strong polarity asymmetry → even harmonics; Drive map goes to 36 dB so
 * max Drive is denser than Diode while Drive~0.5 stays usable at −18.
 */
namespace tube
{
struct Coeffs
{
    float aPos = 0.90f;
    float aNeg = 0.24f;   // much lower → strong 2nd
    float sharpness = 1.35f;
    float makeupBiasDb = 0.0f;
};

inline Coeffs defaultCoeffs() noexcept
{
    return {};
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
    constexpr float kMaxDb = 36.0f;
    constexpr float kCurve = 1.55f;
    const float t = std::pow (drive01, kCurve);
    const float db = kMaxDb * t;
    return std::pow (10.0f, db / 20.0f);
}

inline float makeupGainLinear (float drive01, const Coeffs& c) noexcept
{
    drive01 = std::clamp (drive01, 0.0f, 1.0f);
    if (drive01 < 1.0e-6f)
        return std::pow (10.0f, c.makeupBiasDb / 20.0f);

    constexpr float kRefPeak = 0.177827941f;
    const float g = driveGainLinear (drive01);
    const float driven = kRefPeak * g;
    const float shaped = 0.5f * (std::abs (shape (driven, c)) + std::abs (shape (-driven, c)));
    const float ratio = shaped / std::max (kRefPeak, 1.0e-8f);
    const float comp = 1.0f / std::sqrt (std::max (ratio, 1.0e-3f));
    const float blend = 0.30f * std::pow (drive01, 0.9f);
    const float makeup = 1.0f + blend * (comp - 1.0f);
    return makeup * std::pow (10.0f, c.makeupBiasDb / 20.0f);
}
} // namespace tube
