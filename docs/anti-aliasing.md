# Anti-aliasing

## Why it matters

Any instantaneous nonlinearity (clip, tube curve, diode I–V) creates new harmonics. Harmonics that land **above the Nyquist frequency** fold back as aliases—inharmonic junk that does not exist in analog gear.

## What we ship in v1

**4× oversampling** around the saturation model only (`Oversampler.h`), using JUCE high-quality half-band filters (same idea as AmpStudio’s circuit island).

Path: upsample mid → model → downsample mid. Linear filters and out-of-band audio stay at base rate.

Latency from the oversampler is added to the FIR latency and reported to the host.

## Layered defenses (later)

| Technique | Role |
|-----------|------|
| Oversampling | Push images above a higher Nyquist, then filter |
| ADAA (anti-derivative anti-aliasing) | Reduce aliases of memoryless waveshapers cheaply |
| Analog HF limiting in a circuit model | Physical rolloff—not a substitute for OS |

When real diode/tube curves land, keep OS on by default; consider ADAA as an extra layer for cheap shapers.

## Offline note

`engine_passthrough_verify` tests the diode stub **without** JUCE oversampling. Full OS residual belongs in host null tests once nonlinear DSP exists.
