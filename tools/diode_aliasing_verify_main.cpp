// Offline diode aliasing metrics (no JUCE). Permanent tools/ suite member.
// Build: clang++ -std=c++17 -O2 -I Source tools/diode_aliasing_verify_main.cpp -o tools/diode_aliasing_verify

#include "dsp/verify/DiodeAliasingVerify.h"
#include <iostream>

int main()
{
    const auto report = verify::runDiodeAliasingVerifications();
    std::cout << report.text;
    return report.ok ? 0 : 1;
}
