// Offline engine / registry / diode stub verifies (no JUCE).
// Build: clang++ -std=c++17 -O2 -I Source tools/engine_passthrough_verify_main.cpp -o tools/engine_passthrough_verify

#include "dsp/verify/EnginePassthroughVerify.h"
#include <iostream>

int main()
{
    const auto report = verify::runEnginePassthroughVerifications();
    std::cout << report.text;
    return report.ok ? 0 : 1;
}
