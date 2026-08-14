#pragma once

#include "NewtonSolver.h"
#include "TubeDevice.h"
#include <algorithm>
#include <array>
#include <cmath>

/**
 * Cathode-biased common-cathode triode island — first circuit consumer of TubeDevice.
 *
 * Unknowns: plate Vp, cathode Vk (2×2 Newton).
 * Grid: Drive→volts → Rg/Miller LPF → clamp → Newton.
 * KCL plate:   (Vb - Vp)/Ra - Ip = 0
 * KCL cathode: Ip - Vk/Rk - Ic = 0  (Ck trapezoidal companion)
 * Output: plate AC scale → Cc/Rl coupling HPF.
 *
 * Teaching defaults (Ra/Rk/Ck/Vb/Cc/Rl/Rg/Cm) are starting points — not a Champ amp.
 * Ra=120k for a bit more stage gain into plate clip.
 * Grid clamp ≈ [−8, +1] V; Drive→grid map owns saturation depth (not TubeDevice µ/KP).
 * Miller is a light 1st-order LPF (keeps 2×2 Newton; not a full Cgp stamp).
 * No NFB. Live TubeModel stamps this stage per channel inside the 4× OS island.
 *
 * See docs/devices/overview.md, docs/models/tube.md.
 */
namespace devices
{
/** Trapezoidal companion for a capacitor to ground (cathode bypass). */
struct CapacitorTrap
{
    float c = 0.0f;
    float sampleRate = 48000.0f;
    float geq = 0.0f;
    float iEq = 0.0f;
    float vPrev = 0.0f;

    void prepare (float capacitance, float fs) noexcept
    {
        c = capacitance;
        sampleRate = fs;
        const float T = 1.0f / std::max (fs, 1.0f);
        geq = 2.0f * c / T;
        reset();
    }

    void reset() noexcept
    {
        iEq = 0.0f;
        vPrev = 0.0f;
    }

    void advance (float vNew) noexcept
    {
        const float i = geq * vNew - iEq;
        iEq = geq * vNew + i;
        vPrev = vNew;
    }
};

/** Series-C / load-R high-pass (output coupling). y = α (y_prev + x − x_prev). */
struct CouplingHpf
{
    float alpha = 1.0f;
    float xPrev = 0.0f;
    float yPrev = 0.0f;

    void prepare (float capacitance, float loadR, float fs) noexcept
    {
        const float T = 1.0f / std::max (fs, 1.0f);
        const float RC = std::max (capacitance, 1.0e-15f) * std::max (loadR, 1.0f);
        alpha = RC / (RC + 0.5f * T);
        reset();
    }

    void reset() noexcept
    {
        xPrev = 0.0f;
        yPrev = 0.0f;
    }

    float process (float x) noexcept
    {
        const float y = alpha * (yPrev + x - xPrev);
        xPrev = x;
        yPrev = y;
        return y;
    }
};

/** One-pole LPF for Rg × Cmiller (grid stopper + effective Miller). */
struct MillerLpf
{
    float coeff = 1.0f;
    float y = 0.0f;

    void prepare (float rgOhms, float cmFarads, float fs) noexcept
    {
        const float tau = std::max (rgOhms, 1.0f) * std::max (cmFarads, 1.0e-18f);
        const float T = 1.0f / std::max (fs, 1.0f);
        coeff = 1.0f - std::exp (-T / tau);
        coeff = std::clamp (coeff, 1.0e-6f, 1.0f);
        reset();
    }

    void reset() noexcept { y = 0.0f; }

    float process (float x) noexcept
    {
        y += coeff * (x - y);
        return y;
    }
};

class TriodeStage
{
public:
    /**
     * Default teaching load:
     * Ra=120k, Rk=1.5k, Ck=22µF, Vb=250 V,
     * Cc=22nF / Rl=1M (coupling), Rg=68k / Cm=100pF (Miller).
     */
    void prepare (float sampleRateHz,
                  TubeDevice tubeDevice = twelveAx7(),
                  float plateR = 120.0e3f,
                  float cathodeR = 1.5e3f,
                  float bypassC = 22.0e-6f,
                  float bplus = 250.0f,
                  float couplingC = 22.0e-9f,
                  float couplingLoadR = 1.0e6f,
                  float gridStopR = 68.0e3f,
                  float millerC = 100.0e-12f) noexcept
    {
        fs = std::max (sampleRateHz, 1.0f);
        tube = tubeDevice;
        ra = std::max (plateR, 1.0f);
        rk = std::max (cathodeR, 1.0f);
        vb = bplus;
        ga = 1.0f / ra;
        gk = 1.0f / rk;
        bypass.prepare (bypassC, fs);
        coupling.prepare (couplingC, couplingLoadR, fs);
        miller.prepare (gridStopR, millerC, fs);
        updateGridGain();
        reset();
    }

    void setTube (TubeDevice tubeDevice) noexcept
    {
        tube = tubeDevice;
        reset();
    }

    /** Drive 0…1 → grid AC volts per unit plugin input (see updateGridGain). */
    void setDrive (float drive01) noexcept
    {
        drive = std::clamp (drive01, 0.0f, 1.0f);
        updateGridGain();
    }

    float getDrive() const noexcept { return drive; }

