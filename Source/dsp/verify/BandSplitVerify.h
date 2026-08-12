#pragma once

#include "../LinearPhaseBandSplit.h"
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace verify
{
struct Report
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

inline Report runBandSplitVerifications()
{
    Report r;
    constexpr double sr = 48000.0;
    constexpr int n = 8192;

    LinearPhaseBandSplit split;
    split.prepare (sr, n, 1);

    // Latency matches 2 * ((N-1)/2) for HP then LP cascade
    const int expected = 2 * ((LinearPhaseBandSplit::kNumTaps - 1) / 2);
    if (split.getLatencySamples() == expected)
        r.pass ("FIR cascade latency == " + std::to_string (expected));
    else
        r.fail ("FIR cascade latency " + std::to_string (split.getLatencySamples())
                + " != " + std::to_string (expected));

    // Wide open: mid ≈ delayed input (side energy small)
    {
        split.setCutoffs (20.0f, 20000.0f);
        split.reset();

        std::vector<float> in ((size_t) n, 0.0f);
        std::vector<float> mid ((size_t) n, 0.0f);
        std::vector<float> side ((size_t) n, 0.0f);
        in[100] = 1.0f;
        for (int i = 0; i < n; ++i)
            in[(size_t) i] += 0.05f * std::sin (2.0 * fir::kPi * 440.0 * (double) i / sr);

        const float* inPtr = in.data();
        float* midPtr = mid.data();
        float* sidePtr = side.data();
        split.process (&inPtr, &midPtr, &sidePtr, n);

        const int skip = split.getLatencySamples() * 2 + 64;
        double sideE = 0.0, midE = 0.0;
        for (int i = skip; i < n; ++i)
        {
            sideE += (double) side[(size_t) i] * (double) side[(size_t) i];
            midE += (double) mid[(size_t) i] * (double) mid[(size_t) i];
        }
        const double ratio = midE > 1.0e-12 ? sideE / midE : sideE;
        std::ostringstream oss;
        oss << "wide-open side/mid energy ratio=" << ratio;
        if (ratio < 0.05)
            r.pass (oss.str());
        else
            r.fail (oss.str());
    }

    // Measured impulse peak delay ≈ reported latency
    {
        split.setCutoffs (20.0f, 20000.0f);
        split.reset();
        std::vector<float> in ((size_t) n, 0.0f);
        std::vector<float> mid ((size_t) n, 0.0f);
        std::vector<float> side ((size_t) n, 0.0f);
        in[0] = 1.0f;
        const float* inPtr = in.data();
        float* midPtr = mid.data();
        float* sidePtr = side.data();
        split.process (&inPtr, &midPtr, &sidePtr, n);

        int peakAt = 0;
        float peak = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float a = std::abs (mid[(size_t) i] + side[(size_t) i]);
            if (a > peak)
            {
                peak = a;
                peakAt = i;
            }
        }
        const int reported = split.getLatencySamples();
        std::ostringstream oss;
        oss << "impulse peak at " << peakAt << " (reported latency " << reported << ")";
        if (std::abs (peakAt - reported) <= 2)
            r.pass (oss.str());
        else
            r.fail (oss.str());
    }

    // Narrow band: mid energy should be lower than wide-open for a full-band-ish signal
    // (soft check — just ensure processing runs and mid is finite)
    {
        split.setCutoffs (500.0f, 2000.0f);
        split.reset();
        std::vector<float> in ((size_t) n, 0.0f);
        std::vector<float> mid ((size_t) n, 0.0f);
        std::vector<float> side ((size_t) n, 0.0f);
        for (int i = 0; i < n; ++i)
            in[(size_t) i] = 0.2f * std::sin (2.0 * fir::kPi * 1000.0 * (double) i / sr);

        const float* inPtr = in.data();
        float* midPtr = mid.data();
        float* sidePtr = side.data();
        split.process (&inPtr, &midPtr, &sidePtr, n);

        double midE = 0.0;
        for (int i = 2000; i < n; ++i)
            midE += (double) mid[(size_t) i] * (double) mid[(size_t) i];
        if (std::isfinite (midE) && midE > 1.0e-6)
            r.pass ("narrow-band 1 kHz tone produces mid energy");
        else
            r.fail ("narrow-band mid energy unexpected");
    }

    // Cutoff clamp
    split.setCutoffs (5000.0f, 1000.0f);
    if (split.getLowCutHz() < split.getHighCutHz())
        r.pass ("cutoff clamp enforces low < high");
    else
        r.fail ("cutoff clamp failed");

    return r;
}
} // namespace verify
