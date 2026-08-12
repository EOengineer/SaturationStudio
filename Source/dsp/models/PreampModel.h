#pragma once

#include "SaturationModel.h"
#include "../../util/ParamIDs.h"

/** Preamp family stub — Neve 1073 / API 512 flavor selector wired for UI. */
class PreampModel final : public SaturationModel
{
public:
    const char* getId() const override { return ModelIds::preamp; }
    const char* getDisplayName() const override { return "Preamp"; }

    void prepare (double, int, int) override {}
    void reset() override {}

    void setPreampFlavor (int flavor) override { preampFlavor = flavor; }
    int getPreampFlavor() const noexcept { return preampFlavor; }

    void process (float* const*, int, int) override {}

private:
    int preampFlavor = PreampFlavorIds::neve1073;
};
