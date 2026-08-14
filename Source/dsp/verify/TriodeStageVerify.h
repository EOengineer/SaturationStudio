#pragma once

#include "../devices/TriodeStage.h"
#include "../devices/TubeDevice.h"
#include "../LevelReference.h"
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace verify
{
struct TriodeStageReport
{
    bool ok = true;
    std::string text;
    void pass (const std::string& msg) { text += "PASS: " + msg + "\n"; }
    void fail (const std::string& msg)
    {
        ok = false;
        text += "FAIL: " + msg + "\n";
    }
};

inline double stageGoertzelPower (const float* x, int n, double freqHz, double sr)
{
    const double w = 2.0 * 3.14159265358979323846 * freqHz / sr;
    const double cw = std::cos (w);
    const double coeff = 2.0 * cw;
    double s0 = 0.0, s1 = 0.0, s2 = 0.0;
    for (int i = 0; i < n; ++i)
    {
        s0 = (double) x[i] + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    return std::max (s1 * s1 + s2 * s2 - coeff * s1 * s2, 0.0);
}

inline TriodeStageReport runTriodeStageVerifications()
{
    TriodeStageReport r;
    constexpr double sr = 48000.0;
    constexpr int n = 8192;
    constexpr float kPi = 3.14159265358979323846f;

    // Idle settle: finite Vp/Vk; Ip > 0 for AX7
    {
        devices::TriodeStage stage;
        stage.prepare ((float) sr, devices::twelveAx7());
        stage.setDrive (0.0f);
        stage.reset();

        const float vp = stage.getPlate();
        const float vk = stage.getCathode();
        const float vgk = 0.0f - vk;
        const float vak = vp - vk;
        const float ip = devices::twelveAx7().plateCurrent (vgk, vak);

        std::ostringstream oss;
        oss << "AX7 idle Vp=" << vp << " Vk=" << vk << " Ip=" << ip;
        if (std::isfinite (vp) && std::isfinite (vk) && vp > 20.0f && vp < 400.0f
            && vk > 0.1f && vk < 15.0f && ip > 1.0e-5f)
            r.pass (oss.str());
        else
            r.fail (oss.str());
    }

    // Hot sine finite at Drive mid/high
    {
        auto runFinite = [&] (float drive) -> bool
        {
            devices::TriodeStage stage;
            stage.prepare ((float) sr, devices::twelveAx7());
            stage.setDrive (drive);
            stage.reset();

            const float rmsLin = std::pow (10.0f, LevelReference::kReferenceRmsDb / 20.0f);
            const float peak = rmsLin * std::sqrt (2.0f);
            for (int i = 0; i < n; ++i)
            {
                const float vin = peak * std::sin (2.0f * kPi * 1000.0f * (float) i / (float) sr);
                const float y = stage.processSample (vin);
                if (! std::isfinite (y) || ! std::isfinite (stage.getPlate())
                    || ! std::isfinite (stage.getCathode()))
                    return false;
            }
            return true;
        };

        const bool mid = runFinite (0.5f);
        const bool hot = runFinite (1.0f);
        std::ostringstream oss;
        oss << "hot sine finite Drive0.5=" << (mid ? "ok" : "FAIL")
            << " Drive1=" << (hot ? "ok" : "FAIL");
        if (mid && hot)
            r.pass (oss.str());
        else
            r.fail (oss.str());
    }

    // AX7 |gain| / energy > AU7 at same Drive
    {
        auto rmsOut = [&] (devices::TubeDevice tube, float drive) -> double
        {
            devices::TriodeStage stage;
            stage.prepare ((float) sr, tube);
            stage.setDrive (drive);
            stage.reset();

            const float rmsLin = std::pow (10.0f, LevelReference::kReferenceRmsDb / 20.0f);
            const float peak = rmsLin * std::sqrt (2.0f);
            double sum = 0.0;
            const int skip = 1024;
            for (int i = 0; i < n; ++i)
            {
                const float vin = peak * std::sin (2.0f * kPi * 1000.0f * (float) i / (float) sr);
                const float y = stage.processSample (vin);
                if (i >= skip)
                    sum += (double) y * (double) y;
            }
            return std::sqrt (sum / (double) (n - skip));
        };

        const double ax = rmsOut (devices::twelveAx7(), 0.55f);
        const double au = rmsOut (devices::twelveAu7(), 0.55f);
        std::ostringstream oss;
        oss << "Drive0.55 RMS out AX7=" << ax << " AU7=" << au;
        if (ax > au * 1.15 && ax > 1.0e-4 && au >= 0.0)
            r.pass (oss.str());
        else
            r.fail (oss.str() + " (expected AX7 louder/gainier than AU7)");
    }

    // Drive≈0: small-signal output much quieter than Drive 0.5
    {
        auto energy = [&] (float drive) -> double
        {
            devices::TriodeStage stage;
            stage.prepare ((float) sr, devices::twelveAx7());
            stage.setDrive (drive);
            stage.reset();

            const float rmsLin = std::pow (10.0f, LevelReference::kReferenceRmsDb / 20.0f);
            const float peak = rmsLin * std::sqrt (2.0f) * 0.25f;
            double sum = 0.0;
            const int skip = 1024;
            for (int i = 0; i < n; ++i)
            {
                const float vin = peak * std::sin (2.0f * kPi * 440.0f * (float) i / (float) sr);
                const float y = stage.processSample (vin);
                if (! std::isfinite (y))
                    return -1.0;
                if (i >= skip)
                    sum += (double) y * (double) y;
            }
            return sum;
        };

        const double e0 = energy (0.0f);
        const double e5 = energy (0.5f);
        std::ostringstream oss;
        oss << "small-signal energy Drive0=" << e0 << " Drive0.5=" << e5;
        if (e0 >= 0.0 && e5 > e0 * 20.0)
            r.pass (oss.str());
        else
            r.fail (oss.str() + " (expected Drive0 << Drive0.5)");
    }

    // Abuse Drive 1 + hot peak: still finite
    {
        devices::TriodeStage stage;
        stage.prepare ((float) sr, devices::twelveAx7());
        stage.setDrive (1.0f);
        stage.reset();

        bool finite = true;
        for (int i = 0; i < n; ++i)
        {
            const float vin = 0.95f * std::sin (2.0f * kPi * 2000.0f * (float) i / (float) sr);
            const float y = stage.processSample (vin);
            if (! std::isfinite (y) || ! std::isfinite (stage.getPlate()))
                finite = false;
        }
        if (finite)
            r.pass ("abuse Drive1 peak0.95 finite");
        else
            r.fail ("abuse Drive1 produced NaN/Inf");
    }

    // Harmonic content at Drive 0.7
    {
        devices::TriodeStage stage;
        stage.prepare ((float) sr, devices::twelveAx7());
        stage.setDrive (0.7f);
        stage.reset();

        const float rmsLin = std::pow (10.0f, LevelReference::kReferenceRmsDb / 20.0f);
        const float peak = rmsLin * std::sqrt (2.0f);
        constexpr float f0 = 1000.0f;
        std::vector<float> buf ((size_t) n);
        for (int i = 0; i < n; ++i)
        {
            const float vin = peak * std::sin (2.0f * kPi * f0 * (float) i / (float) sr);
            buf[(size_t) i] = stage.processSample (vin);
        }
        const double h1 = stageGoertzelPower (buf.data() + 512, n - 512, f0, sr);
        const double h2 = stageGoertzelPower (buf.data() + 512, n - 512, 2.0 * f0, sr);
        const double h3 = stageGoertzelPower (buf.data() + 512, n - 512, 3.0 * f0, sr);
        std::ostringstream oss;
        oss << "AX7 Drive0.7 H1=" << h1 << " H2=" << h2 << " H3=" << h3;
        if (h1 > 1.0e-6 && (h2 + h3) > h1 * 1.0e-4)
            r.pass (oss.str());
        else
            r.fail (oss.str() + " (expected measurable harmonics)");
    }

    // Coupling HPF: after a DC-ish step into the stage settles, AC out → ~0
    {
        devices::TriodeStage stage;
        stage.prepare ((float) sr, devices::twelveAx7());
        stage.setDrive (0.5f);
        stage.reset();

        // Warm with AC, then hold a constant input; coupling should settle near 0.
        const float hold = 0.2f;
        for (int i = 0; i < 2048; ++i)
            (void) stage.processSample (hold * std::sin (2.0f * kPi * 100.0f * (float) i / (float) sr));
        for (int i = 0; i < 8192; ++i)
            (void) stage.processSample (hold);

        double sum = 0.0;
        const int settle = 4096;
        for (int i = 0; i < settle; ++i)
        {
            const float y = stage.processSample (hold);
            sum += (double) y * (double) y;
        }
        const double rms = std::sqrt (sum / (double) settle);
        std::ostringstream oss;
        oss << "coupling DC hold RMS=" << rms;
        if (std::isfinite (rms) && rms < 5.0e-3)
            r.pass (oss.str());
        else
            r.fail (oss.str() + " (expected HPF to block DC)");
    }

    // Miller: high-freq energy ≤ mid-freq (mild HF roll-off into grid)
    {
        auto energyAt = [&] (float fHz) -> double
        {
            devices::TriodeStage stage;
            stage.prepare ((float) sr, devices::twelveAx7());
            stage.setDrive (0.5f);
            stage.reset();

            const float peak = 0.05f; // small-signal — stay linear-ish
            double sum = 0.0;
            const int skip = 2048;
            for (int i = 0; i < n; ++i)
            {
                const float vin = peak * std::sin (2.0f * kPi * fHz * (float) i / (float) sr);
                const float y = stage.processSample (vin);
                if (i >= skip)
                    sum += (double) y * (double) y;
            }
            return sum;
        };

        const double eMid = energyAt (1000.0f);
        const double eHi = energyAt (15000.0f);
        std::ostringstream oss;
        oss << "Miller energy 1k=" << eMid << " 15k=" << eHi;
        if (eMid > 1.0e-8 && eHi >= 0.0 && eHi <= eMid * 1.05)
            r.pass (oss.str());
        else
            r.fail (oss.str() + " (expected HF ≤ mid)");
    }

    // Grid conduction: hard positive Vdrive does not brick-wall Vg at +1 V
    {
        devices::TriodeStage stage;
        stage.prepare ((float) sr, devices::twelveAx7());
        stage.setDrive (1.0f);
        stage.reset();

        float maxVg = -1.0e9f;
        float minVg = 1.0e9f;
        for (int i = 0; i < n; ++i)
        {
            const float vin = 0.9f * std::sin (2.0f * kPi * 200.0f * (float) i / (float) sr);
            (void) stage.processSample (vin);
            maxVg = std::max (maxVg, stage.getGrid());
            minVg = std::min (minVg, stage.getGrid());
            if (! std::isfinite (stage.getGrid()))
            {
                maxVg = -1.0f;
                break;
            }
        }
        std::ostringstream oss;
        oss << "grid conduction Vg range [" << minVg << "," << maxVg << "]";
        // Soft diode: positive peaks may exceed old +1 V clamp; still finite and bounded.
        if (std::isfinite (maxVg) && std::isfinite (minVg) && maxVg > 0.2f && maxVg < 20.0f
            && minVg > -80.0f)
            r.pass (oss.str());
        else
            r.fail (oss.str() + " (expected soft grid dig-in, not NaN/brick)");
    }

    return r;
}
} // namespace verify
