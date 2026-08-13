#pragma once

#include "../models/TubeCurve.h"
#include "../models/TubeModel.h"
#include "../models/DiodeModel.h"
#include "../LevelReference.h"
#include "../../util/ParamIDs.h"
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace verify
{
struct TubeCurveReport
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

inline double tubeGoertzelPower (const float* x, int n, double freqHz, double sr)
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

inline TubeCurveReport runTubeCurveVerifications()
{
    TubeCurveReport r;
    constexpr double sr = 48000.0;
    constexpr int n = 8192;
    constexpr float kPi = 3.14159265358979323846f;

    // Drive=0 identity
    {
        TubeModel m;
        m.prepare (sr, n, 1);
        m.setDrive (0.0f);
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

    // −18 / Drive 0.5: even > odd (vs Silicon diode odd-heavy)
    {
        auto runH2H3 = [&] (auto& model) -> std::pair<double, double>
        {
            const float rmsLin = std::pow (10.0f, LevelReference::kReferenceRmsDb / 20.0f);
            const float peak = rmsLin * std::sqrt (2.0f);
            constexpr float f0 = 1000.0f;
            std::vector<float> buf ((size_t) n);
            for (int i = 0; i < n; ++i)
                buf[(size_t) i] = peak * std::sin (2.0f * kPi * f0 * (float) i / (float) sr);
            float* chans[] = { buf.data() };
            model.process (chans, 1, n);
            const double h2 = tubeGoertzelPower (buf.data() + 256, n - 256, 2.0 * f0, sr);
            const double h3 = tubeGoertzelPower (buf.data() + 256, n - 256, 3.0 * f0, sr);
            return { h2, h3 };
        };

        TubeModel tube;
        tube.prepare (sr, n, 1);
        tube.setDrive (0.5f);
        tube.reset();
        const auto [h2t, h3t] = runH2H3 (tube);

        DiodeModel diode;
        diode.prepare (sr, n, 1);
        diode.setDrive (0.5f);
        diode.setDiodeFlavor (DiodeFlavorIds::silicon);
        diode.reset();
        const auto [h2d, h3d] = runH2H3 (diode);

        std::ostringstream oss;
        oss << "Tube H2=" << h2t << " H3=" << h3t << " | Si diode H2=" << h2d << " H3=" << h3d;

        if (h2t < 1.0e-6)
            r.fail (oss.str() + " (Tube even too weak)");
        else if (! (h2t > h3t))
            r.fail (oss.str() + " (expected Tube H2 > H3)");
        else if (! (h2t > h2d * 2.0))
            r.fail (oss.str() + " (Tube should be more even than Si diode)");
        else
            r.pass (oss.str());
    }

    // Drive ramp + finite
    {
        auto third = [&] (float drive) -> double
        {
            TubeModel m;
            m.prepare (sr, n, 1);
            m.setDrive (drive);
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
            return tubeGoertzelPower (buf.data() + 256, n - 256, 2.0 * f0, sr);
        };

        const double h2lo = third (0.5f);
        const double h2hi = third (1.0f);
        // Also check overall error growth (H2 alone can fall when waveform hardens)
        auto errAt = [&] (float drive) -> double
        {
            TubeModel m;
            m.prepare (sr, n, 1);
            m.setDrive (drive);
            m.reset();
            const float rmsLin = std::pow (10.0f, LevelReference::kReferenceRmsDb / 20.0f);
            const float peak = rmsLin * std::sqrt (2.0f);
            constexpr float f0 = 1000.0f;
            std::vector<float> buf ((size_t) n);
            for (int i = 0; i < n; ++i)
                buf[(size_t) i] = peak * std::sin (2.0f * kPi * f0 * (float) i / (float) sr);
            float* chans[] = { buf.data() };
            m.process (chans, 1, n);
            double e = 0.0, ref = 0.0;
            for (int i = 256; i < n; ++i)
            {
                const double dry = (double) peak * std::sin (2.0 * kPi * (double) f0 * (double) i / sr);
                const double d = (double) buf[(size_t) i] - dry;
                e += d * d;
                ref += dry * dry;
            }
            return ref > 0.0 ? e / ref : e;
        };
        const double elo = errAt (0.5f);
        const double ehi = errAt (1.0f);
        std::ostringstream oss;
        oss << "Drive ramp H2(0.5)=" << h2lo << " H2(1.0)=" << h2hi
            << " err(0.5)=" << elo << " err(1.0)=" << ehi;
        if (h2lo < 0.0 || h2hi < 0.0)
            r.fail (oss.str() + " (NaN)");
        else if (ehi > elo * 1.25)
            r.pass (oss.str());
        else
            r.fail (oss.str());
    }

    // f'(0)≈1
    {
        const auto c = tube::defaultCoeffs();
        const float eps = 1.0e-4f;
        const float deriv = (tube::shape (eps, c) - tube::shape (-eps, c)) / (2.0f * eps);
        if (std::abs (deriv - 1.0f) < 1.0e-3f)
            r.pass ("shape derivative at 0 ≈ 1");
        else
            r.fail ("shape derivative at 0 not ≈ 1");
    }

    return r;
}
} // namespace verify
