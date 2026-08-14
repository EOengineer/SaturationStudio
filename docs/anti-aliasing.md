# Anti-aliasing

## Why it matters

Any instantaneous nonlinearity (clip, tube curve, diode I–V) creates new harmonics. Harmonics that land **above the Nyquist frequency** fold back as aliases—inharmonic junk that does not exist in analog gear.

## What we ship

**4× oversampling** around the saturation model (`Oversampler.h`), **linear-phase FIR** half-band stages (`filterHalfBandFIREquiripple`). Diode uses a Newton clipper (no ADAA); Tube/Tape/Transformer/Preamp keep first-order ADAA on their waveshapers.

Path: upsample mid → model (Drive → nonlinearity → makeup) → downsample mid.

FIR OS (not IIR) keeps mid/side delay compensation phase-aligned. **`useIntegerLatency` is on** so reported OS latency is an integer — truncating a fractional latency was combing wet vs dry/side.

Raising the OS factor is on the table if abuse cases still alias after 4× — especially for the diode Newton path.

## Layered defenses

| Technique | Role |
|-----------|------|
| Oversampling (4× now) | Push images above a higher Nyquist, then filter |
| ADAA (Tube / Tape / Transformer / Preamp) | Cut aliases of memoryless shapers cheaply |
| Diode Newton clipper | Relies on OS; raise factor if needed |
| Analog HF limiting in a circuit model | Later dynamic networks — not a substitute for OS |

## Offline note

`diode_curve_verify` checks the device/clipper/Drive/harmonics **without** oversampling.

`diode_aliasing_verify` is the permanent AA development tool: Silicon diode at −18 dBFS, 5 kHz sine, Drive 0.5 (gate) and Drive 1.0 (report). It runs an analytic F× sine through `DiodeModel`, Kaiser-LPF decimates to 48 kHz, and prints harmonic/total ratio + residual dB for 1× vs 4×. That FIR proxy is **not** identical to `juce::dsp::Oversampling` equiripple half-bands — use it to catch regressions while iterating; confirm meaningful moves with Plugin Doctor / host sweeps.
