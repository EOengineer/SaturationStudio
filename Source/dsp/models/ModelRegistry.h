#pragma once

#include "DiodeModel.h"
#include "PreampModel.h"
#include "TapeModel.h"
#include "TransformerModel.h"
#include "TubeModel.h"
#include "../../util/ParamIDs.h"
#include <memory>
#include <string>
#include <vector>

struct ModelFlavorInfo
{
    int id = 0;
    const char* name = "";
};

struct ModelFamilyInfo
{
    const char* id = "";
    const char* displayName = "";
    bool hasFlavor = false;
    std::vector<ModelFlavorInfo> flavors;
};

/**
 * Topology + flavor registry. Metadata for UI; factory for DSP instances.
 */
class ModelRegistry
{
public:
    static const std::vector<ModelFamilyInfo>& families()
    {
        static const std::vector<ModelFamilyInfo> list {
            { ModelIds::diode, "Diode", true,
              { { DiodeFlavorIds::silicon, "Silicon" },
                { DiodeFlavorIds::germanium, "Germanium" },
                { DiodeFlavorIds::led, "LED" },
                { DiodeFlavorIds::asymmetric, "Asymmetric" } } },
            { ModelIds::tube, "Tube", false, {} },
            { ModelIds::tape, "Tape", false, {} },
            { ModelIds::transformer, "Transformer", false, {} },
            { ModelIds::preamp, "Preamp", true,
              { { PreampFlavorIds::neve1073, "Neve 1073" },
                { PreampFlavorIds::api512, "API 512" } } },
        };
        return list;
    }

    static int indexOfFamily (const char* id)
    {
        const auto& list = families();
        for (int i = 0; i < (int) list.size(); ++i)
            if (std::string (list[(size_t) i].id) == id)
                return i;
        return 0;
    }

    static const char* defaultFamilyId() { return ModelIds::diode; }

    static std::unique_ptr<SaturationModel> create (int familyIndex)
    {
        switch (familyIndex)
        {
            case 0: return std::make_unique<DiodeModel>();
            case 1: return std::make_unique<TubeModel>();
            case 2: return std::make_unique<TapeModel>();
            case 3: return std::make_unique<TransformerModel>();
            case 4: return std::make_unique<PreampModel>();
            default: return std::make_unique<DiodeModel>();
        }
    }

    static std::unique_ptr<SaturationModel> createById (const char* id)
    {
        return create (indexOfFamily (id));
    }

    /** Offline verify helper. */
    static bool sanityCheck (std::string& report)
    {
        const auto& list = families();
        bool ok = true;
        if (list.size() != 5)
        {
            report += "FAIL: expected 5 families\n";
            ok = false;
        }
        if (std::string (defaultFamilyId()) != ModelIds::diode)
        {
            report += "FAIL: default family is not diode\n";
            ok = false;
        }
        if (! list[0].hasFlavor || list[0].flavors.size() != 4
            || std::string (list[0].flavors[0].name) != "Silicon")
        {
            report += "FAIL: diode flavors incomplete / Silicon not default index 0\n";
            ok = false;
        }
        if (! list[4].hasFlavor || list[4].flavors.size() != 2)
        {
            report += "FAIL: preamp flavors incomplete\n";
            ok = false;
        }
        if (ok)
            report += "PASS: registry sanity (5 families, Diode/Silicon default, Preamp flavors)\n";
        return ok;
    }
};
