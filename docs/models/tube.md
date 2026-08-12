# Tube model

Product-grade **asymmetric tanh** tube-family saturator with first-order **ADAA**, inside the 4× OS island (same shell as Diode).

## Signal path (inside OS island)

```text
x → Drive gain (0…~24 dB, drive^1.4) → ADAA tube transfer → Makeup → y
```

## Transfer

\[
f(x) = a\,\tanh(x/a)
\quad\text{with } a = a_{+} \text{ (}x\ge 0\text{) or } a_{-} \text{ (}x<0\text{)}
\]

- \(f'(0)=1\) from both sides (C¹)
- \(a_{+} \ne a_{-}\) → **even** harmonics (vs odd-heavy Silicon diode)
- Mild sharpening (\(s>1\)) + Drive up to ~36 dB → denser at max Drive than the first soft tanh pass
- Antiderivative \(F(x)=(a/s)^{2}\log\cosh(s\,x/a)\) feeds ADAA

`Drive ≈ 0` hard-bypasses to identity.

## Level / Drive contract

Same as Diode:

| Setting | Intent |
|---------|--------|
| Plugin input ≈ **−18 dBFS** RMS | Modeling reference |
| Drive **0** | Transparent |
| Drive **~0.5** | Mostly clean warmth at −18 |
| Drive **1** | Clearly saturated; makeup holds loudness |

## Sources

| File | Role |
|------|------|
| `TubeCurve.h` | Shape, ADAA, Drive/makeup |
| `TubeModel.h` | `SaturationModel` wrapper |

## Verifies

```bash
clang++ -std=c++17 -O2 -I Source tools/tube_curve_verify_main.cpp -o tools/tube_curve_verify
./tools/tube_curve_verify
```

## Later

- Tube flavors (soft / bold / more asymmetric)
- Full MNA / Newton triode stages (Champ-style) — separate milestone
