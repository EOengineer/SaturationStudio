# Diode model

Product-grade **memoryless diode-family saturator** with first-order **ADAA**, running inside SaturationStudio’s 4× oversampling island.

## Signal path (inside OS island)

```text
x → Drive gain (0…~24 dB, drive^1.4) → ADAA soft-clip → Makeup → y
```

## Transfer

Algebraic soft-clip (C¹, f'(0)=1):

\[
f(x) = a\,\frac{x}{\sqrt{a^2 + x^2}}
\]

Positive and negative polarities can use different thresholds \(a_{+}, a_{-}\) (asymmetric flavor). Antiderivative \(F(0)=0\) feeds first-order ADAA:

\[
y[n] = \frac{F(x[n]) - F(x[n-1])}{x[n] - x[n-1]}
\quad\text{(else } f(x[n]) \text{ when } \Delta x \approx 0\text{)}
\]

`Drive ≈ 0` hard-bypasses to identity.

## Level / Drive contract

| Setting | Intent |
|---------|--------|
| Plugin input ≈ **−18 dBFS** RMS | Modeling reference (`LevelReference::kReferenceRmsDb`) |
| Drive **0** | Transparent |
| Drive **~0.5** | Mostly clean, hint of grit at −18 |
| Drive **1** | Clearly saturated; makeup holds loudness roughly stable |

## Flavors

| Flavor | Character |
|--------|-----------|
| Silicon | Tight, odd-heavy, modern knee |
| Germanium | Earlier / softer threshold |
| LED | More headroom, then firmer grab |
| Asymmetric | Uneven knees → even harmonics |

Coeffs live in `Source/dsp/models/DiodeCurve.h` — tuned by ear/spectrum, not pedal clones.

## Sources

| File | Role |
|------|------|
| `DiodeCurve.h` | Shape, ADAA, Drive/makeup maps, flavor table |
| `DiodeModel.h` | `SaturationModel` wrapper (per-channel ADAA state) |

## Verifies

```bash
clang++ -std=c++17 -O2 -I Source tools/diode_curve_verify_main.cpp -o tools/diode_curve_verify
./tools/diode_curve_verify
```

Checks Drive=0 identity, mild Si harmonics at −18 / Drive 0.5, Asym even rise, Drive ramp, curve math.

## Later

- Raise OS factor if ADAA+4× still aliases under abuse
- Optional light pre/post linear color per flavor
