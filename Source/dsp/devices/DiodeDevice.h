#pragma once

#include <algorithm>
#include <cmath>

/**
 * Shockley diode + bulk series Rs for Newton stamps.
 * Topology-agnostic part — no Drive, OS, or saturator concerns.
 *
 * Junction:
 *   Ij(Vj) = Is * (exp(Vj / (n Vt)) - 1)
 *   Gj(Vj) = (Is / (n Vt)) * exp(Vj / (n Vt))
 *
 * Terminal V = Vj + I * Rs. Equivalent stamp:
 *   G = Gj / (1 + Gj * Rs)   →  1/Rs at high forward current
 *
 * |Vj| is clamped to vMax before the exp. Extra forward voltage beyond
 * that drop sits on Rs (G → 1/Rs) so Newton is not stuck on a G=0 plateau.
 * Rs = 0 keeps the original Shockley + clamp path.
 *
 * Bulk Rs is a device property — not the shunt-clipper source R, not Drive→Rin.
 */
namespace devices
{
struct DiodeDevice
{
    float isat = 2.52e-9f;        // ~1N4148-ish
    float nVt  = 0.026f * 1.75f;  // n * thermal voltage
    float vMax = 0.9f;            // clamp |Vj| for exp stability
    float rs   = 1.0f;            // bulk series ohms
    int   rsIters = 8;

    /** Junction + Rs stamp: current and dI/dV at terminal voltage v. */
    void stamp (float v, float& i, float& g) const noexcept
    {
        const float nv = std::max (nVt, 1.0e-9f);
        const float invNv = 1.0f / nv;
        const float lim = std::max (vMax, 0.05f);

        if (rs < 1.0e-6f)
        {
            const float vc = std::clamp (v, -lim, lim);
            i = isat * (std::exp (vc * invNv) - 1.0f);
            g = (v > lim || v < -lim) ? 0.0f : (isat * invNv) * std::exp (v * invNv);
            return;
        }

        const float invRs = 1.0f / rs;
        const float iMax = isat * (std::exp (lim * invNv) - 1.0f);
        const float vFwdLim = lim + iMax * rs;
        if (v >= vFwdLim)
        {
            i = (v - lim) * invRs;
            g = invRs;
            return;
        }

        const float iMin = isat * (std::exp (-lim * invNv) - 1.0f);
        const float vRevLim = -lim + iMin * rs;
        if (v <= vRevLim)
        {
            i = iMin;
            g = 0.0f;
            return;
        }

        float vj = std::clamp (v, -lim, lim);
        const float stepLim = std::max (0.25f * lim, 0.1f);
        for (int n = 0; n < rsIters; ++n)
        {
            const float vc = std::clamp (vj, -lim, lim);
            const float id = isat * (std::exp (vc * invNv) - 1.0f);
            const float gd = (isat * invNv) * std::exp (vc * invNv);
            const float f  = vj + id * rs - v;
            const float df = 1.0f + gd * rs;
            if (std::abs (df) < 1.0e-20f)
                break;
            float dv = f / df;
            dv = std::clamp (dv, -stepLim, stepLim);
            vj -= dv;
            vj = std::clamp (vj, -lim, lim);
            if (std::abs (dv) < 1.0e-9f)
                break;
        }

        const float vc = std::clamp (vj, -lim, lim);
        i = isat * (std::exp (vc * invNv) - 1.0f);
        const float gj = (isat * invNv) * std::exp (vc * invNv);
        g = gj / (1.0f + gj * rs);
    }

    float current (float v) const noexcept
    {
        float i = 0.0f, g = 0.0f;
        stamp (v, i, g);
        return i;
    }

    float conductance (float v) const noexcept
    {
        float i = 0.0f, g = 0.0f;
        stamp (v, i, g);
        return g;
    }
};

inline DiodeDevice siliconSignal() noexcept
{
    return { 2.52e-9f, 0.026f * 1.75f, 0.9f, 1.0f };
}

inline DiodeDevice germanium() noexcept
{
    return { 2.0e-7f, 0.026f * 1.0f, 0.5f, 10.0f };
}

inline DiodeDevice ledRed() noexcept
{
    return { 1.0e-12f, 0.026f * 2.0f, 2.0f, 30.0f };
}
} // namespace devices
