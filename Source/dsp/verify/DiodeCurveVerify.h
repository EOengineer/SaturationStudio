#pragma once

#include "../models/DiodeCurve.h"
#include "../models/DiodeModel.h"
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

        // Skip startup
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

    // --- High Drive: stronger 3rd than Drive 0.5, finite ---
    {
        auto thirdAt = [&] (float drive) -> double
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
            return goertzelPower (buf.data() + 256, n - 256, 3.0 * f0, sr);
        };

        const double h3lo = thirdAt (0.5f);
        const double h3hi = thirdAt (1.0f);
        std::ostringstream oss;
        oss << "Drive ramp H3(0.5)=" << h3lo << " H3(1.0)=" << h3hi;
        if (h3lo < 0.0 || h3hi < 0.0)
            r.fail (oss.str() + " (NaN)");
        else if (h3hi > h3lo * 1.5)
            r.pass (oss.str());
        else
            r.fail (oss.str());
    }

    // --- Curve math sanity: f'(0)≈1, |f| < |a| asymptote ---
    {
        const auto c = diode::coeffsForFlavor (DiodeFlavorIds::silicon);
        const float eps = 1.0e-4f;
        const float deriv = (diode::shape (eps, c) - diode::shape (-eps, c)) / (2.0f * eps);
        if (std::abs (deriv - 1.0f) < 1.0e-3f)
            r.pass ("shape derivative at 0 ≈ 1");
        else
            r.fail ("shape derivative at 0 not ≈ 1");

        const float big = diode::shape (100.0f, c);
        if (big < c.aPos && big > 0.0f)
            r.pass ("shape asymptote under aPos");
        else
            r.fail ("shape asymptote check");
    }

    return r;
}
} // namespace verify
