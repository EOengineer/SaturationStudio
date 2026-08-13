# Diode model

**Shockley diode device** + **dynamic antiparallel clipper** (`Rs` + diode pair + shunt `C`), running inside SaturationStudio’s 4× oversampling island. Flavors select real device params (`Is`, `n Vt`), not ear-tuned algebraic knees.

## Layers

```text
DiodeDevice              → Shockley I(V), G(V)              (reusable part)
AntiParallelClipper      → Vin—Rs—V, diodes to gnd          (static net, still available)
AntiParallelRcClipper    → same + shunt C (trapezoidal)     (Diode family saturator)
DiodeModel               → Drive → RC clipper → makeup      (saturator shell)
```

Audio samples are treated as volts into the clipper. Drive is product gain into that network (not a schematic pot).

## Signal path (inside OS island)

```text
x → Drive gain (0…~34 dB, drive^1.3) → Newton RC clipper → Makeup → y
```

`Drive ≈ 0` hard-bypasses to identity. Anti-aliasing relies on **4× OS** (no ADAA — the Newton transfer has no simple analytic antiderivative). Raise OS later if abuse still aliases.

## Clipper physics

KCL at the clip node \(V\):

\[
\frac{V_\mathrm{in}-V}{R_s} = I_\mathrm{pos}(V) - I_\mathrm{neg}(-V) + I_C
\]

with Shockley \(I = I_S(e^{V/(n V_T)}-1)\). Capacitance uses a **trapezoidal companion** at the oversampled rate (\(G_\mathrm{eq}=2C/T\)). Solved with 1-D Newton each sample; per-channel state holds \(V\) and \(I_C\).

Default parts: \(R_s = 1\,\mathrm{k}\Omega\), \(C = 4.7\,\mathrm{nF}\) (edge softening without killing grit at −18 / Drive ~0.5).

Static [`AntiParallelClipper`](../../Source/dsp/devices/AntiParallelClipper.h) remains for composition / compares (C open ≡ DC solve of the RC net).

## Level / Drive contract

| Setting | Intent |
|---------|--------|
| Plugin input ≈ **−18 dBFS** RMS | Modeling reference (`LevelReference::kReferenceRmsDb`) |
| Drive **0** | Transparent |
| Drive **~0.5** | Clear warmth / grit at −18 |
| Drive **1** | Dense saturation; makeup holds loudness roughly stable |

## Flavors

| Flavor | Devices |
|--------|---------|
| Silicon | Si / Si antiparallel |
| Germanium | Ge / Ge (earlier conduction) |
| LED | LED / LED (later / firmer) |
| Asymmetric | Si pos / Ge neg → even harmonics |

## Sources

| File | Role |
|------|------|
| `Source/dsp/devices/DiodeDevice.h` | Shockley part + Si/Ge/LED factories |
| `Source/dsp/devices/AntiParallelClipper.h` | Static series-R + antiparallel |
| `Source/dsp/devices/AntiParallelRcClipper.h` | Dynamic clipper (shunt C) |
| `Source/dsp/models/DiodeCurve.h` | Flavor → devices, Drive/makeup maps |
| `Source/dsp/models/DiodeModel.h` | `SaturationModel` wrapper + C state |

## Verifies

```bash
clang++ -std=c++17 -O2 -I Source tools/diode_curve_verify_main.cpp -o tools/diode_curve_verify
./tools/diode_curve_verify
```

Checks device physics, DC clipper, RC vs static edge energy, Drive=0 identity, harmonics, high-Drive finite.

## Later

- Raise OS / LUT-ADAA if 4× still aliases under abuse
- Richer dynamic networks (feedback) reusing `DiodeDevice`
- Optional junction capacitance / temperature on the device
