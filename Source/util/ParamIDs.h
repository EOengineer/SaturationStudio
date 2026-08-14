#pragma once

namespace ParamIDs
{
    inline constexpr const char* lowCutHz  = "lowCutHz";
    inline constexpr const char* highCutHz = "highCutHz";
    inline constexpr const char* drive     = "drive";
    inline constexpr const char* mix       = "mix";
    /** Band / sat makeup (future per-band). */
    inline constexpr const char* outputDb  = "outputDb";
    inline constexpr const char* satModel  = "satModel";

    inline constexpr const char* diodeFlavor  = "diodeFlavor";
    inline constexpr const char* tubeFlavor   = "tubeFlavor";
    inline constexpr const char* preampFlavor = "preampFlavor";
}

namespace ModelIds
{
    inline constexpr const char* diode       = "diode";
    inline constexpr const char* tube        = "tube";
    inline constexpr const char* tape        = "tape";
    inline constexpr const char* transformer = "transformer";
    inline constexpr const char* preamp      = "preamp";
}

namespace DiodeFlavorIds
{
    inline constexpr int silicon    = 0;
    inline constexpr int germanium  = 1;
    inline constexpr int led        = 2;
    inline constexpr int asymmetric = 3;
}

namespace TubeFlavorIds
{
    inline constexpr int ax7      = 0; // 12AX7
    inline constexpr int type5751 = 1; // 5751
    inline constexpr int au7      = 2; // 12AU7
}

namespace PreampFlavorIds
{
    inline constexpr int neve1073 = 0;
    inline constexpr int api512   = 1;
}
