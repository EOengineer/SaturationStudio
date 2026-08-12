// Offline Tape / Transformer / Preamp curve verifies (no JUCE).
// Build: clang++ -std=c++17 -O2 -I Source tools/family_curves_verify_main.cpp -o tools/family_curves_verify

#include "dsp/verify/FamilyCurvesVerify.h"
#include <iostream>

int main()
{
    const auto report = verify::runFamilyCurvesVerifications();
    std::cout << report.text;
    return report.ok ? 0 : 1;
}
