#pragma once

#include "../devices/TubeDevice.h"
#include <cmath>
#include <sstream>
#include <string>

namespace verify
{
struct TubeDeviceReport
{
    bool ok = true;
    std::string text;
    void pass (const std::string& msg) { text += "PASS: " + msg + "\n"; }
    void fail (const std::string& msg)
    {
        ok = false;
        text += "FAIL: " + msg + "\n";
    }
};

inline TubeDeviceReport runTubeDeviceVerifications()
{
    TubeDeviceReport r;

    // Conductances match finite-difference at a typical preamp bias probe
    {
        const auto t = devices::twelveAx7();
        constexpr float vgk = -1.0f;
        constexpr float vak = 150.0f;
        constexpr float h = 1.0e-3f;

        float gG = 0.0f, gP = 0.0f;
        t.plateConductances (vgk, vak, gG, gP);

        const float i0 = t.plateCurrent (vgk, vak);
        const float fdG = (t.plateCurrent (vgk + h, vak) - i0) / h;
        const float fdP = (t.plateCurrent (vgk, vak + h) - i0) / h;

        const float relG = std::abs (gG - fdG) / std::max (std::abs (fdG), 1.0e-12f);
        const float relP = std::abs (gP - fdP) / std::max (std::abs (fdP), 1.0e-12f);

        std::ostringstream oss;
        oss << "AX7 conductances vs FD relG=" << relG << " relP=" << relP;
        if (relG < 1.0e-3f && relP < 1.0e-3f && gG >= 0.0f && gP > 0.0f)
            r.pass (oss.str());
        else
            r.fail (oss.str());
    }

    // Ip >= 0 in forward region; cutoff-ish when grid very negative
    {
        const auto t = devices::twelveAx7();
        const float iFwd = t.plateCurrent (-0.5f, 200.0f);
        const float iCut = t.plateCurrent (-5.0f, 200.0f);
        std::ostringstream oss;
        oss << "AX7 Ip(fwd)=" << iFwd << " Ip(cut)=" << iCut;
        if (iFwd > 0.0f && iCut >= 0.0f && iCut < iFwd * 0.05f)
            r.pass (oss.str());
        else
            r.fail (oss.str());
    }

    // Flavor ordering: µ AX7 > 5751 > AU7; each has sane Ip at a typical bias
    {
        const auto ax = devices::twelveAx7();
        const auto mid = devices::type5751();
        const auto au = devices::twelveAu7();

        // Classic-ish probes (not the same Vgk for every type)
        const float iAx = ax.plateCurrent (-1.5f, 250.0f);
        const float i5751 = mid.plateCurrent (-1.2f, 250.0f);
        const float iAu = au.plateCurrent (-5.0f, 250.0f);

        std::ostringstream oss;
        oss << "mu AX7=" << ax.mu << " 5751=" << mid.mu << " AU7=" << au.mu
            << " | Ip AX7(-1.5,250)=" << iAx
            << " 5751(-1.2,250)=" << i5751
            << " AU7(-5,250)=" << iAu;

        const bool muOrder = ax.mu > mid.mu && mid.mu > au.mu;
        const bool saneIp = iAx > 1.0e-4f && iAx < 5.0e-3f
                         && i5751 > 1.0e-4f && i5751 < 8.0e-3f
                         && iAu > 1.0e-4f && iAu < 2.0e-2f;
        if (muOrder && saneIp)
            r.pass (oss.str());
        else
            r.fail (oss.str() + " (expected mu AX7 > 5751 > AU7 and sane Ip at typical biases)");
    }

    // Finite at abuse bias
    {
        bool finite = true;
        for (const auto& t : { devices::twelveAx7(), devices::type5751(), devices::twelveAu7() })
        {
            for (float vgk : { -10.0f, -1.0f, 0.0f, 1.0f })
                for (float vak : { 0.0f, 50.0f, 300.0f, 400.0f })
                {
                    const float ip = t.plateCurrent (vgk, vak);
                    float gG = 0.0f, gP = 0.0f;
                    t.plateConductances (vgk, vak, gG, gP);
                    if (! std::isfinite (ip) || ! std::isfinite (gG) || ! std::isfinite (gP))
                        finite = false;
                    if (ip < 0.0f)
                        finite = false;
                }
        }
        if (finite)
            r.pass ("all flavors finite / Ip>=0 on abuse grid");
        else
            r.fail ("NaN/Inf or negative Ip on abuse grid");
    }

    return r;
}
} // namespace verify
