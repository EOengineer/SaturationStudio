# Transformer model

Product-grade **asymmetric algebraic** transformer-family saturator with first-order **ADAA**, inside the 4× OS island.

**No hysteresis / memory yet** — “iron” feel via \(a_{+}\ne a_{-}\) only (same methodology as Diode Asym / Tube). Full BH-loop hysteresis is a later milestone.

## Signal path (inside OS island)

```text
x → Drive gain (0…~30 dB) → ADAA transformer transfer → Makeup → y
```

## Transfer

\[
f(x) = a\,\frac{x}{\sqrt{a^{2}+x^{2}}}
\quad\text{with } a = a_{+} \text{ (}x\ge 0\text{) or } a_{-} \text{ (}x<0\text{)}
\]

- Asymmetry → **even**-leaning vs Tape / Silicon
- Softer algebraic ceiling than a hard clip
- Antiderivative per-side \(F(x)=a(\sqrt{a^{2}+x^{2}}-a)\) feeds ADAA

`Drive ≈ 0` hard-bypasses to identity.

## Level / Drive contract

Same as Diode / Tube (−18 dBFS reference at plugin input).

## Sources

| File | Role |
|------|------|
| `TransformerCurve.h` | Shape, ADAA, Drive/makeup |
| `TransformerModel.h` | `SaturationModel` wrapper |

## Verifies

```bash
clang++ -std=c++17 -O2 -I Source tools/family_curves_verify_main.cpp -o tools/family_curves_verify
./tools/family_curves_verify
```

## Later

- True hysteresis (stateful BH / Jiles–Atherton-style)
- Frequency-dependent iron loss
