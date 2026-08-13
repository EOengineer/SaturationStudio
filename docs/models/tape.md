# Tape model

Product-grade **soft symmetric** algebraic tape-family saturator with first-order **ADAA**, inside the 4× OS island (same shell as Diode / Tube).

Not a tape-machine circuit (no bias, no HF loss, no compression dynamics yet) — a soft ceiling that leans **odd**.

## Signal path (inside OS island)

```text
x → Drive gain (0…~28 dB) → ADAA tape transfer → Makeup → y
```

## Transfer

\[
f(x) = a\,\frac{x}{\sqrt{a^{2}+x^{2}}}
\]

- Symmetric → **odd**-leaning harmonics
- Softer than Silicon diode snap at the same Drive
- Antiderivative \(F(x)=a(\sqrt{a^{2}+x^{2}}-a)\) feeds ADAA

`Drive ≈ 0` hard-bypasses to identity.

## Level / Drive contract

Same as Diode / Tube (−18 dBFS reference at plugin input).

## Sources

| File | Role |
|------|------|
| `TapeCurve.h` | Shape, ADAA, Drive/makeup |
| `TapeModel.h` | `SaturationModel` wrapper |

## Verifies

```bash
clang++ -std=c++17 -O2 -I Source tools/family_curves_verify_main.cpp -o tools/family_curves_verify
./tools/family_curves_verify
```

## Later

- HF roll / bias noise character
- Soft compression envelope
