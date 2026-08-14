#pragma once

#include "../FirDesign.h"
#include "../LevelReference.h"
#include "../models/TubeModel.h"
#include "../../util/ParamIDs.h"
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace verify
{
/**
 * Offline tube aliasing metrics (no JUCE).
 *
 * Proxy for the engine OS island: analytic sine @ F·fs → TubeModel (AX7) → Kaiser LPF
 * → decimate to base rate. Not identical to juce::dsp::Oversampling equiripple
 * half-bands; directionally comparable. Plugin Doctor remains host ground truth.
 *
 * Mirrors DiodeAliasingVerify for the Newton TriodeStage path.
 */
struct TubeAliasingReport
{
    bool ok = true;
    std::string text;
    void pass (const std::string& msg) { text += "PASS: " + msg + "\n"; }
    void fail (const std::string& msg)
    {
        ok = false;
        text += "FAIL: " + msg + "\n";
    }
    void info (const std::string& msg) { text += "INFO: " + msg + "\n"; }
};

inline double tubeAliasingGoertzelPower (const float* x, int n, double freqHz, double sr)
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

struct TubeAliasingMetrics
{
    double harmTotalRatio = 0.0;
    double residualDb = 0.0;
    bool finite = true;
};

inline TubeAliasingMetrics measureTubeHarmonicRatio (const std::vector<float>& y, double baseFs, double f0, int skip)
{
    TubeAliasingMetrics m;
    const int n = (int) y.size();
    if (n <= skip + 16)
    {
        m.finite = false;
        return m;
    }

    for (int i = skip; i < n; ++i)
    {
        if (! std::isfinite (y[(size_t) i]))
        {
            m.finite = false;
            return m;
        }
    }

    const float* x = y.data() + skip;
    const int nn = n - skip;

    double harm = 0.0;
    const double nyquist = 0.5 * baseFs;
    for (int h = 1; h <= 8; ++h)
    {
        const double fh = f0 * (double) h;
        if (fh >= nyquist * 0.98)
            break;
        harm += tubeAliasingGoertzelPower (x, nn, fh, baseFs);
    }

    double total = 0.0;
    for (int i = 0; i < nn; ++i)
        total += (double) x[i] * (double) x[i];
    total = std::max (total, 1.0e-30);

    // Goertzel |Y|^2 ≈ (N A/2)^2 for a tone; time energy ≈ N A^2/2 → scale 2/N.
    const double harmE = harm * (2.0 / (double) nn);
    m.harmTotalRatio = std::min (1.0, harmE / total);
    const double residual = std::max (0.0, 1.0 - m.harmTotalRatio);
    m.residualDb = 10.0 * std::log10 (std::max (residual, 1.0e-30));
    return m;
}

/** Process AX7 tube at F·fs with bandlimited downsample to baseFs. */
inline std::vector<float> runTubeOsProxy (double baseFs,
                                          int baseN,
                                          int factor,
                                          float drive,
                                          double f0Hz)
{
    factor = std::max (1, factor);
    const double osFs = baseFs * (double) factor;
    const int osN = baseN * factor;

    const float rmsLin = std::pow (10.0f, LevelReference::kReferenceRmsDb / 20.0f);
    const float peak = rmsLin * std::sqrt (2.0f);
    constexpr float kPi = 3.14159265358979323846f;

    std::vector<float> osBuf ((size_t) osN);
    for (int i = 0; i < osN; ++i)
        osBuf[(size_t) i] = peak * std::sin (2.0f * kPi * (float) f0Hz * (float) i / (float) osFs);

    TubeModel m;
    m.prepare (osFs, osN, 1);
    m.setDrive (drive);
    m.setTubeFlavor (TubeFlavorIds::ax7);
    m.reset();

    float* chans[] = { osBuf.data() };
    m.process (chans, 1, osN);

    if (factor == 1)
        return osBuf;

    // Anti-image LPF before decimate (cutoff just below base Nyquist).
    const auto taps = fir::designLowpass (255, osFs, 0.45 * baseFs, 8.0);
    fir::FirFilter lpf;
    lpf.setCoefficients (taps);
    lpf.reset();
    lpf.process (osBuf.data(), osN);

    const int latency = lpf.getLatencySamples();
    std::vector<float> out ((size_t) baseN, 0.0f);
    for (int i = 0; i < baseN; ++i)
    {
        const int src = i * factor + latency;
        if (src < osN)
            out[(size_t) i] = osBuf[(size_t) src];
    }
    return out;
}

inline TubeAliasingReport runTubeAliasingVerifications()
{
    TubeAliasingReport r;
    constexpr double baseFs = 48000.0;
    constexpr int baseN = 8192;
    constexpr double f0 = 5000.0;
    constexpr int skip = 512;

    r.info ("Tube/AX7 aliasing proxy (analytic OS sine → TubeModel → Kaiser LPF → decimate)");
    r.info ("Not identical to JUCE FIR half-band OS; use Plugin Doctor for host sweeps");

    auto runCase = [&] (float drive, bool hardGate) {
        const auto y1 = runTubeOsProxy (baseFs, baseN, 1, drive, f0);
        const auto y4 = runTubeOsProxy (baseFs, baseN, 4, drive, f0);
        const auto m1 = measureTubeHarmonicRatio (y1, baseFs, f0, skip);
        const auto m4 = measureTubeHarmonicRatio (y4, baseFs, f0, skip);

        std::ostringstream oss;
        oss << "Drive=" << drive << " f0=" << f0 << "Hz"
            << " 1x ratio=" << m1.harmTotalRatio << " residualDb=" << m1.residualDb
            << " | 4x ratio=" << m4.harmTotalRatio << " residualDb=" << m4.residualDb;

        if (! m1.finite || ! m4.finite)
        {
            r.fail (oss.str() + " (NaN/Inf)");
            return;
        }

        if (hardGate)
        {
            // 4× must not be meaningfully worse than 1× (regression gate).
            if (m4.harmTotalRatio >= m1.harmTotalRatio * 0.95)
                r.pass (oss.str());
            else
                r.fail (oss.str() + " (4x worse than 1x)");
        }
        else
        {
            r.info (oss.str() + " (abuse report only)");
        }
    };

    runCase (0.5f, true);  // plugin default
    runCase (1.0f, false); // abuse — numbers only

    return r;
}
} // namespace verify