    void reset() noexcept
    {
        // Champ-typical idle seeds for 12AX7-ish / 250 V B+
        vk = 1.5f;
        vp = 170.0f;
        bypass.reset();
        miller.reset();
        coupling.reset();

        // DC solve with C open (Ic=0) — large Ck makes AC Newton stiff;
        // seed trap at true DC so vin=0 does not drift.
        settleDc();
        bypass.vPrev = vk;
        bypass.iEq = bypass.geq * vk;

        for (int i = 0; i < 8; ++i)
            (void) processSampleRaw (0.0f);

        bypass.vPrev = vk;
        bypass.iEq = bypass.geq * vk;
        idleVp = vp;

        // Coupling is AC-out only — clear after DC settle.
        miller.reset();
        coupling.reset();
    }

    /**
     * One sample. vin = plugin-scale audio (−18 dBFS ≈ 0.126 RMS).
     * Returns coupled AC plate scaled toward plugin float range.
     */
    float processSample (float vin) noexcept
    {
        const float vpNow = processSampleRaw (vin);
        if (! std::isfinite (vpNow))
            return 0.0f;
        // Plate AC is tens of volts; bring toward plugin-ish amplitude, then couple.
        const float ac = (vpNow - idleVp) * kPlateToAudio;
        return coupling.process (ac);
    }

    float getPlate() const noexcept { return vp; }
    float getCathode() const noexcept { return vk; }
    float getIdlePlate() const noexcept { return idleVp; }
    float getGridGain() const noexcept { return gridGain; }

private:
    static constexpr float kPlateToAudio = 1.0f / 300.0f; // plate AC → plugin float
    static constexpr float kMaxGridGain = 72.0f;          // overall hotter Drive→grid
    static constexpr float kDriveCurve = 2.55f;           // mid milder; Drive 1 still maxes

    void updateGridGain() noexcept
    {
        // Drive≈0 → tiny grid push; Drive 1 → hot. Soft curve for usable mid.
        const float t = std::pow (drive, kDriveCurve);
        gridGain = std::max (1.0e-4f, t * kMaxGridGain);
    }

    /** DC Newton with cathode C open (Ic = 0). */
    void settleDc() noexcept
    {
        std::array<float, 2> x { vp, vk };
        NewtonSolver<2> newton;
        newton.maxIterations = 24;
        newton.absTol = 1.0e-8f;

        const auto fill = [&] (const std::array<float, 2>& xIn,
                               std::array<float, 2>& f,
                               std::array<float, 2 * 2>& j)
        {
            fillKcl (xIn, /*vg*/ 0.0f, /*geq*/ 0.0f, /*iEq*/ 0.0f, f, j);
        };

        newton.solve (x, fill);
        if (std::isfinite (x[0]) && std::isfinite (x[1]))
        {
            vp = x[0];
            vk = x[1];
        }
    }

    void fillKcl (const std::array<float, 2>& xIn,
                  float vg,
                  float geq,
                  float iEq,
                  std::array<float, 2>& f,
                  std::array<float, 2 * 2>& j) const noexcept
    {
        const float vpX = xIn[0];
        const float vkX = xIn[1];
        const float vgk = std::clamp (vg - vkX, -5.0f, 1.0f);
        const float vak = vpX - vkX;

        float gG = 0.0f, gP = 0.0f;
        const float ip = tube.plateCurrent (vgk, vak);
        tube.plateConductances (vgk, vak, gG, gP);

        const float ic = geq * vkX - iEq;

        // KCL plate: (Vb - Vp)*ga - Ip = 0
        f[0] = (vb - vpX) * ga - ip;
        // KCL cathode: Ip - vk*gk - ic = 0
        f[1] = ip - vkX * gk - ic;

        const float dIp_dVp = gP;
        const float dIp_dVk = -gG - gP;

        j[0] = -ga - dIp_dVp;
        j[1] = -dIp_dVk;
        j[2] = dIp_dVp;
        j[3] = dIp_dVk - gk - geq;
    }

    /** Newton step; returns raw plate volts. Updates vp/vk/bypass/miller. */
    float processSampleRaw (float vin) noexcept
    {
        // Drive map → Miller (Rg×Cm) → clamp. Keeps Newton 2×2.
        float vg = miller.process (gridGain * vin);
        vg = std::clamp (vg, -8.0f, 1.0f);

        std::array<float, 2> x { vp, vk };
        NewtonSolver<2> newton;
        newton.maxIterations = 10;
        newton.absTol = 1.0e-7f;

        const float geq = bypass.geq;
        const float iEq = bypass.iEq;
        const auto fill = [&] (const std::array<float, 2>& xIn,
                               std::array<float, 2>& f,
                               std::array<float, 2 * 2>& j)
        {
            fillKcl (xIn, vg, geq, iEq, f, j);
        };

        newton.solve (x, fill);
        if (! std::isfinite (x[0]) || ! std::isfinite (x[1]))
            return vp;

        vp = x[0];
        vk = x[1];
        bypass.advance (vk);
        return vp;
    }

    float fs = 48000.0f;
    float ra = 120.0e3f, rk = 1.5e3f, vb = 250.0f;
    float ga = 0.0f, gk = 0.0f;
    float vp = 170.0f, vk = 1.5f, idleVp = 170.0f;
    float drive = 0.5f;
    float gridGain = 1.0f;
    CapacitorTrap bypass;
    CouplingHpf coupling;
    MillerLpf miller;
    TubeDevice tube {};
};
} // namespace devices
