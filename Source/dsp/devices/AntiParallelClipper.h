#pragma once

#include "DiodeDevice.h"
#include <algorithm>
#include <cmath>

/**
 * Static resistive clipper: Vin — Rs — Vout, antiparallel diodes Vout→gnd.
 *
 * KCL at the clip node V:
 *   (Vin - V) / Rs = I_pos(V) - I_neg(-V)
 *
 * Solved with 1-D Newton each sample. Memoryless network (no C).
 * Iterates are seeded into the diode voltage range so large |Vin|
 * (high Drive) cannot stall on the Shockley exp-clamp plateau.
 */
namespace devices
{
struct AntiParallelClipper
{
    DiodeDevice dPos = siliconSignal();
    DiodeDevice dNeg = siliconSignal();
    float rs = 1000.0f; // ohms
    int maxIters = 12;

    /** Cold solve (makeup / verifies). */
    float solve (float vin) const noexcept
    {
        const float lim = diodeLimit();
        float v = std::clamp (vin, -lim, lim);
        newton (vin, v, lim);
        return v;
    }

    /**
     * Per-sample solve. vPrev is an optional warm hint only when it already
     * lies in the active diode region; otherwise we re-seed from Vin.
     */
    float process (float vin, float& vPrev) const noexcept
    {
        const float lim = diodeLimit();
        float v = std::clamp (vin, -lim, lim);
        if (std::abs (vPrev) <= lim * 1.25f)
            v = vPrev;

        newton (vin, v, lim);
        vPrev = v;
        return v;
    }

private:
    float diodeLimit() const noexcept
    {
        return std::max (dPos.vMax, dNeg.vMax);
    }

    void newton (float vin, float& v, float lim) const noexcept
    {
        const float invRs = 1.0f / std::max (rs, 1.0e-3f);
        const float stepLim = std::max (0.25f * lim, 0.1f);

        for (int i = 0; i < maxIters; ++i)
        {
            // Stay in / near the exponential region so G stays useful
            if (std::abs (v) > lim * 1.25f)
                v = std::clamp (vin, -lim, lim);

            float ip = 0.0f, gp = 0.0f, in = 0.0f, gn = 0.0f;
            dPos.stamp (v, ip, gp);
            dNeg.stamp (-v, in, gn);
            const float id = ip - in;
            const float gd = gp + gn;
            const float f  = (vin - v) * invRs - id;
            const float df = -invRs - gd;

            // Plateau (G≈0): damping alone oscillates; re-seed into the knee
            if (gd < 1.0e-12f && std::abs (f) > 1.0e-6f)
            {
                v = std::clamp (vin, -lim, lim);
                continue;
            }

            if (std::abs (df) < 1.0e-20f)
                break;

            float dv = f / df;
            dv = std::clamp (dv, -stepLim, stepLim);
            v -= dv;

            if (std::abs (dv) < 1.0e-9f)
                break;
        }

        v = std::clamp (v, -lim * 2.0f, lim * 2.0f);
        if (! std::isfinite (v))
            v = 0.0f;
    }
};
} // namespace devices
