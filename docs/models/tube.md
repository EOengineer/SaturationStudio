# Tube model

Live **common-cathode Newton** tube saturator: `TubeDevice` (Koren \(I_p\)) stamped by `TriodeStage`, inside the 4× OS island.

```text
TubeDevice factory (flavor) → TriodeStage (per channel) → makeup → y
```

Waveshape `TubeCurve` (ADAA tanh) is **parked** in-tree and unused by `TubeModel`. See [`docs/devices/overview.md`](../devices/overview.md).

**Not in scope:** a full Champ amp in SaturationStudio.

## Device vs stage (clipper)

Same split as `DiodeDevice` vs `FeedbackDiodeClipper`:

```text
TubeDevice     → accurate Ip(Vgk, Vak)  — do not inflate µ/KP for “more grit”
TriodeStage    → Drive→grid, Miller, load, coupling, plate scale  — saturation depth lives here
TubeModel      → makeup / −18 loudness contract
```

Extra clipping and Drive feel come from **how hard we drive the grid and how the load presents**, not from fudging Koren factory params.

## Signal path (inside OS island) — live

```text
x → Drive→grid AC gain → Rg/Miller LPF → clamp → Newton (Vp, Vk) → AC plate scale → Cc coupling HPF → makeup → y
```

`Drive ≈ 0` hard-bypasses to identity (no stage processing).

Teaching stage defaults:

| Part | Value | Role |
|------|-------|------|
| \(R_a\) | \(120\,\mathrm{k}\Omega\) | Plate load (raised slightly for stage gain into clip) |
| \(R_k\) / \(C_k\) | \(1.5\,\mathrm{k}\Omega\) / \(22\,\mu\mathrm{F}\) | Cathode bias + bypass |
| \(V_b\) | \(250\,\mathrm{V}\) | B+ |
| \(C_c\) / \(R_l\) | \(22\,\mathrm{nF}\) / \(1\,\mathrm{M}\Omega\) | Output coupling HPF (~7 Hz) |
| \(R_g\) / \(C_m\) | \(68\,\mathrm{k}\Omega\) / \(100\,\mathrm{pF}\) | Grid stopper + effective Miller (1st-order LPF; not a full Cgp stamp) |

Grid clamp ≈ \([-8,+1]\,\mathrm{V}\). No NFB. Newton stays **2×2** (Vp, Vk).

## Flavors

| Flavor | Library part |
|--------|----------------|
| **12AX7** (default) | `devices::twelveAx7()` |
| **5751** | `devices::type5751()` |
| **12AU7** | `devices::twelveAu7()` |

## Level / Drive contract

Targets at input ≈ **−18 dBFS** RMS (AX7 unless noted):

| Setting | Intent |
|---------|--------|
| Drive **0** | Transparent (identity bypass) |
| Drive **~0.5** | Warm; RMS(out) roughly **0…+11 dB** vs input (prefer ~+6…+10, closer to Diode) |
| Drive **1** | Clearly more saturated than 0.5 (higher H2…H5); loudness held by makeup |
| AU7 @ **0.5** | Quieter / cleaner than AX7 |

Stage knobs: `kMaxGridGain` / `kDriveCurve` / `kPlateToAudio` in `TriodeStage`; `TubeModel::updateMakeup()` includes a fixed **+6.5 dB** output nudge (Drive > 0) plus Drive-dependent compensation.

## Sources

| File | Role |
|------|------|
| `devices/TubeDevice.h` | Koren triode library part |
| `devices/TriodeStage.h` | Common-cathode Newton consumer (live) — coupling + Miller |
| `devices/NewtonSolver.h` | Dense Newton helper |
| `TubeModel.h` | `SaturationModel` wrapper — owns per-channel stages |
| `TubeCurve.h` | Parked waveshape (unused by live path) |

## Verifies

```bash
clang++ -std=c++17 -O2 -I Source tools/tube_curve_verify_main.cpp -o tools/tube_curve_verify
./tools/tube_curve_verify

clang++ -std=c++17 -O2 -I Source tools/tube_aliasing_verify_main.cpp -o tools/tube_aliasing_verify
./tools/tube_aliasing_verify

clang++ -std=c++17 -O2 -I Source tools/tube_device_verify_main.cpp -o tools/tube_device_verify
./tools/tube_device_verify

clang++ -std=c++17 -O2 -I Source tools/tube_stage_verify_main.cpp -o tools/tube_stage_verify
./tools/tube_stage_verify
```

`tube_curve_verify` locks −18 RMS in 0…+11 dB @ Drive 0.5/1, harmonic density Drive 1 ≫ 0.5, and AX7 > AU7 RMS. `tube_stage_verify` also locks coupling DC block and mild Miller HF ≤ mid. `tube_aliasing_verify` mirrors the diode AA proxy (1× vs 4× at 5 kHz).

## Later

- True grid conduction vs hard clamp
- Full Cgp stamp (expand Newton) if the light Miller LPF is not enough
- Richer stage — still not a Champ amp
