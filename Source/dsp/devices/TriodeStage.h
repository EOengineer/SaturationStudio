#pragma once

#include "DiodeDevice.h"
#include "NewtonSolver.h"
#include "TubeDevice.h"
#include <algorithm>
#include <array>
#include <cmath>

/**
 * Cathode-biased common-cathode triode island — first circuit consumer of TubeDevice.
 *
 * Unknowns: plate Vp, cathode Vk, grid Vg (3×3 Newton).
 * Drive: Vdrive = G(drive)·vin through series Rg into Vg.
 * Grid–cathode: teaching Shockley diode (conduction) + Cgk.
 * Plate–grid: Cgp (true Miller via stamped C, not a fixed LPF).
 * KCL plate:   (Vb - Vp)/Ra - Ip - Icgp = 0
 * KCL cathode: Ip - Vk/Rk - Ick + Icgk + Ig = 0
 * KCL grid:    (Vdrive - Vg)/Rg - Ig - Icgk + Icgp - Vg/Rleak = 0
 * Output: plate AC scale → Cc/Rl coupling HPF.
 *
 * Teaching defaults — not a Champ amp. No NFB.
 * Live TubeModel stamps this stage per channel inside the 4× OS island.
 *
 * See docs/devices/overview.md, docs/models/tube.md (Tech debt).
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

/** Trapezoidal companion for a floating capacitor (Cgp / Cgk). */
struct BranchCapTrap
{
    float geq = 0.0f;
    float iEq = 0.0f;

    void prepare (float capacitance, float fs) noexcept
    {
        const float T = 1.0f / std::max (fs, 1.0f);
        geq = 2.0f * std::max (capacitance, 0.0f) / T;
        reset();
    }

    void reset() noexcept { iEq = 0.0f; }

    /** Current from node A → node B. */
    float current (float vA, float vB) const noexcept
    {
        return geq * (vA - vB) - iEq;
    }

