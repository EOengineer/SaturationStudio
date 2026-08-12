# Anti-aliasing

## Why it matters

Any instantaneous nonlinearity (clip, tube curve, diode I–V) creates new harmonics. Harmonics that land **above the Nyquist frequency** fold back as aliases—inharmonic junk that does not exist in analog gear.

## What we ship

**4× oversampling** around the saturation model (`Oversampler.h`), **linear-phase FIR** half-band stages (`filterHalfBandFIREquiripple`), plus **first-order ADAA** on the Diode waveshaper (`DiodeCurve.h`).

Path: upsample mid → model (Drive → ADAA shape → makeup) → downsample mid.

FIR OS (not IIR) keeps mid/side delay compensation phase-aligned. **`useIntegerLatency` is on** so reported OS latency is an integer — truncating a fractional latency was combing wet vs dry/side.

Raising the OS factor is on the table if abuse cases still alias after ADAA+4× — tune after the diode path is proven in-host.

## Layered defenses

| Technique | Role |
|-----------|------|
| Oversampling (4× now) | Push images above a higher Nyquist, then filter |
| ADAA (Diode) | Cut aliases of the memoryless shaper cheaply |
| Analog HF limiting in a circuit model | Later families — not a substitute for OS |

## Offline note

`diode_curve_verify` checks the shaper/Drive/harmonics **without** JUCE oversampling. Full OS+ADAA residual belongs in host listening / future OS-linked tools.
