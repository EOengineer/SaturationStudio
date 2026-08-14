// Offline TubeDevice (Koren) verifies — library part, no JUCE / no saturator.
// Build: clang++ -std=c++17 -O2 -I Source tools/tube_device_verify_main.cpp -o tools/tube_device_verify

#include "dsp/verify/TubeDeviceVerify.h"
#include <iostream>

int main()
{
    const auto report = verify::runTubeDeviceVerifications();
    std::cout << report.text;
    return report.ok ? 0 : 1;
}
