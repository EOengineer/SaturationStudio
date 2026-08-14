// Offline tube aliasing metrics (no JUCE). Permanent tools/ suite member.
// Build: clang++ -std=c++17 -O2 -I Source tools/tube_aliasing_verify_main.cpp -o tools/tube_aliasing_verify

#include "dsp/verify/TubeAliasingVerify.h"
#include <iostream>

int main()
{
    const auto report = verify::runTubeAliasingVerifications();
    std::cout << report.text;
    return report.ok ? 0 : 1;
}
