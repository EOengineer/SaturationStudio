#pragma once

#include <cstddef>
#include <string>

/**
 * Base interface for topology-based saturation models.
 * process() runs at the oversampled rate inside SaturationEngine.
 *
 * JUCE-free surface so registry / stubs can be verified offline.
 * Plugin builds pass float planar blocks via SaturationEngine.
 */
class SaturationModel
{
public:
    virtual ~SaturationModel() = default;

    virtual const char* getId() const = 0;
    virtual const char* getDisplayName() const = 0;

    virtual void prepare (double sampleRate, int maxBlock, int numChannels) = 0;
    virtual void reset() = 0;

    /** In-place process of planar float channels at oversampled rate. */
    virtual void process (float* const* channels, int numChannels, int numSamples) = 0;

    virtual void setDrive (float /*drive01*/) {}
    virtual void setDiodeFlavor (int /*flavor*/) {}
    virtual void setTubeFlavor (int /*flavor*/) {}
    virtual void setPreampFlavor (int /*flavor*/) {}
};
