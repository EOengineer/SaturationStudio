# Testing

Offline verifies mirror AmpStudio’s `tools/*_verify` pattern: header-only DSP checks, `clang++` one-shots, exit `0`/`1`.

## Run

```bash
cd /path/to/SaturationStudio

clang++ -std=c++17 -O2 -I Source tools/band_split_verify_main.cpp -o tools/band_split_verify
./tools/band_split_verify

clang++ -std=c++17 -O2 -I Source tools/engine_passthrough_verify_main.cpp -o tools/engine_passthrough_verify
./tools/engine_passthrough_verify

clang++ -std=c++17 -O2 -I Source tools/diode_curve_verify_main.cpp -o tools/diode_curve_verify
./tools/diode_curve_verify

clang++ -std=c++17 -O2 -I Source tools/tube_curve_verify_main.cpp -o tools/tube_curve_verify
./tools/tube_curve_verify

clang++ -std=c++17 -O2 -I Source tools/family_curves_verify_main.cpp -o tools/family_curves_verify
./tools/family_curves_verify
```

## What must pass

| Suite | Locks in |
|-------|----------|
| `band_split_verify` | FIR cascade latency, wide-open side≈0, impulse delay, narrow-band energy, cutoff clamp |
| `engine_passthrough_verify` | Registry, Diode Drive=0 null through split, factory default |
| `diode_curve_verify` | Drive=0 identity, Si harmonics at −18/Drive0.5, Asym even rise, Drive ramp, curve math |
| `tube_curve_verify` | Drive=0 identity, even>odd at −18/Drive0.5 vs Si diode, Drive ramp, curve math |
| `family_curves_verify` | Tape/Transformer/Preamp Drive=0 identity, finite at Drive=1, Tape odd-lean, Transformer even vs Si, Preamp Neve even-lean vs API |

## Adding tests for a new clipper

1. Put pure math / curve checks in `Source/dsp/verify/YourModelVerify.h` (prefer no JUCE).
2. Add `tools/your_model_verify_main.cpp`.
3. Assert known sine → expected harmonic ratios or transfer samples.
4. Add an aliasing smoke (OS on vs off) once the model is nonlinear—may require a small JUCE-linked tool or host script.

## Host smoke

See README checklist (AU + VST3 load, band UI, model switch, latency).
