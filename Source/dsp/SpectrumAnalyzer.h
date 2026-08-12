#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <cmath>
#include <vector>

/**
 * Spectrum snapshot for UI metering (20 Hz–20 kHz).
 * Audio thread only fills a FIFO; UI timer runs the FFT.
 */
class SpectrumAnalyzer
{
public:
    static constexpr int kFftOrder = 12;
    static constexpr int kFftSize = 1 << kFftOrder; // 4096 — smoother lows, still cheap on UI thread
    static constexpr int kNumBins = kFftSize / 2;
    static constexpr float kFloorDb = -90.0f;

    SpectrumAnalyzer()
        : fft (kFftOrder),
          window (kFftSize, juce::dsp::WindowingFunction<float>::hann, true)
    {
        fifo.assign ((size_t) kFftSize, 0.0f);
        pendingFftData.assign ((size_t) kFftSize * 2, 0.0f);
        fftData.assign ((size_t) kFftSize * 2, 0.0f);
        magnitudes.assign ((size_t) kNumBins, kFloorDb);
        magnitudesSmooth.assign ((size_t) kNumBins, kFloorDb);

        // Hann coherent gain ≈ 0.5 → compensate so 0 dBFS sine ≈ 0 dB on the meter
        windowCompensation = 2.0f;
    }

    void prepare (double sampleRateIn)
    {
        sampleRate = sampleRateIn > 0.0 ? sampleRateIn : 48000.0;
        fifoIndex = 0;
        fftReady.store (false, std::memory_order_relaxed);
        std::fill (magnitudes.begin(), magnitudes.end(), kFloorDb);
        std::fill (magnitudesSmooth.begin(), magnitudesSmooth.end(), kFloorDb);
    }

    /** Audio thread: push samples only — no FFT. */
    void pushBlock (const juce::AudioBuffer<float>& buffer)
    {
        const int numCh = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        if (numCh <= 0 || numSamples <= 0)
            return;

        for (int i = 0; i < numSamples; ++i)
        {
            float s = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
                s += buffer.getSample (ch, i);
            s /= (float) numCh;

            fifo[(size_t) fifoIndex] = s;
            if (++fifoIndex == kFftSize)
            {
                fifoIndex = 0;
                // Always keep the newest frame for the UI (drop if UI is behind)
                std::copy (fifo.begin(), fifo.end(), pendingFftData.begin());
                fftReady.store (true, std::memory_order_release);
            }
        }
    }

    /** UI / message thread: run pending FFT if ready. */
    void processPendingFFT()
    {
        if (! fftReady.exchange (false, std::memory_order_acq_rel))
            return;

        std::copy (pendingFftData.begin(), pendingFftData.begin() + kFftSize, fftData.begin());
        std::fill (fftData.begin() + kFftSize, fftData.end(), 0.0f);

        window.multiplyWithWindowingTable (fftData.data(), (size_t) kFftSize);
        fft.performFrequencyOnlyForwardTransform (fftData.data());

        const float norm = windowCompensation / (float) kFftSize;
        const float decay = 0.78f;
        const float attack = 0.45f;

        for (int i = 0; i < kNumBins; ++i)
        {
            const float mag = fftData[(size_t) i] * norm;
            const float db = juce::Decibels::gainToDecibels (mag, kFloorDb);
            magnitudes[(size_t) i] = db;
            const float prev = magnitudesSmooth[(size_t) i];
            const float coeff = db > prev ? attack : decay;
            magnitudesSmooth[(size_t) i] = prev + coeff * (db - prev);
        }
    }

    void getMagnitudesDb (std::vector<float>& out) const
    {
        out = magnitudesSmooth;
    }

    /** Linear-interpolated smoothed dB at an arbitrary frequency. */
    float dbAtHz (float hz) const noexcept
    {
        if (sampleRate <= 0.0)
            return kFloorDb;

        const float binHz = (float) (sampleRate / (double) kFftSize);
        const float bin = juce::jlimit (1.0f, (float) (kNumBins - 1) - 0.001f, hz / binHz);
        const int i0 = (int) bin;
        const float frac = bin - (float) i0;
        const float a = magnitudesSmooth[(size_t) i0];
        const float b = magnitudesSmooth[(size_t) juce::jmin (i0 + 1, kNumBins - 1)];
        return a + frac * (b - a);
    }

    /**
     * Peak in [hzLo, hzHi] via several interpolated samples (smoother than raw bin max).
     */
    float peakDbInRange (float hzLo, float hzHi) const noexcept
    {
        hzLo = juce::jmax (1.0f, hzLo);
        hzHi = juce::jmax (hzLo, hzHi);

        float peak = kFloorDb;
        constexpr int kSubs = 6;
        for (int s = 0; s < kSubs; ++s)
        {
            const float t = (kSubs == 1) ? 0.0f : (float) s / (float) (kSubs - 1);
            const float hz = hzLo * std::pow (hzHi / hzLo, t);
            peak = juce::jmax (peak, dbAtHz (hz));
        }
        return peak;
    }

    float binFrequency (int bin) const noexcept
    {
        return (float) (bin * sampleRate / (double) kFftSize);
    }

    double getSampleRate() const noexcept { return sampleRate; }

    void setHeat (float heat01) noexcept
    {
        heat.store (juce::jlimit (0.0f, 1.0f, heat01), std::memory_order_relaxed);
    }

    float getHeat() const noexcept { return heat.load (std::memory_order_relaxed); }

private:
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::vector<float> fifo;
    std::vector<float> pendingFftData;
    std::vector<float> fftData;
    std::vector<float> magnitudes;
    std::vector<float> magnitudesSmooth;
    int fifoIndex = 0;
    std::atomic<bool> fftReady { false };
    double sampleRate = 48000.0;
    float windowCompensation = 2.0f;
    std::atomic<float> heat { 0.0f };
};
