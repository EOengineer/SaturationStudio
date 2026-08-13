# Diode model

**Shockley diode device** + **ideal-op-amp feedback clipper** (`Rin`, `Rf`, antiparallel FB diodes, `Cf`), running inside SaturationStudio’s 4× oversampling island. Flavors select real device params (`Is`, `n Vt`).

## Layers

```text
DiodeDevice              → Shockley I(V), G(V)                 (reusable part)
AntiParallelClipper      → Vin—Rs—V, diodes to gnd             (static net)
AntiParallelRcClipper    → same + shunt C                      (dynamic shunt net)
FeedbackDiodeClipper     → ideal OA + Rin/Rf + FB diodes + Cf  (Diode family saturator)
DiodeModel               → Drive→Rin → feedback clipper → makeup
```

## Signal path (inside OS island)

```text
x → Newton feedback clipper (Rin from Drive) → Makeup → y
```

`Drive ≈ 0` hard-bypasses to identity. **Drive is a component change:** \(R_\mathrm{in}=R_f/G(\mathrm{drive})\) with \(G=10^{(34\cdot\mathrm{drive}^{1.3})/20}\) (~1 → ~50) — same feel as the old pre-gain map, not a separate input gain stage. Anti-aliasing relies on **4× OS** (no ADAA).

## Feedback clipper physics

Ideal OA with `(+)` grounded → virtual ground at `(−)`. KCL at the inverting node:

\[
\frac{V_\mathrm{in}}{R_\mathrm{in}} + \frac{V_\mathrm{out}}{R_f} + I_d(V_\mathrm{out}) + I_{C_f} = 0
\]

with Shockley \(I_d(V)=I_\mathrm{pos}(V)-I_\mathrm{neg}(-V)\). `Cf` uses a trapezoidal companion. Audio out is \(-V_\mathrm{out}\) so small-signal polarity matches the older shunt clippers.

Default: \(R_f=10\,\mathrm{k}\Omega\), \(C_f=100\,\mathrm{pF}\); \(R_\mathrm{in}\) set by Drive.

## Level / Drive contract

| Setting | Intent |
|---------|--------|
| Plugin input ≈ **−18 dBFS** RMS | Modeling reference |
| Drive **0** | Transparent |
| Drive **~0.5** | Clear warmth / grit at −18 |
| Drive **1** | Dense saturation (\(G\approx 50\), ~34 dB); makeup holds loudness roughly stable |

## Flavors

| Flavor | Devices |
|--------|---------|
| Silicon | Si / Si in feedback |
| Germanium | Ge / Ge |
| LED | LED / LED |
| Asymmetric | Si pos / Ge neg → even harmonics |

## Sources

| File | Role |
|------|------|
| `Source/dsp/devices/DiodeDevice.h` | Shockley part |
| `Source/dsp/devices/FeedbackDiodeClipper.h` | Ideal-OA feedback clipper |
| `Source/dsp/models/DiodeCurve.h` | Flavor, Drive→Rin, makeup |
| `Source/dsp/models/DiodeModel.h` | `SaturationModel` + Cf state |

## Verifies

```bash
clang++ -std=c++17 -O2 -I Source tools/diode_curve_verify_main.cpp -o tools/diode_curve_verify
./tools/diode_curve_verify
```

## Later

- Raise OS / LUT-ADAA if needed
- Richer islands (tone) toward pedal-style stages
- Tube-as-device; optional junction C / temperature on diodes
