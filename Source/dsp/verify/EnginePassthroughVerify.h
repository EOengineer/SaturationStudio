#pragma once

#include "../LinearPhaseBandSplit.h"
#include "../models/DiodeModel.h"
#include "../models/ModelRegistry.h"
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace verify
{
struct EngineReport
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

/**
 * Offline engine checks without JUCE oversampling:
 * registry, diode passthrough through band split (wide open) ≈ delayed input.
 */
inline EngineReport runEnginePassthroughVerifications()
{
    EngineReport r;

    std::string reg;
    if (ModelRegistry::sanityCheck (reg))
        r.pass ("registry sanity");
    else
    {
        r.fail ("registry sanity");
        r.text += reg;
    }

    constexpr double sr = 48000.0;
    constexpr int n = 8192;

    LinearPhaseBandSplit split;
    split.prepare (sr, n, 1);
    split.setCutoffs (20.0f, 20000.0f);
    split.reset();

    DiodeModel diode;
    diode.prepare (sr, n, 1);
    diode.setDrive (0.0f); // identity bypass
    diode.reset();

    std::vector<float> in ((size_t) n, 0.0f);
    std::vector<float> mid ((size_t) n, 0.0f);
    std::vector<float> side ((size_t) n, 0.0f);
    std::vector<float> delayed ((size_t) n, 0.0f);

    in[200] = 1.0f;
    for (int i = 0; i < n; ++i)
        in[(size_t) i] += 0.1f * std::sin (2.0 * fir::kPi * 1000.0 * (double) i / sr);

    fir::DelayLine dly;
    dly.setDelay (split.getLatencySamples());
    for (int i = 0; i < n; ++i)
        delayed[(size_t) i] = dly.processSample (in[(size_t) i]);

    const float* inPtr = in.data();
    float* midPtr = mid.data();
    float* sidePtr = side.data();
    split.process (&inPtr, &midPtr, &sidePtr, n);

    float* chans[] = { mid.data() };
    diode.process (chans, 1, n); // identity

    const int skip = split.getLatencySamples() * 2 + 64;
    double err = 0.0, ref = 0.0;
    for (int i = skip; i < n; ++i)
    {
        const float y = mid[(size_t) i] + side[(size_t) i];
        const float e = y - delayed[(size_t) i];
        err += (double) e * (double) e;
        ref += (double) delayed[(size_t) i] * (double) delayed[(size_t) i];
    }
    const double rel = ref > 1.0e-12 ? err / ref : err;
    std::ostringstream oss;
    oss << "diode Drive=0 passthrough (no OS) relErr=" << rel;
    if (rel < 1.0e-4)
        r.pass (oss.str());
    else
        r.fail (oss.str());

    // Default model factory
    auto m = ModelRegistry::createById (ModelRegistry::defaultFamilyId());
    if (m != nullptr && std::string (m->getId()) == ModelIds::diode)
        r.pass ("default factory creates Diode");
    else
        r.fail ("default factory did not create Diode");

    return r;
}
} // namespace verify
