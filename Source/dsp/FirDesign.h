#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

/**
 * Pure C++ Kaiser-windowed sinc FIR design (no JUCE).
 * Used by LinearPhaseBandSplit and offline verifies.
 */
namespace fir
{
inline constexpr int kDefaultNumTaps = 1023; // odd → integer group delay (N-1)/2
inline constexpr double kPi = 3.14159265358979323846;

inline double besselI0 (double x) noexcept
{
    double sum = 1.0;
    double term = 1.0;
    const double x2 = x * x * 0.25;
    for (int k = 1; k < 32; ++k)
    {
        term *= x2 / (double) (k * k);
        sum += term;
        if (term < 1.0e-12 * sum)
            break;
    }
    return sum;
}

inline void kaiserWindow (std::vector<double>& w, double beta) noexcept
{
    const int n = (int) w.size();
    if (n <= 1)
        return;

    const double denom = besselI0 (beta);
    const double m = (double) (n - 1);
    for (int i = 0; i < n; ++i)
    {
        const double r = (2.0 * (double) i / m) - 1.0;
        const double a = 1.0 - r * r;
        w[(size_t) i] = besselI0 (beta * std::sqrt (std::max (0.0, a))) / denom;
    }
}

/** Ideal low-pass impulse (linear phase, odd length), then Kaiser window + normalize. */
inline std::vector<float> designLowpass (int numTaps, double sampleRate, double cutoffHz, double beta = 8.0)
{
    if ((numTaps & 1) == 0)
        ++numTaps;

    std::vector<double> h ((size_t) numTaps, 0.0);
    std::vector<double> w ((size_t) numTaps, 0.0);
    kaiserWindow (w, beta);

    const double fc = std::clamp (cutoffHz / sampleRate, 1.0e-6, 0.499);
    const int mid = (numTaps - 1) / 2;

    for (int i = 0; i < numTaps; ++i)
    {
        const int k = i - mid;
        const double sinc = (k == 0) ? (2.0 * fc)
                                     : std::sin (2.0 * kPi * fc * (double) k) / (kPi * (double) k);
        h[(size_t) i] = sinc * w[(size_t) i];
    }

    double sum = 0.0;
    for (double v : h)
        sum += v;
    if (std::abs (sum) > 1.0e-18)
        for (double& v : h)
            v /= sum;

    std::vector<float> out ((size_t) numTaps);
    for (int i = 0; i < numTaps; ++i)
        out[(size_t) i] = (float) h[(size_t) i];
    return out;
}

/** High-pass via spectral inversion of a low-pass at the same cutoff. */
inline std::vector<float> designHighpass (int numTaps, double sampleRate, double cutoffHz, double beta = 8.0)
{
    auto h = designLowpass (numTaps, sampleRate, cutoffHz, beta);
    const int mid = ((int) h.size() - 1) / 2;
    for (float& v : h)
        v = -v;
    h[(size_t) mid] += 1.0f;
    return h;
}

/** Direct-form FIR (linear phase when coeffs are symmetric). */
class FirFilter
{
public:
    void setCoefficients (const std::vector<float>& coeffs)
    {
        taps = coeffs;
        history.assign (taps.size(), 0.0f);
        write = 0;
    }

    void reset() noexcept
    {
        std::fill (history.begin(), history.end(), 0.0f);
        write = 0;
    }

    int getNumTaps() const noexcept { return (int) taps.size(); }

    int getLatencySamples() const noexcept
    {
        const int n = getNumTaps();
        return n > 0 ? (n - 1) / 2 : 0;
    }

    float processSample (float x) noexcept
    {
        if (taps.empty())
            return x;

        history[(size_t) write] = x;
        float y = 0.0f;
        int idx = write;
        for (float c : taps)
        {
            y += c * history[(size_t) idx];
            if (--idx < 0)
                idx = (int) history.size() - 1;
        }
        if (++write >= (int) history.size())
            write = 0;
        return y;
    }

    void process (float* data, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            data[i] = processSample (data[i]);
    }

private:
    std::vector<float> taps;
    std::vector<float> history;
    int write = 0;
};

/** Integer-sample delay for latency matching. */
class DelayLine
{
public:
    void setDelay (int samples)
    {
        delaySamples = std::max (0, samples);
        buffer.assign ((size_t) delaySamples + 1, 0.0f);
        write = 0;
    }

    void reset() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        write = 0;
    }

    int getDelay() const noexcept { return delaySamples; }

    float processSample (float x) noexcept
    {
        if (delaySamples <= 0)
            return x;

        buffer[(size_t) write] = x;
        int read = write - delaySamples;
        if (read < 0)
            read += (int) buffer.size();
        const float y = buffer[(size_t) read];
        if (++write >= (int) buffer.size())
            write = 0;
        return y;
    }

private:
    std::vector<float> buffer;
    int delaySamples = 0;
    int write = 0;
};
} // namespace fir
