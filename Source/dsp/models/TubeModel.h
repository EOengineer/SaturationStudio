#pragma once

#include "SaturationModel.h"
#include "../../util/ParamIDs.h"

/** Tube family stub — future 12AX7-style transfer curves. */
class TubeModel final : public SaturationModel
{
public:
    const char* getId() const override { return ModelIds::tube; }
    const char* getDisplayName() const override { return "Tube"; }

    void prepare (double, int, int) override {}
    void reset() override {}
    void process (float* const*, int, int) override {}
};
