#pragma once

#include "SaturationModel.h"
#include "../../util/ParamIDs.h"

/**
 * Diode family — v1 Silicon (and other flavors) are identity stubs.
 *
 * Next step: Shockley / soft-clip curves. Study AmpStudio:
 *   Projects/AmpStudio/Source/dsp/circuit/DiodeModel.h
 *   Projects/AmpStudio/Source/dsp/fx/ts/TsClippingAmp.h
 */
class DiodeModel final : public SaturationModel
{
public:
    const char* getId() const override { return ModelIds::diode; }
    const char* getDisplayName() const override { return "Diode"; }

    void prepare (double, int, int) override {}
    void reset() override {}

    void setDiodeFlavor (int flavor) override { diodeFlavor = flavor; }
    int getDiodeFlavor() const noexcept { return diodeFlavor; }

    void process (float* const*, int, int) override
    {
        // Passthrough stub — nonlinear diode curves come later.
    }

private:
    int diodeFlavor = DiodeFlavorIds::silicon;
};
