# Tube model

Product-grade **triode-ish asymmetric tanh** tube-family saturator with first-order **ADAA**, inside the 4× OS island (same shell as Diode).

Flavors are **waveshape stand-ins for mu** (12AX7 / 5751 / 12AU7) — not Koren `Ip` / Newton stages. Champ-style device physics stays a later milestone.

## Signal path (inside OS island)

```text
x → Drive gain (flavor) → ADAA tube transfer → Makeup → y
```

## Flavors

| Flavor | Intent |
|--------|--------|
| **12AX7** (default) | Highest gain density — hotter Drive map, stronger asymmetry |
| **5751** | Mid (~70% of AX7 feel) |
| **12AU7** | Lowest gain — cleaner longer, softest clip |

## Transfer

\[
f(x) = a\,\tanh(s\,x/a)
\quad\text{with } a = a_{+} \text{ (}x\ge 0\text{) or } a_{-} \text{ (}x<0\text{)}
\]

- \(f'(0)=1\) from both sides (C¹)
- \(a_{+} \ne a_{-}\) → **even** harmonics (vs odd-heavy Silicon diode)
- Per-flavor `maxDriveDb` / `driveCurve` / asymmetry
- Antiderivative \(F(x)=(a/s)^{2}\log\cosh(s\,x/a)\) feeds ADAA

`Drive ≈ 0` hard-bypasses to identity.

## Level / Drive contract

Same as Diode (per flavor):

| Setting | Intent |
|---------|--------|
| Plugin input ≈ **−18 dBFS** RMS | Modeling reference |
| Drive **0** | Transparent |
| Drive **~0.5** | Mostly clean warmth at −18 |
| Drive **1** | Clearly saturated; AU7 less crushed than AX7 at the same knob |

## Sources

| File | Role |
|------|------|
| `TubeCurve.h` | Shape, ADAA, flavor coeffs / Drive maps |
| `TubeModel.h` | `SaturationModel` wrapper + `setTubeFlavor` |

## Verifies

```bash
clang++ -std=c++17 -O2 -I Source tools/tube_curve_verify_main.cpp -o tools/tube_curve_verify
./tools/tube_curve_verify
```

## Later

- Full MNA / Newton triode stages (Champ-style Koren `Ip`) — separate milestone
