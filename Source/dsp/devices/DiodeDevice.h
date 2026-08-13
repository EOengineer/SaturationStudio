#pragma once

#include <algorithm>
#include <cmath>

/**
 * Shockley diode I–V + small-signal conductance for Newton stamps.
 * Topology-agnostic part — no Drive, OS, or saturator concerns.
 *
 *   I(V) = Is * (exp(V / (n Vt)) - 1)
 *   G(V) = dI/dV = (Is / (n Vt)) * exp(V / (n Vt))
 *
 * |V| is clamped to vMax before the exp for numerical stability.
 */
namespace devices
{
struct DiodeDevice
{
    float isat = 2.52e-9f;        // ~1N4148-ish
    float nVt  = 0.026f * 1.75f;  // n * thermal voltage
    float vMax = 0.9f;            // clamp |V| for exp stability

    float current (float v) const noexcept
    {
        const float vc = std::clamp (v, -vMax, vMax);
        return isat * (std::exp (vc / nVt) - 1.0f);
    }

    float conductance (float v) const noexcept
    {
        // Flat outside the clamp: dI/dV = 0 so Newton can leave the plateau.
        if (v > vMax || v < -vMax)
            return 0.0f;
        return (isat / nVt) * std::exp (v / nVt);
    }
};

inline DiodeDevice siliconSignal() noexcept
{
    return { 2.52e-9f, 0.026f * 1.75f, 0.9f };
}

inline DiodeDevice germanium() noexcept
{
    return { 2.0e-7f, 0.026f * 1.0f, 0.5f };
}

inline DiodeDevice ledRed() noexcept
{
    return { 1.0e-12f, 0.026f * 2.0f, 2.0f };
}
} // namespace devices
