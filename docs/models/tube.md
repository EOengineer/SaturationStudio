# Tube model

Product-grade **triode-ish asymmetric tanh** tube-family saturator with first-order **ADAA**, inside the 4× OS island (same shell as Diode).

Flavors are **waveshape stand-ins for mu** (12AX7 / 5751 / 12AU7). The physically modeled library part is [`TubeDevice`](../../Source/dsp/devices/TubeDevice.h) (Koren \(I_p\)) — see [`docs/devices/overview.md`](../devices/overview.md). A Newton stage that *stamps* that part is the next clipping-realism step; this waveshape saturator stays live until then.

**Not in scope:** a full Champ amp in SaturationStudio.

## Signal path (inside OS island)

```text
x → Drive gain (flavor) → ADAA tube transfer → Makeup → y
```

## Flavors

| Flavor | Intent (waveshape today) | Library part (later wiring) |
|--------|--------------------------|-----------------------------|
| **12AX7** (default) | Highest gain density | `devices::twelveAx7()` |
| **5751** | Mid (~70% of AX7 feel) | `devices::type5751()` |
| **12AU7** | Lowest gain — cleaner longer | `devices::twelveAu7()` |

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
| `devices/TubeDevice.h` | Koren triode library part (no Drive / OS) |
| `TubeCurve.h` | Interim shape, ADAA, flavor coeffs / Drive maps |
| `TubeModel.h` | `SaturationModel` wrapper + `setTubeFlavor` |

## Verifies

```bash
clang++ -std=c++17 -O2 -I Source tools/tube_curve_verify_main.cpp -o tools/tube_curve_verify
./tools/tube_curve_verify

clang++ -std=c++17 -O2 -I Source tools/tube_device_verify_main.cpp -o tools/tube_device_verify
./tools/tube_device_verify
```

## Later

- Common-cathode Newton island that stamps `TubeDevice` (first circuit consumer)
- Rewire UI flavors to device factories instead of tanh coeffs
