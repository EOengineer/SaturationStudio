// Offline tube curve / Drive / harmonic verifies (no JUCE).
// Build: clang++ -std=c++17 -O2 -I Source tools/tube_curve_verify_main.cpp -o tools/tube_curve_verify

#include "dsp/verify/TubeCurveVerify.h"
#include <iostream>

int main()
{
    const auto report = verify::runTubeCurveVerifications();
    std::cout << report.text;
    return report.ok ? 0 : 1;
}
