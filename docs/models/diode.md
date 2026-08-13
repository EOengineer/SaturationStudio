# Diode model

**Shockley diode device** + static **antiparallel clipper** (`Rs` + diode pair), running inside SaturationStudio’s 4× oversampling island. Flavors select real device params (`Is`, `n Vt`), not ear-tuned algebraic knees.

## Layers

```text
DiodeDevice          → Shockley I(V), G(V)          (reusable part)
AntiParallelClipper  → Vin—Rs—V, diodes to gnd     (static resistive network)
DiodeModel           → Drive → clipper → makeup    (saturator shell)
```

Audio samples are treated as volts into the clipper. Drive is product gain into that network (not a schematic pot).

## Signal path (inside OS island)

```text
x → Drive gain (0…~34 dB, drive^1.3) → Newton clipper → Makeup → y
```

`Drive ≈ 0` hard-bypasses to identity. Anti-aliasing relies on **4× OS** (no ADAA — the Newton transfer has no simple analytic antiderivative).

## Clipper physics

KCL at the clip node \(V\):

\[
\frac{V_\mathrm{in}-V}{R_s} = I_\mathrm{pos}(V) - I_\mathrm{neg}(-V)
\]

with Shockley \(I = I_S(e^{V/(n V_T)}-1)\). Solved with 1-D Newton each sample. Network is **static** (fixed `Rs`, no capacitors); warm-start state is solver continuity only.

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
| `Source/dsp/devices/AntiParallelClipper.h` | Series-R + antiparallel Newton clipper |
| `Source/dsp/models/DiodeCurve.h` | Flavor → devices, Drive/makeup maps |
| `Source/dsp/models/DiodeModel.h` | `SaturationModel` wrapper |

## Verifies

```bash
clang++ -std=c++17 -O2 -I Source tools/diode_curve_verify_main.cpp -o tools/diode_curve_verify
./tools/diode_curve_verify
```

Checks device conductance vs FD, Vf ordering Ge&lt;Si&lt;LED, clipper small-signal identity, Drive=0 identity, Si harmonics at −18 / Drive 0.5, Asym even rise, Drive ramp.

## Later

- Raise OS factor if 4× still aliases under abuse
- Dynamic networks (C, feedback) that reuse `DiodeDevice`
- Optional junction capacitance on the device
