#pragma once

#include "SaturationModel.h"
#include "../../util/ParamIDs.h"

/** Tape family stub — future soft clip + HF character. */
class TapeModel final : public SaturationModel
{
public:
    const char* getId() const override { return ModelIds::tape; }
    const char* getDisplayName() const override { return "Tape"; }

    void prepare (double, int, int) override {}
    void reset() override {}
    void process (float* const*, int, int) override {}
};
