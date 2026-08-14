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
TriodeStage    → Drive→grid volts, Ra/Rk/Ck/Vb, plate scale  — saturation depth lives here
TubeModel      → makeup / −18 loudness contract
```

Extra clipping and Drive feel come from **how hard we drive the grid and how the load presents**, not from fudging Koren factory params.

## Signal path (inside OS island) — live

```text
x → Drive→grid AC gain → Newton common-cathode (Vp, Vk) → AC plate scale → makeup → y
```

`Drive ≈ 0` hard-bypasses to identity (no stage processing).

Teaching stage defaults: \(R_a=120\,\mathrm{k}\Omega\), \(R_k=1.5\,\mathrm{k}\Omega\), \(C_k=22\,\mu\mathrm{F}\), \(V_b=250\,\mathrm{V}\) (Ra raised slightly for stage gain into plate clip). Grid clamp ≈ \([-8,+1]\,\mathrm{V}\). No NFB.

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
| `devices/TriodeStage.h` | Common-cathode Newton consumer (live) |
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

`tube_curve_verify` locks −18 RMS in 0…+11 dB @ Drive 0.5/1, harmonic density Drive 1 ≫ 0.5, and AX7 > AU7 RMS. `tube_aliasing_verify` mirrors the diode AA proxy (1× vs 4× at 5 kHz).

## Later

- Coupling HPF / Miller if needed
- Richer stage — still not a Champ amp
