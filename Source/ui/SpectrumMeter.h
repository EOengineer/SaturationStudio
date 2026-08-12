#pragma once

#include <JuceHeader.h>
#include "../dsp/SpectrumAnalyzer.h"
#include "HardwarePanelLook.h"
#include <vector>

/**
 * Log-frequency spectrum (20 Hz–20 kHz).
 * Resamples FFT bins onto a dense log grid so the left side isn't empty.
 * In-band region is brighter; heat strip shows saturation amount.
 */
class SpectrumMeter final : public juce::Component
{
public:
    SpectrumMeter() = default;

    void setAnalyzer (SpectrumAnalyzer* a) { analyzer = a; }
    void setBandRange (float lowHz, float highHz)
    {
        lowCutHz = lowHz;
        highCutHz = highHz;
    }

    void timerUpdate()
    {
        if (analyzer != nullptr)
        {
            analyzer->processPendingFFT();
            heat = analyzer->getHeat();
            rebuildDisplayPath();
        }
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (HardwarePanelLook::accentBrass().withAlpha (0.35f));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

        // Saturation heat — labeled so it doesn't read as a glitch
        constexpr float heatW = 14.0f;
        auto heatArea = bounds.removeFromRight (heatW).reduced (2.0f, 10.0f);
        auto plot = bounds.reduced (8.0f, 8.0f);
        plot.removeFromBottom (12.0f); // freq labels
        plot.removeFromLeft (28.0f);   // dB labels

        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (heatArea, 2.0f);
        const float hh = heatArea.getHeight() * juce::jlimit (0.0f, 1.0f, heat);
        g.setColour (juce::Colour::fromHSV (0.08f * (1.0f - heat), 0.85f, 0.9f, 0.95f));
        g.fillRect (heatArea.getX(), heatArea.getBottom() - hh, heatArea.getWidth(), hh);
        g.setColour (HardwarePanelLook::engraving().withAlpha (0.5f));
        g.setFont (juce::Font (juce::FontOptions (8.0f)));
        g.drawText ("SAT",
                    juce::Rectangle<float> (heatArea.getX() - 2.0f, bounds.getBottom() - 14.0f, heatW + 4.0f, 10.0f),
                    juce::Justification::centred, false);

        if (analyzer == nullptr || plot.getWidth() < 4.0f)
            return;

        const float minDb = -80.0f;
        const float maxDb = 0.0f;

        auto yForDb = [&] (float db) -> float
        {
            db = juce::jlimit (minDb, maxDb, db);
            return plot.getBottom() - ((db - minDb) / (maxDb - minDb)) * plot.getHeight();
        };

        auto xForHz = [&] (float hz) -> float
        {
            hz = juce::jlimit (minHz, maxHz, hz);
            const float t = std::log (hz / minHz) / std::log (maxHz / minHz);
            return plot.getX() + t * plot.getWidth();
        };

        // dB grid
        g.setColour (HardwarePanelLook::engraving().withAlpha (0.10f));
        for (float db : { -20.0f, -40.0f, -60.0f })
        {
            const float y = yForDb (db);
            g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        }
        g.setColour (HardwarePanelLook::engraving().withAlpha (0.35f));
        g.setFont (juce::Font (juce::FontOptions (9.0f)));
        for (float db : { 0.0f, -20.0f, -40.0f, -60.0f })
        {
            const float y = yForDb (db);
            g.drawText (juce::String ((int) db),
                        juce::Rectangle<float> (bounds.getX() + 2.0f, y - 6.0f, 26.0f, 12.0f),
                        juce::Justification::centredRight, false);
        }

        // Classic analyzer markers: 20 / 50 / 100 / 200 / 500 / 1k / 2k / 5k / 10k / 20k
        struct FreqMark { float hz; const char* label; bool major; };
        const FreqMark marks[] = {
            { 20.0f,    "20",  true  },
            { 30.0f,    nullptr, false },
            { 40.0f,    nullptr, false },
            { 50.0f,    "50",  true  },
            { 60.0f,    nullptr, false },
            { 70.0f,    nullptr, false },
            { 80.0f,    nullptr, false },
            { 90.0f,    nullptr, false },
            { 100.0f,   "100", true  },
            { 200.0f,   "200", true  },
            { 300.0f,   nullptr, false },
            { 400.0f,   nullptr, false },
            { 500.0f,   "500", true  },
            { 600.0f,   nullptr, false },
            { 700.0f,   nullptr, false },
            { 800.0f,   nullptr, false },
            { 900.0f,   nullptr, false },
            { 1000.0f,  "1k",  true  },
            { 2000.0f,  "2k",  true  },
            { 3000.0f,  nullptr, false },
            { 4000.0f,  nullptr, false },
            { 5000.0f,  "5k",  true  },
            { 6000.0f,  nullptr, false },
            { 7000.0f,  nullptr, false },
            { 8000.0f,  nullptr, false },
            { 9000.0f,  nullptr, false },
            { 10000.0f, "10k", true  },
            { 20000.0f, "20k", true  },
        };

        for (const auto& m : marks)
        {
            if (m.hz < minHz || m.hz > maxHz + 0.5f)
                continue;
            const float x = xForHz (m.hz);
            if (m.major)
            {
                g.setColour (HardwarePanelLook::engraving().withAlpha (0.22f));
                g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
            }
            else
            {
                g.setColour (HardwarePanelLook::engraving().withAlpha (0.08f));
                g.drawVerticalLine ((int) x, plot.getBottom() - plot.getHeight() * 0.12f, plot.getBottom());
            }
        }

        // Band highlight
        {
            const float x0 = xForHz (lowCutHz);
            const float x1 = xForHz (highCutHz);
            g.setColour (HardwarePanelLook::accentBrass().withAlpha (0.12f));
            g.fillRect (x0, plot.getY(), juce::jmax (1.0f, x1 - x0), plot.getHeight());
            g.setColour (HardwarePanelLook::accentBrass().withAlpha (0.45f));
            g.drawVerticalLine ((int) x0, plot.getY(), plot.getBottom());
            g.drawVerticalLine ((int) x1, plot.getY(), plot.getBottom());
        }

        if (displayDb.empty())
            return;

        // Full-width fill + stroke from resampled log points
        juce::Path fill, line;
        const int n = (int) displayDb.size();
        for (int i = 0; i < n; ++i)
        {
            const float t = (n == 1) ? 0.0f : (float) i / (float) (n - 1);
            const float hz = minHz * std::pow (maxHz / minHz, t);
            const float x = xForHz (hz);
            const float y = yForDb (displayDb[(size_t) i]);

            if (i == 0)
            {
                fill.startNewSubPath (x, plot.getBottom());
                fill.lineTo (x, y);
                line.startNewSubPath (x, y);
            }
            else
            {
                fill.lineTo (x, y);
                line.lineTo (x, y);
            }
        }

        fill.lineTo (xForHz (maxHz), plot.getBottom());
        fill.closeSubPath();

        g.setColour (HardwarePanelLook::engraving().withAlpha (0.18f));
        g.fillPath (fill);

        // Brighter in-band fill
        g.saveState();
        g.reduceClipRegion (juce::Rectangle<int> ((int) xForHz (lowCutHz),
                                                  (int) plot.getY(),
                                                  (int) juce::jmax (1.0f, xForHz (highCutHz) - xForHz (lowCutHz)),
                                                  (int) plot.getHeight()));
        g.setColour (HardwarePanelLook::accentBrass().withAlpha (0.28f));
        g.fillPath (fill);
        g.setColour (HardwarePanelLook::engraving().withAlpha (0.95f));
        g.strokePath (line, juce::PathStrokeType (1.8f));
        g.restoreState();

        g.setColour (HardwarePanelLook::engraving().withAlpha (0.45f));
        g.strokePath (line, juce::PathStrokeType (1.0f));

        // Frequency labels under major marks
        g.setColour (HardwarePanelLook::engraving().withAlpha (0.65f));
        g.setFont (juce::Font (juce::FontOptions (9.0f)));
        const float labelY = bounds.getBottom() - 12.0f;
        for (const auto& m : marks)
        {
            if (! m.major || m.label == nullptr || m.hz < minHz || m.hz > maxHz + 0.5f)
                continue;
            const float x = xForHz (m.hz);
            juce::Justification just = juce::Justification::centred;
            float w = 28.0f;
            float left = x - w * 0.5f;
            if (m.hz <= minHz + 0.1f)
            {
                just = juce::Justification::centredLeft;
                left = plot.getX();
                w = 28.0f;
            }
            else if (m.hz >= maxHz - 0.1f)
            {
                just = juce::Justification::centredRight;
                left = plot.getRight() - 32.0f;
                w = 32.0f;
            }
            g.drawText (m.label, juce::Rectangle<float> (left, labelY, w, 12.0f), just, false);
        }
    }

