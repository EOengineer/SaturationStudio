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
    static constexpr int kFftOrder = 11;
    static constexpr int kFftSize = 1 << kFftOrder; // 2048
    static constexpr int kNumBins = kFftSize / 2;

    SpectrumAnalyzer()
        : fft (kFftOrder),
          window (kFftSize, juce::dsp::WindowingFunction<float>::hann, true)
    {
        fifo.assign ((size_t) kFftSize, 0.0f);
        pendingFftData.assign ((size_t) kFftSize * 2, 0.0f);
        fftData.assign ((size_t) kFftSize * 2, 0.0f);
        magnitudes.assign ((size_t) kNumBins, 0.0f);
        magnitudesSmooth.assign ((size_t) kNumBins, 0.0f);
    }

    void prepare (double sampleRateIn)
    {
        sampleRate = sampleRateIn > 0.0 ? sampleRateIn : 48000.0;
        fifoIndex = 0;
        fftReady.store (false, std::memory_order_relaxed);
        std::fill (magnitudesSmooth.begin(), magnitudesSmooth.end(), 0.0f);
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
                // Only overwrite pending buffer if UI has consumed the previous one
                if (! fftReady.load (std::memory_order_acquire))
                {
                    std::copy (fifo.begin(), fifo.end(), pendingFftData.begin());
                    fftReady.store (true, std::memory_order_release);
                }
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

        const float decay = 0.85f;
        const float attack = 0.35f;
        for (int i = 0; i < kNumBins; ++i)
        {
            const float mag = fftData[(size_t) i];
            const float db = juce::Decibels::gainToDecibels (mag / (float) kFftSize, -100.0f);
            magnitudes[(size_t) i] = db;
            const float prev = magnitudesSmooth[(size_t) i];
            const float coeff = db > prev ? attack : decay;
            magnitudesSmooth[(size_t) i] = prev + coeff * (db - prev);
        }
    }

    void getMagnitudesDb (std::vector<float>& out) const
    {
        out.resize ((size_t) kNumBins);
        for (int i = 0; i < kNumBins; ++i)
            out[(size_t) i] = magnitudesSmooth[(size_t) i];
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
    std::atomic<float> heat { 0.0f };
};
