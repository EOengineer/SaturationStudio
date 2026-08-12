#pragma once

#include <JuceHeader.h>
#include "../dsp/SpectrumAnalyzer.h"
#include "HardwarePanelLook.h"
#include <vector>

/** Log-frequency spectrum with band highlight + saturation heat indicator. */
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
            analyzer->getMagnitudesDb (mags);
            heat = analyzer->getHeat();
        }
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (HardwarePanelLook::accentBrass().withAlpha (0.35f));
        g.drawRoundedRectangle (r, 4.0f, 1.0f);

        if (mags.empty() || analyzer == nullptr)
            return;

        const double sr = analyzer->getSampleRate();
        const float minDb = -90.0f;
        const float maxDb = 0.0f;
        const float minHz = 20.0f;
        const float maxHz = 20000.0f;

        auto xForHz = [&] (float hz) -> float
        {
            hz = juce::jlimit (minHz, maxHz, hz);
            const float t = std::log (hz / minHz) / std::log (maxHz / minHz);
            return r.getX() + t * r.getWidth();
        };

        // Band highlight
        const float x0 = xForHz (lowCutHz);
        const float x1 = xForHz (highCutHz);
        g.setColour (HardwarePanelLook::accentBrass().withAlpha (0.12f));
        g.fillRect (x0, r.getY(), juce::jmax (1.0f, x1 - x0), r.getHeight());

        juce::Path inBand, outBand;
        bool startedIn = false, startedOut = false;

        for (int i = 1; i < (int) mags.size(); ++i)
        {
            const float hz = (float) (i * sr / (double) SpectrumAnalyzer::kFftSize);
            if (hz < minHz || hz > maxHz)
                continue;

            const float x = xForHz (hz);
            const float db = juce::jlimit (minDb, maxDb, mags[(size_t) i]);
            const float y = r.getBottom() - ((db - minDb) / (maxDb - minDb)) * r.getHeight();
            const bool inside = hz >= lowCutHz && hz <= highCutHz;

            if (inside)
            {
                if (! startedIn) { inBand.startNewSubPath (x, y); startedIn = true; }
                else inBand.lineTo (x, y);
            }
            else
            {
                if (! startedOut) { outBand.startNewSubPath (x, y); startedOut = true; }
                else outBand.lineTo (x, y);
            }
        }

        g.setColour (HardwarePanelLook::engraving().withAlpha (0.35f));
        g.strokePath (outBand, juce::PathStrokeType (1.0f));
        g.setColour (HardwarePanelLook::engraving().withAlpha (0.95f));
        g.strokePath (inBand, juce::PathStrokeType (1.6f));

        // Heat indicator (right edge)
        auto heatArea = r.removeFromRight (8.0f).reduced (1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRect (heatArea);
        const float hh = heatArea.getHeight() * heat;
        g.setColour (juce::Colour::fromHSV (0.08f * (1.0f - heat), 0.85f, 0.9f, 0.9f));
        g.fillRect (heatArea.getX(), heatArea.getBottom() - hh, heatArea.getWidth(), hh);
    }

private:
    SpectrumAnalyzer* analyzer = nullptr;
    std::vector<float> mags;
    float lowCutHz = 20.0f;
    float highCutHz = 20000.0f;
    float heat = 0.0f;
};
