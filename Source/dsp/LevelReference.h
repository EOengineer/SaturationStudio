#pragma once

namespace LevelReference
{
    /** Modeling reference RMS at plugin input. Author saturation curves against this. */
    inline constexpr float kReferenceRmsDb = -18.0f;

    /** Band makeup (future per-band). Not global master. */
    inline constexpr float kBandOutputMinDb = -24.0f;
    inline constexpr float kBandOutputMaxDb = 24.0f;
}
