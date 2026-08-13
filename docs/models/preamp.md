# Preamp model

Console **preamp-family** waveshapers with first-order **ADAA**, inside the 4× OS island. Flavors are tuned characters — **not** 1073 / API circuit recreations.

| Flavor | Intent |
|--------|--------|
| **Neve 1073** | Rounder, more even-leaning |
| **API 512** | Punchier, harder knee (sharper tanh) |

## Signal path (inside OS island)

```text
x → Drive gain (0…~30 dB) → ADAA preamp transfer → Makeup → y
```

## Transfer

Sharpened asymmetric tanh (same family as Tube):

\[
f(x)=\frac{a}{s}\tanh\!\left(\frac{s\,x}{a}\right)
\quad\text{with } a = a_{+}\text{ or }a_{-},\ s\ge 1
\]

Flavor coeffs live in `preamp::coeffsForFlavor`.

`Drive ≈ 0` hard-bypasses to identity.

## Level / Drive contract

Same as Diode / Tube (−18 dBFS reference at plugin input).

## Sources

| File | Role |
|------|------|
| `PreampCurve.h` | Flavor coeffs, shape, ADAA, Drive/makeup |
| `PreampModel.h` | `SaturationModel` wrapper |

## Verifies

```bash
clang++ -std=c++17 -O2 -I Source tools/family_curves_verify_main.cpp -o tools/family_curves_verify
./tools/family_curves_verify
```

## Later

- Real console stage topology (gain + iron)
- More flavors without exploding the UI
