#pragma once

#include <algorithm>
#include <cmath>

/**
 * Koren-style triode plate law for Newton stamps.
 * Topology-agnostic part — no Drive, OS, or saturator concerns.
 *
 *   Ip = (E1^EX) / KG1
 *   E1 from KP / MU / KVB soft knee (Ayumi / SPICE-family form)
 *
 * Conductances via central differences (island size is tiny).
 * See docs/devices/overview.md.
 */
namespace devices
{
struct TubeDevice
{
    float mu  = 100.0f;
    float ex  = 1.4f;
    float kg1 = 1060.0f;
    float kp  = 600.0f;
    float kvb = 300.0f;

    /** Plate current (A) for grid-cathode Vgk and plate-cathode Vak. */
    float plateCurrent (float vgk, float vak) const noexcept
    {
        const float va = std::max (vak, 0.0f);
        const float sqrtTerm = std::sqrt (std::max (kvb, 0.0f) + va * va);
        const float expArg = kp * (1.0f / std::max (mu, 1.0f) + vgk / std::max (sqrtTerm, 1.0e-6f));
        const float clamped = std::clamp (expArg, -80.0f, 80.0f);
        float e1 = (va / std::max (kp, 1.0f)) * std::log1p (std::exp (clamped));
        if (e1 < 0.0f)
            e1 = 0.0f;
        const float ip = std::pow (e1, ex) / std::max (kg1, 1.0f);
        return std::isfinite (ip) ? ip : 0.0f;
    }

    /** dIp/dVgk and dIp/dVak (A/V). Optionally returns Ip at the probe. */
    void plateConductances (float vgk, float vak, float& gGrid, float& gPlate,
                            float* ipOut = nullptr) const noexcept
    {
        constexpr float h = 1.0e-3f;
        const float i0 = plateCurrent (vgk, vak);
        if (ipOut != nullptr)
            *ipOut = i0;
        gGrid = (plateCurrent (vgk + h, vak) - i0) / h;
        gPlate = (plateCurrent (vgk, vak + h) - i0) / h;
        gGrid = std::max (gGrid, 0.0f);
        gPlate = std::max (gPlate, 1.0e-12f);
    }
};

/** Stock 12AX7A-ish Koren set (µ ≈ 100). */
inline TubeDevice twelveAx7() noexcept
{
    TubeDevice t;
    t.mu = 100.0f;
    t.ex = 1.4f;
    t.kg1 = 1060.0f;
    t.kp = 600.0f;
    t.kvb = 300.0f;
    return t;
}

/**
 * 5751-ish starting set (µ ≈ 70).
 * Literature-scaled from the AX7 Koren base — refine when a stage consumer lands.
 */
inline TubeDevice type5751() noexcept
{
    TubeDevice t = twelveAx7();
    t.mu = 70.0f;
    t.kg1 = 1400.0f; // lower gain density vs AX7 at shared probes
    t.kp = 500.0f;
    return t;
}

/**
 * 12AU7-ish starting set (µ ≈ 20).
 * Literature-scaled from the AX7 Koren base — refine when a stage consumer lands.
 */
inline TubeDevice twelveAu7() noexcept
{
    TubeDevice t;
    t.mu = 20.0f;
    t.ex = 1.3f;
    t.kg1 = 1200.0f;
    t.kp = 180.0f;
    t.kvb = 300.0f;
    return t;
}
} // namespace devices