    void advance (float vA, float vB) noexcept
    {
        const float v = vA - vB;
        const float i = geq * v - iEq;
        iEq = geq * v + i;
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

/** Teaching grid–cathode diode (not a measured 12AX7 Ig curve). */
inline DiodeDevice teachingGridDiode() noexcept
{
    // Soft forward around Vgk≳0; modest Rs so G stays nonzero when |Vj| hits vMax (Newton).
    return { 1.0e-12f, 0.026f * 2.0f, 0.7f, 50.0f };
}

class TriodeStage
{
public:
    /**
     * Default teaching load:
     * Ra=120k, Rk=1.5k, Ck=22µF, Vb=250 V,
     * Cc=22nF / Rl=1M (coupling),
     * Rg=68k, Rleak=1M, Cgp=Cgk=1.6pF.
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
                  float gridLeakR = 1.0e6f,
                  float cgpFarads = 1.6e-12f,
                  float cgkFarads = 1.6e-12f) noexcept
    {
        fs = std::max (sampleRateHz, 1.0f);
        tube = tubeDevice;
        ra = std::max (plateR, 1.0f);
        rk = std::max (cathodeR, 1.0f);
        vb = bplus;
        ga = 1.0f / ra;
        gk = 1.0f / rk;
        rg = std::max (gridStopR, 1.0f);
        gg = 1.0f / rg;
        rleak = std::max (gridLeakR, 1.0f);
        gleak = 1.0f / rleak;
        gridDiode = teachingGridDiode();
        bypass.prepare (bypassC, fs);
        cgp.prepare (cgpFarads, fs);
        cgk.prepare (cgkFarads, fs);
        coupling.prepare (couplingC, couplingLoadR, fs);
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
        vk = 1.5f;
        vp = 170.0f;
        vg = 0.0f;
        bypass.reset();
        cgp.reset();
        cgk.reset();
        coupling.reset();

        // DC solve with C open — seed traps at true DC so vin=0 does not drift.
        settleDc();
        bypass.vPrev = vk;
        bypass.iEq = bypass.geq * vk;
        // Floating caps at DC: v constant → iEq = geq * v so current is 0.
        cgp.iEq = cgp.geq * (vp - vg);
        cgk.iEq = cgk.geq * (vg - vk);

        for (int i = 0; i < 8; ++i)
            (void) processSampleRaw (0.0f);

        bypass.vPrev = vk;
        bypass.iEq = bypass.geq * vk;
        cgp.iEq = cgp.geq * (vp - vg);
        cgk.iEq = cgk.geq * (vg - vk);
        idleVp = vp;

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
        const float ac = (vpNow - idleVp) * kPlateToAudio;
        return coupling.process (ac);
    }

    float getPlate() const noexcept { return vp; }
    float getCathode() const noexcept { return vk; }
    float getGrid() const noexcept { return vg; }
    float getIdlePlate() const noexcept { return idleVp; }
    float getGridGain() const noexcept { return gridGain; }

private:
    static constexpr float kPlateToAudio = 1.0f / 300.0f;
    static constexpr float kMaxGridGain = 86.4f; // +20% vs prior 72
    static constexpr float kDriveCurve = 2.55f;
    /** Soft-limit |Vdrive| so Drive 1 does not thrash 3×3 Newton (RT). */
    static constexpr float kVdriveLimit = 18.0f;

    void updateGridGain() noexcept
    {
        const float t = std::pow (drive, kDriveCurve);
        gridGain = std::max (1.0e-4f, t * kMaxGridGain);
    }

    /** DC Newton with all C open (companions zero). */
    void settleDc() noexcept
    {
        std::array<float, 3> x { vp, vk, vg };
        NewtonSolver<3> newton;
        newton.maxIterations = 24;
        newton.maxLineSearch = 4;
        newton.absTol = 1.0e-8f;

        const auto fill = [&] (const std::array<float, 3>& xIn,
                               std::array<float, 3>& f,
                               std::array<float, 3 * 3>& j)
        {
            fillKcl (xIn, /*vdrive*/ 0.0f,
                     /*geqCk*/ 0.0f, /*iEqCk*/ 0.0f,
                     /*geqGp*/ 0.0f, /*iEqGp*/ 0.0f,
                     /*geqGk*/ 0.0f, /*iEqGk*/ 0.0f,
                     f, j);
        };

        newton.solve (x, fill);
        if (std::isfinite (x[0]) && std::isfinite (x[1]) && std::isfinite (x[2]))
        {
            vp = x[0];
            vk = x[1];
            vg = x[2];
        }
    }

    void fillKcl (const std::array<float, 3>& xIn,
                  float vdrive,
                  float geqCk,
                  float iEqCk,
                  float geqGp,
                  float iEqGp,
                  float geqGk,
                  float iEqGk,
                  std::array<float, 3>& f,
                  std::array<float, 3 * 3>& j) const noexcept
    {
        const float vpX = xIn[0];
        const float vkX = xIn[1];
        const float vgX = xIn[2];
        const float vgk = vgX - vkX;
        const float vak = vpX - vkX;

        float gG = 0.0f, gP = 0.0f, ip = 0.0f;
        tube.plateConductances (vgk, vak, gG, gP, &ip);

        float ig = 0.0f, gDiode = 0.0f;
        gridDiode.stamp (vgk, ig, gDiode);

        const float ick = geqCk * vkX - iEqCk;
        const float icgp = geqGp * (vpX - vgX) - iEqGp; // plate → grid
        const float icgk = geqGk * (vgX - vkX) - iEqGk; // grid → cathode

        // Plate: (Vb-Vp)/Ra - Ip - Icgp = 0
        f[0] = (vb - vpX) * ga - ip - icgp;
        // Cathode: Ip - Vk/Rk - Ick + Icgk + Ig = 0
        f[1] = ip - vkX * gk - ick + icgk + ig;
        // Grid: (Vdrive-Vg)/Rg - Ig - Icgk + Icgp - Vg/Rleak = 0
        f[2] = (vdrive - vgX) * gg - ig - icgk + icgp - vgX * gleak;

        const float dIp_dVp = gP;
        const float dIp_dVk = -gG - gP;
        const float dIp_dVg = gG;

        // df0 / d(Vp,Vk,Vg)
        j[0] = -ga - dIp_dVp - geqGp;
        j[1] = -dIp_dVk;
        j[2] = -dIp_dVg + geqGp;

        // df1 / d(Vp,Vk,Vg)
        j[3] = dIp_dVp;
        j[4] = dIp_dVk - gk - geqCk - geqGk - gDiode;
        j[5] = dIp_dVg + geqGk + gDiode;

        // df2 / d(Vp,Vk,Vg)
        j[6] = geqGp;
        j[7] = gDiode + geqGk;
        j[8] = -gg - gDiode - geqGk - geqGp - gleak;
    }

    float processSampleRaw (float vin) noexcept
    {
        // Soft-limit Vdrive: keeps Drive 1 in a Newton-friendly range (see Tech debt).
        const float raw = gridGain * vin;
        const float lim = kVdriveLimit;
        const float vdrive = lim * std::tanh (raw / lim);

        std::array<float, 3> x { vp, vk, vg };
        NewtonSolver<3> newton;
        newton.maxIterations = 8;
        newton.maxLineSearch = 3;
        newton.absTol = 1.0e-6f;

        const float geqCk = bypass.geq;
        const float iEqCk = bypass.iEq;
        const float geqGp = cgp.geq;
        const float iEqGp = cgp.iEq;
        const float geqGk = cgk.geq;
        const float iEqGk = cgk.iEq;

        const auto fill = [&] (const std::array<float, 3>& xIn,
                               std::array<float, 3>& f,
                               std::array<float, 3 * 3>& j)
        {
            fillKcl (xIn, vdrive, geqCk, iEqCk, geqGp, iEqGp, geqGk, iEqGk, f, j);
        };

        newton.solve (x, fill);
        if (! std::isfinite (x[0]) || ! std::isfinite (x[1]) || ! std::isfinite (x[2]))
            return vp;

        vp = x[0];
        vk = x[1];
        vg = x[2];
        bypass.advance (vk);
        cgp.advance (vp, vg);
        cgk.advance (vg, vk);
        return vp;
    }

    float fs = 48000.0f;
    float ra = 120.0e3f, rk = 1.5e3f, vb = 250.0f;
    float ga = 0.0f, gk = 0.0f;
    float rg = 68.0e3f, gg = 0.0f;
    float rleak = 1.0e6f, gleak = 0.0f;
    float vp = 170.0f, vk = 1.5f, vg = 0.0f, idleVp = 170.0f;
    float drive = 0.5f;
    float gridGain = 1.0f;
    CapacitorTrap bypass;
    BranchCapTrap cgp;
    BranchCapTrap cgk;
    CouplingHpf coupling;
    DiodeDevice gridDiode {};
    TubeDevice tube {};
};
} // namespace devices
