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
 * Grid: Vg = G(drive) * vin (plugin-scale AC → grid volts), clamped.
 * KCL plate:   (Vb - Vp)/Ra - Ip = 0
 * KCL cathode: Ip - Vk/Rk - Ic = 0  (Ck trapezoidal companion)
 *
 * Teaching defaults (Ra/Rk/Ck/Vb) are starting points — not a Champ amp.
 * No NFB. Live TubeModel still uses waveshape; this stage is offline-first.
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

class TriodeStage
{
public:
    /** Default teaching load: Ra=100k, Rk=1.5k, Ck=22µF, Vb=250 V. */
    void prepare (float sampleRateHz,
                  TubeDevice tubeDevice = twelveAx7(),
                  float plateR = 100.0e3f,
                  float cathodeR = 1.5e3f,
                  float bypassC = 22.0e-6f,
                  float bplus = 250.0f) noexcept
    {
        fs = std::max (sampleRateHz, 1.0f);
        tube = tubeDevice;
        ra = std::max (plateR, 1.0f);
        rk = std::max (cathodeR, 1.0f);
        vb = bplus;
        ga = 1.0f / ra;
        gk = 1.0f / rk;
        bypass.prepare (bypassC, fs);
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
    }

    /**
     * One sample. vin = plugin-scale audio (−18 dBFS ≈ 0.126 RMS).
     * Returns AC plate (vp − idle) scaled toward plugin float range.
     */
    float processSample (float vin) noexcept
    {
        const float vpNow = processSampleRaw (vin);
        if (! std::isfinite (vpNow))
            return 0.0f;
        // Plate AC is tens of volts; bring toward plugin-ish amplitude.
        return (vpNow - idleVp) * kPlateToAudio;
    }

    float getPlate() const noexcept { return vp; }
    float getCathode() const noexcept { return vk; }
    float getIdlePlate() const noexcept { return idleVp; }
    float getGridGain() const noexcept { return gridGain; }

private:
    static constexpr float kPlateToAudio = 1.0f / 40.0f;
    static constexpr float kMaxGridGain = 28.0f; // vin=1 → 28 V before clamp
    static constexpr float kDriveCurve = 1.35f;

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

    /** Newton step; returns raw plate volts. Updates vp/vk/bypass. */
    float processSampleRaw (float vin) noexcept
    {
        float vg = gridGain * vin;
        vg = std::clamp (vg, -5.0f, 1.0f);

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
    float ra = 100.0e3f, rk = 1.5e3f, vb = 250.0f;
    float ga = 0.0f, gk = 0.0f;
    float vp = 170.0f, vk = 1.5f, idleVp = 170.0f;
    float drive = 0.5f;
    float gridGain = 1.0f;
    CapacitorTrap bypass;
    TubeDevice tube {};
};
} // namespace devices
