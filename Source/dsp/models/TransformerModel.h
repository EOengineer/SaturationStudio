#pragma once

#include "SaturationModel.h"
#include "../../util/ParamIDs.h"

/** Transformer family stub — future hysteresis / low-end thicken. */
class TransformerModel final : public SaturationModel
{
public:
    const char* getId() const override { return ModelIds::transformer; }
    const char* getDisplayName() const override { return "Transformer"; }

    void prepare (double, int, int) override {}
    void reset() override {}
    void process (float* const*, int, int) override {}
};
