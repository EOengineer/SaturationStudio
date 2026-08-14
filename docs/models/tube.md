# Tube model

Product-grade **triode-ish asymmetric tanh** tube-family saturator with first-order **ADAA**, inside the 4× OS island (same shell as Diode).

Flavors are **waveshape stand-ins for mu** (12AX7 / 5751 / 12AU7). The physically modeled stack is:

```text
TubeDevice (Koren Ip) → TriodeStage (common-cathode Newton) → TubeModel (live flip: gated)
```

`TriodeStage` exists and passes `tube_stage_verify` offline. **Live Tube UI still uses the waveshape** until a follow-up wires `TubeModel` to the stage. See [`docs/devices/overview.md`](../devices/overview.md).

**Not in scope:** a full Champ amp in SaturationStudio.

## Signal path (inside OS island) — live today

```text
x → Drive gain (flavor) → ADAA tube transfer → Makeup → y
```

## Offline TriodeStage (not live yet)

```text
x → Drive→grid AC gain → Newton common-cathode (TubeDevice stamp) → AC plate → scale
```

Teaching defaults: \(R_a=100\,\mathrm{k}\Omega\), \(R_k=1.5\,\mathrm{k}\Omega\), \(C_k=22\,\mu\mathrm{F}\), \(V_b=250\,\mathrm{V}\). Drive maps plugin input to grid volts (clamped). No NFB.

## Flavors

| Flavor | Intent (waveshape today) | Library part / stage |
|--------|--------------------------|----------------------|
| **12AX7** (default) | Highest gain density | `devices::twelveAx7()` |
| **5751** | Mid (~70% of AX7 feel) | `devices::type5751()` |
| **12AU7** | Lowest gain — cleaner longer | `devices::twelveAu7()` |

## Transfer (live waveshape)

\[
f(x) = a\,\tanh(s\,x/a)
\quad\text{with } a = a_{+} \text{ (}x\ge 0\text{) or } a_{-} \text{ (}x<0\text{)}
\]

- \(f'(0)=1\) from both sides (C¹)
- \(a_{+} \ne a_{-}\) → **even** harmonics (vs odd-heavy Silicon diode)
- Per-flavor `maxDriveDb` / `driveCurve` / asymmetry
- Antiderivative \(F(x)=(a/s)^{2}\log\cosh(s\,x/a)\) feeds ADAA

`Drive ≈ 0` hard-bypasses to identity on the waveshape path.

## Level / Drive contract

Same as Diode (per flavor):

| Setting | Intent |
|---------|--------|
| Plugin input ≈ **−18 dBFS** RMS | Modeling reference |
| Drive **0** | Transparent (waveshape) / near-silent grid (stage) |
| Drive **~0.5** | Mostly clean warmth at −18 |
| Drive **1** | Clearly saturated; AU7 less crushed than AX7 at the same knob |

## Sources

| File | Role |
|------|------|
| `devices/TubeDevice.h` | Koren triode library part (no Drive / OS) |
| `devices/TriodeStage.h` | Common-cathode Newton consumer (offline) |
| `devices/NewtonSolver.h` | Dense Newton helper |
| `TubeCurve.h` | Interim shape, ADAA, flavor coeffs / Drive maps |
| `TubeModel.h` | `SaturationModel` wrapper + `setTubeFlavor` (waveshape until flip) |

## Verifies

```bash
clang++ -std=c++17 -O2 -I Source tools/tube_curve_verify_main.cpp -o tools/tube_curve_verify
./tools/tube_curve_verify

clang++ -std=c++17 -O2 -I Source tools/tube_device_verify_main.cpp -o tools/tube_device_verify
./tools/tube_device_verify

clang++ -std=c++17 -O2 -I Source tools/tube_stage_verify_main.cpp -o tools/tube_stage_verify
./tools/tube_stage_verify
```

## Later

- Wire `TubeModel::process` → `TriodeStage` (per channel); flavors → device factories
- Host-listen vs waveshape; then park tanh path
- Richer stage (Miller, coupling) — still not a Champ amp
