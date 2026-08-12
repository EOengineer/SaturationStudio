# Testing

Offline verifies mirror AmpStudio’s `tools/*_verify` pattern: header-only DSP checks, `clang++` one-shots, exit `0`/`1`.

## Run

```bash
cd /path/to/SaturationStudio

clang++ -std=c++17 -O2 -I Source tools/band_split_verify_main.cpp -o tools/band_split_verify
./tools/band_split_verify

clang++ -std=c++17 -O2 -I Source tools/engine_passthrough_verify_main.cpp -o tools/engine_passthrough_verify
./tools/engine_passthrough_verify
```

## What must pass (v1)

| Suite | Locks in |
|-------|----------|
| `band_split_verify` | FIR cascade latency, wide-open side≈0, impulse delay, narrow-band energy, cutoff clamp |
| `engine_passthrough_verify` | Registry (5 families, Diode/Silicon default, Preamp flavors), diode stub null through split, factory default |

## Adding tests for a new clipper

1. Put pure math / curve checks in `Source/dsp/verify/YourModelVerify.h` (prefer no JUCE).
2. Add `tools/your_model_verify_main.cpp`.
3. Assert known sine → expected harmonic ratios or transfer samples.
4. Add an aliasing smoke (OS on vs off) once the model is nonlinear—may require a small JUCE-linked tool or host script.

## Host smoke

See README checklist (AU + VST3 load, band UI, model switch, latency).
