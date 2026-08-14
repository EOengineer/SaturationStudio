// Offline TriodeStage (Newton common-cathode) verifies — no JUCE / no live TubeModel.
// Build: clang++ -std=c++17 -O2 -I Source tools/tube_stage_verify_main.cpp -o tools/tube_stage_verify

#include "dsp/verify/TriodeStageVerify.h"
#include <iostream>

int main()
{
    const auto report = verify::runTriodeStageVerifications();
    std::cout << report.text;
    return report.ok ? 0 : 1;
}
