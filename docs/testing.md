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

clang++ -std=c++17 -O2 -I Source tools/diode_aliasing_verify_main.cpp -o tools/diode_aliasing_verify
./tools/diode_aliasing_verify

clang++ -std=c++17 -O2 -I Source tools/tube_curve_verify_main.cpp -o tools/tube_curve_verify
./tools/tube_curve_verify

clang++ -std=c++17 -O2 -I Source tools/tube_device_verify_main.cpp -o tools/tube_device_verify
./tools/tube_device_verify

clang++ -std=c++17 -O2 -I Source tools/family_curves_verify_main.cpp -o tools/family_curves_verify
./tools/family_curves_verify
```

## What must pass

| Suite | Locks in |
|-------|----------|
| `band_split_verify` | FIR cascade latency, wide-open side≈0, impulse delay, narrow-band energy, cutoff clamp |
| `engine_passthrough_verify` | Registry, Diode Drive=0 null through split, factory default |
| `diode_curve_verify` | Device physics (incl. bulk Rs), DC/FB/RC nets, Drive→Rin, Drive=0, harmonics, high-Drive finite |
| `diode_aliasing_verify` | Silicon aliasing proxy: 1× vs 4× harm/total at 5 kHz (−18, Drive 0.5 gate; Drive 1.0 report). Ongoing AA tool — not JUCE OS identical; Plugin Doctor for host sweeps |
| `tube_curve_verify` | Drive=0 identity, AX7 even>odd vs Si diode, AX7>5751>AU7 Drive1 gain + Drive0.5 H2, flavors finite, Drive ramp, f'(0)≈1 |
| `tube_device_verify` | Koren TubeDevice: FD conductances, Ip≥0 / cutoff, µ AX7>5751>AU7 + sane Ip at typical biases, abuse finite |
| `family_curves_verify` | Tape/Transformer/Preamp Drive=0 identity, finite at Drive=1, Tape odd-lean, Transformer even vs Si, Preamp Neve even-lean vs API |

## Adding tests for a new clipper

1. Put pure math / curve checks in `Source/dsp/verify/YourModelVerify.h` (prefer no JUCE).
2. Add `tools/your_model_verify_main.cpp`.
3. Assert known sine → expected harmonic ratios or transfer samples.
4. Add or extend an aliasing smoke (`diode_aliasing_verify` pattern: OS-rate process + LPF decimate + harm/total). Host/Plugin Doctor sweeps remain the ground truth.

## Host smoke

See README checklist (AU + VST3 load, band UI, model switch, latency).
