#pragma once

#include "DiodeDevice.h"
#include <algorithm>
#include <cmath>

/**
 * Dynamic antiparallel clipper: Vin — Rs — V, diodes + shunt C to gnd.
 *
 * KCL at the clip node V:
 *   (Vin - V) / Rs = I_pos(V) - I_neg(-V) + I_c
 *
 * Capacitor uses a trapezoidal companion (sample period T = 1/fs):
 *   I_c = geq * V + ieq,  geq = 2C/T,
 *   ieq = -geq * V_prev - I_c_prev
 *
 * Still one nonlinear node → 1-D Newton; State carries C memory.
 * DiodeDevice (Shockley + bulk Rs) is unchanged here — no junction C / thermal.
 */
namespace devices
{
struct AntiParallelRcClipper
{
    struct State
    {
        float v = 0.0f;
        float iCap = 0.0f;
    };

    DiodeDevice dPos = siliconSignal();
    DiodeDevice dNeg = siliconSignal();
    float rs = 1000.0f;           // ohms
    float c = 4.7e-9f;            // 4.7 nF shunt — softens edges, keeps grit at −18
    float sampleRate = 192000.0f; // oversampled rate inside OS island
    int maxIters = 12;

    void prepare (double sr) noexcept
    {
        sampleRate = (float) std::max (sr, 1.0);
    }

    /** DC / makeup: C open → same as static resistive solve. */
    float solveDc (float vin) const noexcept
    {
        const float lim = diodeLimit();
        float v = std::clamp (vin, -lim, lim);
        newtonResistive (vin, v, lim);
        return v;
    }

    /** One sample at prepared sampleRate; updates capacitor state. */
    float process (float vin, State& state) const noexcept
    {
        const float lim = diodeLimit();
        const float T = 1.0f / sampleRate;
        const float geq = 2.0f * std::max (c, 1.0e-15f) / T;
        const float ieq = -geq * state.v - state.iCap;

        float v = std::clamp (vin, -lim, lim);
        if (std::abs (state.v) <= lim * 1.25f)
            v = state.v;

        newtonDynamic (vin, v, lim, geq, ieq);

        const float iCap = geq * v + ieq;
        state.v = v;
        state.iCap = std::isfinite (iCap) ? iCap : 0.0f;
        return v;
    }

private:
    float diodeLimit() const noexcept
    {
        return std::max (dPos.vMax, dNeg.vMax);
    }

    void newtonResistive (float vin, float& v, float lim) const noexcept
    {
        newton (vin, v, lim, 0.0f, 0.0f);
    }

    void newtonDynamic (float vin, float& v, float lim, float geq, float ieq) const noexcept
    {
        newton (vin, v, lim, geq, ieq);
    }

    void newton (float vin, float& v, float lim, float geq, float ieq) const noexcept
    {
        const float invRs = 1.0f / std::max (rs, 1.0e-3f);
        const float stepLim = std::max (0.25f * lim, 0.1f);

        for (int i = 0; i < maxIters; ++i)
        {
            if (std::abs (v) > lim * 1.25f)
                v = std::clamp (vin, -lim, lim);

            float ip = 0.0f, gp = 0.0f, in = 0.0f, gn = 0.0f;
            dPos.stamp (v, ip, gp);
            dNeg.stamp (-v, in, gn);
            const float id = ip - in;
            const float gd = gp + gn;
            // f = (Vin-V)/Rs - Id - geq*V - ieq
            const float f  = (vin - v) * invRs - id - geq * v - ieq;
            const float df = -invRs - gd - geq;

            if (gd < 1.0e-12f && geq < 1.0e-12f && std::abs (f) > 1.0e-6f)
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