    void resized() override
    {
        rebuildDisplayPath();
    }

private:
    void rebuildDisplayPath()
    {
        displayDb.clear();
        if (analyzer == nullptr)
            return;

        const double sr = analyzer->getSampleRate();
        if (sr <= 0.0)
            return;

        maxHz = juce::jmin (20000.0f, (float) (sr * 0.5));

        // ~1 point per pixel (cap so we stay cheap)
        const int plotW = juce::jmax (64, getWidth() - 50);
        const int numPoints = juce::jlimit (128, 768, plotW);
        displayDb.resize ((size_t) numPoints, SpectrumAnalyzer::kFloorDb);

        const float binHz = (float) (sr / (double) SpectrumAnalyzer::kFftSize);

        for (int i = 0; i < numPoints; ++i)
        {
            const float t0 = (float) i / (float) numPoints;
            const float t1 = (float) (i + 1) / (float) numPoints;
            const float hz0 = minHz * std::pow (maxHz / minHz, t0);
            const float hz1 = minHz * std::pow (maxHz / minHz, t1);
            displayDb[(size_t) i] = analyzer->peakDbInRange (hz0, juce::jmax (hz1, hz0 + binHz * 0.5f));
        }

        // Light spatial smooth — kills stair-steps without smearing peaks much
        if (numPoints >= 5)
        {
            std::vector<float> tmp = displayDb;
            for (int i = 2; i < numPoints - 2; ++i)
            {
                displayDb[(size_t) i] =
                    0.06f * tmp[(size_t) (i - 2)]
                  + 0.24f * tmp[(size_t) (i - 1)]
                  + 0.40f * tmp[(size_t) i]
                  + 0.24f * tmp[(size_t) (i + 1)]
                  + 0.06f * tmp[(size_t) (i + 2)];
            }
        }
    }

    SpectrumAnalyzer* analyzer = nullptr;
    std::vector<float> displayDb;
    float lowCutHz = 20.0f;
    float highCutHz = 20000.0f;
    float heat = 0.0f;
    float minHz = 20.0f;
    float maxHz = 20000.0f;
};
