#pragma once

#include "../models/TapeModel.h"
#include "../models/TransformerModel.h"
#include "../models/PreampModel.h"
#include "../models/DiodeModel.h"
#include "../LevelReference.h"
#include "../../util/ParamIDs.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace verify
{
struct FamilyCurvesReport
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

inline double famGoertzel (const float* x, int n, double freqHz, double sr)
{
    const double w = 2.0 * 3.14159265358979323846 * freqHz / sr;
    const double coeff = 2.0 * std::cos (w);
    double s0 = 0.0, s1 = 0.0, s2 = 0.0;
    for (int i = 0; i < n; ++i)
    {
        s0 = (double) x[i] + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    return std::max (s1 * s1 + s2 * s2 - coeff * s1 * s2, 0.0);
}

template <typename Model>
void checkDrive0Identity (FamilyCurvesReport& r, const char* name)
{
    constexpr double sr = 48000.0;
    constexpr int n = 4096;
    Model m;
    m.prepare (sr, n, 1);
    m.setDrive (0.0f);
    m.reset();
    std::vector<float> buf ((size_t) n), dry ((size_t) n);
    for (int i = 0; i < n; ++i)
        buf[(size_t) i] = dry[(size_t) i] = 0.4f * std::sin (2.0 * 3.14159265358979323846 * 440.0 * (double) i / sr);
    float* ch[] = { buf.data() };
    m.process (ch, 1, n);
    double err = 0.0, ref = 0.0;
    for (int i = 0; i < n; ++i)
    {
        const double e = (double) buf[(size_t) i] - (double) dry[(size_t) i];
        err += e * e;
        ref += (double) dry[(size_t) i] * (double) dry[(size_t) i];
    }
    const double rel = ref > 1.0e-18 ? err / ref : err;
    std::ostringstream oss;
    oss << name << " Drive=0 identity relErr=" << rel;
    if (rel < 1.0e-12)
        r.pass (oss.str());
    else
        r.fail (oss.str());
}

template <typename Model>
void checkFiniteAtDrive1 (FamilyCurvesReport& r, const char* name)
{
    constexpr double sr = 48000.0;
    constexpr int n = 4096;
    Model m;
    m.prepare (sr, n, 1);
    m.setDrive (1.0f);
    m.reset();
    const float peak = std::pow (10.0f, LevelReference::kReferenceRmsDb / 20.0f) * std::sqrt (2.0f);
    std::vector<float> buf ((size_t) n);
    for (int i = 0; i < n; ++i)
        buf[(size_t) i] = peak * std::sin (2.0 * 3.14159265358979323846 * 1000.0 * (double) i / sr);
    float* ch[] = { buf.data() };
    m.process (ch, 1, n);
    bool ok = true;
    for (float v : buf)
        if (! std::isfinite (v))
            ok = false;
    if (ok)
        r.pass (std::string (name) + " Drive=1 finite");
    else
        r.fail (std::string (name) + " Drive=1 NaN/Inf");
}

inline FamilyCurvesReport runFamilyCurvesVerifications()
{
    FamilyCurvesReport r;
    constexpr double sr = 48000.0;
    constexpr int n = 8192;
    constexpr float kPi = 3.14159265358979323846f;

    checkDrive0Identity<TapeModel> (r, "Tape");
    checkDrive0Identity<TransformerModel> (r, "Transformer");
    checkDrive0Identity<PreampModel> (r, "Preamp");

    checkFiniteAtDrive1<TapeModel> (r, "Tape");
    checkFiniteAtDrive1<TransformerModel> (r, "Transformer");
    checkFiniteAtDrive1<PreampModel> (r, "Preamp");

    // Tape: soft symmetric → odd > even at Drive 0.5 / −18
    {
        TapeModel m;
        m.prepare (sr, n, 1);
        m.setDrive (0.5f);
        m.reset();
        const float peak = std::pow (10.0f, LevelReference::kReferenceRmsDb / 20.0f) * std::sqrt (2.0f);
        std::vector<float> buf ((size_t) n);
        for (int i = 0; i < n; ++i)
            buf[(size_t) i] = peak * std::sin (2.0f * kPi * 1000.0f * (float) i / (float) sr);
        float* ch[] = { buf.data() };
        m.process (ch, 1, n);
        const double h2 = famGoertzel (buf.data() + 256, n - 256, 2000.0, sr);
        const double h3 = famGoertzel (buf.data() + 256, n - 256, 3000.0, sr);
        std::ostringstream oss;
        oss << "Tape H2=" << h2 << " H3=" << h3;
        if (h3 > h2)
            r.pass (oss.str());
        else
            r.fail (oss.str() + " (expected odd-leaning)");
    }

    // Transformer: even > Si diode even
    {
        auto h2At = [&] (auto& model) -> double
        {
            const float peak = std::pow (10.0f, LevelReference::kReferenceRmsDb / 20.0f) * std::sqrt (2.0f);
            std::vector<float> buf ((size_t) n);
            for (int i = 0; i < n; ++i)
                buf[(size_t) i] = peak * std::sin (2.0f * kPi * 1000.0f * (float) i / (float) sr);
            float* ch[] = { buf.data() };
            model.process (ch, 1, n);
            return famGoertzel (buf.data() + 256, n - 256, 2000.0, sr);
        };

        TransformerModel tr;
        tr.prepare (sr, n, 1);
        tr.setDrive (0.55f);
        tr.reset();
        const double h2tr = h2At (tr);

        DiodeModel di;
        di.prepare (sr, n, 1);
        di.setDrive (0.55f);
        di.setDiodeFlavor (DiodeFlavorIds::silicon);
        di.reset();
        const double h2di = h2At (di);

        std::ostringstream oss;
        oss << "Transformer H2=" << h2tr << " vs Si H2=" << h2di;
        if (h2tr > h2di * 2.0)
            r.pass (oss.str());
        else
            r.fail (oss.str());
    }

    // Preamp: Neve more even-leaning (H2/H3) than API punch
    {
        auto ratioAt = [&] (int flavor) -> std::pair<double, double>
        {
            PreampModel m;
            m.prepare (sr, n, 1);
            m.setPreampFlavor (flavor);
            m.setDrive (0.65f);
            m.reset();
            const float peak = std::pow (10.0f, LevelReference::kReferenceRmsDb / 20.0f) * std::sqrt (2.0f);
            std::vector<float> buf ((size_t) n);
            for (int i = 0; i < n; ++i)
                buf[(size_t) i] = peak * std::sin (2.0f * kPi * 1000.0f * (float) i / (float) sr);
            float* ch[] = { buf.data() };
            m.process (ch, 1, n);
            const double h2 = famGoertzel (buf.data() + 256, n - 256, 2000.0, sr);
            const double h3 = famGoertzel (buf.data() + 256, n - 256, 3000.0, sr);
            return { h2, h3 };
        };

        const auto [n2, n3] = ratioAt (PreampFlavorIds::neve1073);
        const auto [a2, a3] = ratioAt (PreampFlavorIds::api512);
        const double neveR = n3 > 0.0 ? n2 / n3 : 0.0;
        const double apiR = a3 > 0.0 ? a2 / a3 : 0.0;
        std::ostringstream oss;
        oss << "Preamp Neve H2/H3=" << neveR << " API H2/H3=" << apiR;
        if (neveR > apiR * 2.0 && a3 > 1.0)
            r.pass (oss.str());
        else
            r.fail (oss.str() + " (Neve should be more even-leaning)");
    }

    return r;
}
} // namespace verify
