# Diode model

**Shockley diode device** (junction + bulk series \(R_s\)) + **ideal-op-amp feedback clipper** (`Rin`, `Rf`, antiparallel FB diodes, `Cf`), running inside SaturationStudio’s 4× oversampling island. Flavors select real device params (`Is`, `n Vt`, bulk \(R_s\)).

## Layers

```text
DiodeDevice              → Shockley I(Vj) + bulk Rs stamp      (reusable part)
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

with Shockley \(I_d(V)=I_\mathrm{pos}(V)-I_\mathrm{neg}(-V)\) including each diode’s bulk \(R_s\) (terminal \(V=V_j+IR_s\); high-forward \(G\to 1/R_s\)). That bulk \(R_s\) is **not** the shunt-clipper source resistor and **not** Drive→`Rin`. `Cf` uses a trapezoidal companion. Audio out is \(-V_\mathrm{out}\) so small-signal polarity matches the older shunt clippers.

Default: \(R_f=10\,\mathrm{k}\Omega\), \(C_f=100\,\mathrm{pF}\); \(R_\mathrm{in}\) set by Drive.

## Circuit C vs junction C

Two different capacitors. Do not treat “add C to the diode” as the next topology step — the live path is already a circuit.

**Circuit C (shipped).** A discrete part in the net, roughly constant. Feedback `Cf` (100 pF) and shunt-clipper `C` (4.7 nF) use trapezoidal companions. That is already \(I = f(V) + C\,dV/dt\), but \(f(V)\) is the diode and \(C\) is **not** the diode. Impulse verifies exist because of this **network** memory.

**Junction / diffusion C (not on the part).** Charge stored in the semiconductor (`DiodeDevice`), like `Is` / `nVt` / bulk `rs`:

- Junction (depletion) C — reverse / small forward; \(C_j(V) \propto (1-V/\phi)^{-m}\).
- Diffusion C — strong forward; stored charge \(\propto\) forward current. Reverse recovery is pulling that charge out before the diode can block.

Shockley + bulk \(R_s\) is still memoryless (same \(V\) now \(\Rightarrow\) same \(I\) now). Junction C would make the **part** history-dependent. SPICE-ish \(C_{j0}\) (~2–8 pF Si) is small next to 100 pF `Cf`; diffusion C / Ge would show more. Same story as bulk \(R_s\): authentic on the reusable part, modest change to the current saturator because the net already has C.

Where the stack sits:

| Level | Meaning | This repo |
|-------|---------|-----------|
| 0 | Algebraic waveshaper \(x\to y\) | Replaced |
| 1 | Shockley \(V\to I\) | `DiodeDevice` junction |
| 2 | Junction + bulk \(R_s\) | Shipped |
| 3 | Nonlinear C **on the diode** | Later (not next) |
| 4 | Diode inside a circuit | Live path (OA + Rin/Rf + FB diodes + Cf) |

Status: **static diode + dynamic circuit**. Remaining diode physics (junction C, temperature) belongs on the part, not as a new clipper topology.

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
| `Source/dsp/devices/DiodeDevice.h` | Shockley + bulk Rs stamp |
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
- Tube-as-device; junction/diffusion C and temperature on `DiodeDevice` (companion stamp + per-diode state — not a one-line C)
