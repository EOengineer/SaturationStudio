// Offline diode curve / Drive / harmonic verifies (no JUCE).
// Build: clang++ -std=c++17 -O2 -I Source tools/diode_curve_verify_main.cpp -o tools/diode_curve_verify

#include "dsp/verify/DiodeCurveVerify.h"
#include <iostream>

int main()
{
    const auto report = verify::runDiodeCurveVerifications();
    std::cout << report.text;
    return report.ok ? 0 : 1;
}
