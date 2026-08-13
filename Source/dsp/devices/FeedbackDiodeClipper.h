#pragma once

#include "DiodeDevice.h"
#include <algorithm>
#include <cmath>

/**
 * Ideal-op-amp inverting soft-clipper with diode (+ C) feedback.
 *
 *                    ┌──── D+ / D− ────┐
 *                    │    Rf ∥ Cf       │
 * Vin ── Rin ──●(−)──┤                 ├── Vout
 *              │     │   Ideal OA      │
 *             gnd    └─────────────────┘
 *              (+) = gnd  → virtual ground V− ≈ 0
 *
 * KCL at the inverting node (currents into the node):
 *   Vin/Rin + Vout/Rf + I_d(Vout) + I_Cf = 0
 * where I_d(V) = I_pos(V) - I_neg(-V) and Cf uses a trapezoidal companion
 * on voltage Vout (across Cf to virtual ground):
 *   I_Cf = geq * Vout + ieq,  geq = 2 Cf / T,
 *   ieq = -geq * Vout_prev - I_Cf_prev
 *
 * Unknown is Vout only → 1-D Newton. Audio out is −Vout so small-signal
 * phase matches the non-inverting shunt clippers (gain ≈ +Rf/Rin).
 *
 * Rf is required: diodes alone leave DC feedback open under an ideal OA.
 */
namespace devices
{
struct FeedbackDiodeClipper
{
    struct State
    {
        float vOut = 0.0f; // op-amp output (capacitor voltage)
        float iCap = 0.0f;
    };

    DiodeDevice dPos = siliconSignal();
    DiodeDevice dNeg = siliconSignal();
    float rin = 10000.0f;          // ohms
    float rf  = 10000.0f;          // ohms — unity small-signal |gain|
    float cf  = 100.0e-12f;        // 100 pF feedback — softens edges
    float sampleRate = 192000.0f;
    int maxIters = 12;

    void prepare (double sr) noexcept
    {
        sampleRate = (float) std::max (sr, 1.0);
    }

    /** DC (Cf open). Returns audio-polarity sample (−Vout). */
    float solveDc (float vin) const noexcept
    {
        const float lim = diodeLimit();
        float vOut = std::clamp (-vin * (rf / std::max (rin, 1.0e-3f)), -lim, lim);
        newton (vin, vOut, lim, 0.0f, 0.0f);
        return -vOut;
    }

    /** One sample; updates Cf state. Returns audio-polarity (−Vout). */
    float process (float vin, State& state) const noexcept
    {
        const float lim = diodeLimit();
        const float T = 1.0f / sampleRate;
        const float geq = 2.0f * std::max (cf, 1.0e-18f) / T;
        const float ieq = -geq * state.vOut - state.iCap;

        float vOut = std::clamp (-vin * (rf / std::max (rin, 1.0e-3f)), -lim, lim);
        if (std::abs (state.vOut) <= lim * 1.25f)
            vOut = state.vOut;

        newton (vin, vOut, lim, geq, ieq);

        const float iCap = geq * vOut + ieq;
        state.vOut = vOut;
        state.iCap = std::isfinite (iCap) ? iCap : 0.0f;
        return -vOut;
    }

private:
    float diodeLimit() const noexcept
    {
        return std::max (dPos.vMax, dNeg.vMax);
    }

    void newton (float vin, float& vOut, float lim, float geq, float ieq) const noexcept
    {
        const float invRin = 1.0f / std::max (rin, 1.0e-3f);
        const float invRf  = 1.0f / std::max (rf, 1.0e-3f);
        const float stepLim = std::max (0.25f * lim, 0.1f);

        for (int i = 0; i < maxIters; ++i)
        {
            if (std::abs (vOut) > lim * 1.25f)
                vOut = std::clamp (-vin * (rf * invRin), -lim, lim);

            const float id = dPos.current (vOut) - dNeg.current (-vOut);
            const float gd = dPos.conductance (vOut) + dNeg.conductance (-vOut);
            // Vin/Rin + Vout/Rf + Id + geq*Vout + ieq = 0
            const float f  = vin * invRin + vOut * invRf + id + geq * vOut + ieq;
            const float df = invRf + gd + geq;

            if (std::abs (df) < 1.0e-20f)
                break;

            float dv = f / df;
            dv = std::clamp (dv, -stepLim, stepLim);
            vOut -= dv;

            if (std::abs (dv) < 1.0e-9f)
                break;
        }

        vOut = std::clamp (vOut, -lim * 2.0f, lim * 2.0f);
        if (! std::isfinite (vOut))
            vOut = 0.0f;
    }
};
} // namespace devices
