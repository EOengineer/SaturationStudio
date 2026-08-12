// Offline band-split verifies (no JUCE).
// Build: clang++ -std=c++17 -O2 -I Source tools/band_split_verify_main.cpp -o tools/band_split_verify

#include "dsp/verify/BandSplitVerify.h"
#include <iostream>

int main()
{
    const auto report = verify::runBandSplitVerifications();
    std::cout << report.text;
    return report.ok ? 0 : 1;
}
