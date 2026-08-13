#pragma once

#include "../models/DiodeCurve.h"
#include "../models/DiodeModel.h"
#include "../devices/DiodeDevice.h"
#include "../devices/AntiParallelClipper.h"
#include "../devices/AntiParallelRcClipper.h"
#include "../devices/FeedbackDiodeClipper.h"
#include "../LevelReference.h"
#include "../../util/ParamIDs.h"
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace verify
{
struct DiodeCurveReport
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

inline double goertzelPower (const float* x, int n, double freqHz, double sr)
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
    const double power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
    return std::max (power, 0.0);
}

inline DiodeCurveReport runDiodeCurveVerifications()
{
    DiodeCurveReport r;
    constexpr double sr = 48000.0;
    constexpr int n = 8192;
    constexpr float kPi = 3.14159265358979323846f;

    // --- Device: conductance matches finite-difference dI/dV ---
    {
        const auto d = devices::siliconSignal();
        const float v = 0.4f;
        const float eps = 1.0e-5f;
        const float fd = (d.current (v + eps) - d.current (v - eps)) / (2.0f * eps);
        const float g = d.conductance (v);
        const float rel = std::abs (g - fd) / std::max (std::abs (fd), 1.0e-20f);
        std::ostringstream oss;
        oss << "Si conductance vs FD relErr=" << rel;
        if (rel < 5.0e-3f)
            r.pass (oss.str());
        else
            r.fail (oss.str());
    }

    // --- Device: Ge conducts earlier than Si; LED later (same I probe) ---
    {
        constexpr float iProbe = 1.0e-4f; // 0.1 mA
        auto vfAt = [] (const devices::DiodeDevice& d, float iTarget) -> float
        {
            return d.nVt * std::log (iTarget / d.isat + 1.0f);
        };
        const float vfGe = vfAt (devices::germanium(), iProbe);
        const float vfSi = vfAt (devices::siliconSignal(), iProbe);
        const float vfLed = vfAt (devices::ledRed(), iProbe);
        std::ostringstream oss;
        oss << "Vf order Ge=" << vfGe << " Si=" << vfSi << " LED=" << vfLed;
        if (vfGe < vfSi && vfSi < vfLed)
            r.pass (oss.str());
        else
            r.fail (oss.str());
    }

    // --- Clipper DC: small-signal ≈ identity, large-signal compresses ---
    {
        const auto setup = diode::setupForFlavor (DiodeFlavorIds::silicon);
        const float small = setup.clipper.solveDc (0.05f);
        const float bigIn = 5.0f;
        const float big = setup.clipper.solveDc (bigIn);
        const float smallErr = std::abs (small - 0.05f);
        std::ostringstream oss;
        oss << "clipper DC small=" << small << " big=" << big;
        if (smallErr < 1.0e-3f && big > 0.0f && big < 1.2f)
            r.pass (oss.str());
        else
            r.fail (oss.str());
    }

    // --- Drive≈0 identity ---
    {
        DiodeModel m;
        m.prepare (sr, n, 1);
        m.setDrive (0.0f);
        m.setDiodeFlavor (DiodeFlavorIds::silicon);
        m.reset();

        std::vector<float> buf ((size_t) n);
        for (int i = 0; i < n; ++i)
            buf[(size_t) i] = 0.5f * std::sin (2.0f * kPi * 440.0f * (float) i / (float) sr);

        std::vector<float> dry = buf;
        float* chans[] = { buf.data() };
        m.process (chans, 1, n);

        double err = 0.0, ref = 0.0;
        for (int i = 0; i < n; ++i)
        {
            const double e = (double) buf[(size_t) i] - (double) dry[(size_t) i];
            err += e * e;
            ref += (double) dry[(size_t) i] * (double) dry[(size_t) i];
        }
        const double rel = ref > 1.0e-18 ? err / ref : err;
        std::ostringstream oss;
        oss << "Drive=0 identity relErr=" << rel;
        if (rel < 1.0e-12)
            r.pass (oss.str());
        else
            r.fail (oss.str());
    }

    // --- −18 dBFS sine @ Drive 0.5 Silicon: mild odd harmonics, weak even ---
    {
        DiodeModel m;
        m.prepare (sr, n, 1);
        m.setDrive (0.5f);
        m.setDiodeFlavor (DiodeFlavorIds::silicon);
        m.reset();

        const float rmsLin = std::pow (10.0f, LevelReference::kReferenceRmsDb / 20.0f);
        const float peak = rmsLin * std::sqrt (2.0f);
        constexpr float f0 = 1000.0f;

        std::vector<float> buf ((size_t) n);
        for (int i = 0; i < n; ++i)
            buf[(size_t) i] = peak * std::sin (2.0f * kPi * f0 * (float) i / (float) sr);

        float* chans[] = { buf.data() };
        m.process (chans, 1, n);

        const float* x = buf.data() + 256;
        const int nn = n - 256;
        const double p1 = goertzelPower (x, nn, f0, sr);
        const double p2 = goertzelPower (x, nn, 2.0 * f0, sr);
        const double p3 = goertzelPower (x, nn, 3.0 * f0, sr);

        bool finite = true;
        for (int i = 0; i < n; ++i)
            if (! std::isfinite (buf[(size_t) i]))
                finite = false;

        std::ostringstream oss;
        oss << "Si Drive0.5 @-18 H1=" << p1 << " H2=" << p2 << " H3=" << p3;

        if (! finite)
            r.fail (oss.str() + " (NaN/Inf)");
        else if (p1 < 1.0e-6)
            r.fail (oss.str() + " (fundamental too weak)");
        else if (p3 < p1 * 1.0e-8)
            r.fail (oss.str() + " (expected mild 3rd)");
        else if (p3 > p1 * 0.25)
            r.fail (oss.str() + " (3rd too strong for 'mostly clean')");
        else if (p2 > p3 * 0.5)
            r.fail (oss.str() + " (even should be << odd for Si)");
        else
            r.pass (oss.str());
    }

    // --- Asymmetric: even harmonic rises vs Silicon ---
    {
        auto runFlavor = [&] (int flavor) -> double
        {
            DiodeModel m;
            m.prepare (sr, n, 1);
            m.setDrive (0.65f);
            m.setDiodeFlavor (flavor);
            m.reset();

            const float rmsLin = std::pow (10.0f, LevelReference::kReferenceRmsDb / 20.0f);
            const float peak = rmsLin * std::sqrt (2.0f);
            constexpr float f0 = 1000.0f;
            std::vector<float> buf ((size_t) n);
            for (int i = 0; i < n; ++i)
                buf[(size_t) i] = peak * std::sin (2.0f * kPi * f0 * (float) i / (float) sr);
            float* chans[] = { buf.data() };
            m.process (chans, 1, n);
            return goertzelPower (buf.data() + 256, n - 256, 2.0 * f0, sr);
        };

        const double h2Si = runFlavor (DiodeFlavorIds::silicon);
        const double h2As = runFlavor (DiodeFlavorIds::asymmetric);
        std::ostringstream oss;
        oss << "Asym H2=" << h2As << " vs Si H2=" << h2Si;
        if (h2As > h2Si * 2.0 && h2As > 1.0e-8)
            r.pass (oss.str());
        else
            r.fail (oss.str());
    }

    // --- High Drive: stronger relative 3rd than Drive 0.5, finite ---
    {
        auto thirdRatioAt = [&] (float drive) -> double
        {
            DiodeModel m;
            m.prepare (sr, n, 1);
            m.setDrive (drive);
            m.setDiodeFlavor (DiodeFlavorIds::silicon);
            m.reset();
            const float rmsLin = std::pow (10.0f, LevelReference::kReferenceRmsDb / 20.0f);
            const float peak = rmsLin * std::sqrt (2.0f);
            constexpr float f0 = 1000.0f;
            std::vector<float> buf ((size_t) n);
            for (int i = 0; i < n; ++i)
                buf[(size_t) i] = peak * std::sin (2.0f * kPi * f0 * (float) i / (float) sr);
            float* chans[] = { buf.data() };
            m.process (chans, 1, n);
            for (float v : buf)
                if (! std::isfinite (v))
                    return -1.0;
            const float* x = buf.data() + 256;
            const int nn = n - 256;
            const double p1 = goertzelPower (x, nn, f0, sr);
            const double p3 = goertzelPower (x, nn, 3.0 * f0, sr);
            if (p1 < 1.0e-18)
                return -1.0;
            return p3 / p1;
        };

        const double rLo = thirdRatioAt (0.5f);
        const double rHi = thirdRatioAt (1.0f);
        std::ostringstream oss;
        oss << "Drive ramp H3/H1(0.5)=" << rLo << " H3/H1(1.0)=" << rHi;
        if (rLo < 0.0 || rHi < 0.0)
            r.fail (oss.str() + " (NaN)");
        else if (rHi > rLo * 1.25)
            r.pass (oss.str());
        else
            r.fail (oss.str());
    }

    // --- Clipper DC small-signal slope ≈ 1 ---
    {
        const auto setup = diode::setupForFlavor (DiodeFlavorIds::silicon);
        const float eps = 1.0e-4f;
        const float deriv = (setup.clipper.solveDc (eps) - setup.clipper.solveDc (-eps)) / (2.0f * eps);
        if (std::abs (deriv - 1.0f) < 1.0e-2f)
            r.pass ("clipper DC derivative at 0 ≈ 1");
        else
            r.fail ("clipper DC derivative at 0 not ≈ 1");
    }

    // --- RC vs static: impulse leaves residual on C (static settles immediately) ---
    {
        constexpr float fs = 192000.0f;
        constexpr int nn = 2048;
        devices::AntiParallelClipper staticClip;
        staticClip.dPos = devices::siliconSignal();
        staticClip.dNeg = devices::siliconSignal();
        staticClip.rs = 1000.0f;

        devices::AntiParallelRcClipper rcClip;
        rcClip.dPos = devices::siliconSignal();
        rcClip.dNeg = devices::siliconSignal();
        rcClip.rs = 1000.0f;
        rcClip.c = 4.7e-9f;
        rcClip.prepare ((double) fs);

        devices::AntiParallelRcClipper::State st {};
        float prev = 0.0f;
        double eStatic = 0.0, eRc = 0.0;
        for (int i = 0; i < nn; ++i)
        {
            const float vin = (i == 128) ? 5.0f : 0.0f;
            const float ys = staticClip.process (vin, prev);
            const float yr = rcClip.process (vin, st);
            if (i > 128)
            {
                eStatic += (double) ys * (double) ys;
                eRc += (double) yr * (double) yr;
            }
        }
        std::ostringstream oss;
        oss << "RC post-impulse energy=" << eRc << " static=" << eStatic;
        if (eRc > 1.0e-4 && eRc > eStatic * 10.0)
            r.pass (oss.str());
        else
            r.fail (oss.str());
    }

    // --- Feedback vs RC: DC transfers differ at same Vin (topology fingerprint) ---
    {
        auto fb = diode::setupForFlavor (DiodeFlavorIds::silicon).clipper;
        devices::AntiParallelRcClipper rc;
        rc.dPos = devices::siliconSignal();
        rc.dNeg = devices::siliconSignal();
        rc.rs = 1000.0f;
        rc.c = 4.7e-9f;
        const float vin = 2.0f;
        const float yFb = fb.solveDc (vin);
        const float yRc = rc.solveDc (vin);
        std::ostringstream oss;
        oss << "FB DC=" << yFb << " RC DC=" << yRc;
        if (std::abs (yFb - yRc) > 0.05f && std::abs (yFb) > 0.1f && std::abs (yRc) > 0.1f)
            r.pass (oss.str());
        else
            r.fail (oss.str());
    }

    // --- Feedback Cf: post-impulse residual (dynamic) vs DC reopen ---
    {
        constexpr float fs = 192000.0f;
        constexpr int nn = 2048;
        devices::FeedbackDiodeClipper fb;
        fb.dPos = devices::siliconSignal();
        fb.dNeg = devices::siliconSignal();
        fb.prepare ((double) fs);
        devices::FeedbackDiodeClipper::State st {};
        double eAfter = 0.0;
        bool finite = true;
        for (int i = 0; i < nn; ++i)
        {
            const float vin = (i == 128) ? 5.0f : 0.0f;
            const float y = fb.process (vin, st);
            if (! std::isfinite (y))
                finite = false;
            if (i > 128)
                eAfter += (double) y * (double) y;
        }
        std::ostringstream oss;
        oss << "FB post-impulse energy=" << eAfter;
        if (finite && eAfter > 1.0e-6)
            r.pass (oss.str());
        else
            r.fail (oss.str() + (finite ? "" : " (NaN)"));
    }

    // --- High Drive + hot input: audible & finite (regresses Newton plateau silence) ---
    {
        for (int flavor : { DiodeFlavorIds::silicon, DiodeFlavorIds::germanium,
                            DiodeFlavorIds::led, DiodeFlavorIds::asymmetric })
        {
            DiodeModel m;
            m.prepare (sr, n, 1);
            m.setDrive (1.0f);
            m.setDiodeFlavor (flavor);
            m.reset();

            std::vector<float> buf ((size_t) n);
            for (int i = 0; i < n; ++i)
                buf[(size_t) i] = 0.9f * std::sin (2.0f * kPi * 1000.0f * (float) i / (float) sr);

            float* chans[] = { buf.data() };
            m.process (chans, 1, n);

            float peak = 0.0f;
            bool finite = true;
            for (float v : buf)
            {
                if (! std::isfinite (v))
                    finite = false;
                peak = std::max (peak, std::abs (v));
            }

            std::ostringstream oss;
            oss << "Drive1 hot flavor=" << flavor << " peak=" << peak;
            if (! finite)
                r.fail (oss.str() + " (NaN/Inf)");
            else if (peak < 0.05f)
                r.fail (oss.str() + " (near silence)");
            else
                r.pass (oss.str());
        }
    }

    return r;
}
} // namespace verify
