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
TriodeStage    → Drive→Vdrive, Rg, grid diode, Cgp/Cgk, load, coupling  — circuit
TubeModel      → makeup / −18 loudness contract
```

## Signal path (inside OS island) — live

```text
x → Drive→Vdrive → Rg → Vg (3×3 Newton with Vp,Vk; Ig, Cgp, Cgk, Ck)
  → AC plate scale → Cc coupling HPF → makeup → y
```

`Drive ≈ 0` hard-bypasses to identity (no stage processing).

Teaching stage defaults:

| Part | Value | Role |
|------|-------|------|
| \(R_a\) | \(120\,\mathrm{k}\Omega\) | Plate load |
| \(R_k\) / \(C_k\) | \(1.5\,\mathrm{k}\Omega\) / \(22\,\mu\mathrm{F}\) | Cathode bias + bypass |
| \(V_b\) | \(250\,\mathrm{V}\) | B+ |
| \(R_g\) | \(68\,\mathrm{k}\Omega\) | Grid stopper from Vdrive |
| \(R_\mathrm{leak}\) | \(1\,\mathrm{M}\Omega\) | Grid leak to 0 V |
| \(C_{gp}\) / \(C_{gk}\) | \(1.6\,\mathrm{pF}\) each | Interelectrode C (Miller emerges from Cgp) |
| Grid diode | teaching Shockley | Soft grid conduction (replaces hard clamp) |
| \(C_c\) / \(R_l\) | \(22\,\mathrm{nF}\) / \(1\,\mathrm{M}\Omega\) | Output coupling HPF (~7 Hz) |

Newton unknowns: **Vp, Vk, Vg** (3×3). No NFB.

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
| Drive **~0.5** | Warm; RMS(out) roughly **0…+13 dB** vs input |
| Drive **1** | Clearly more saturated than 0.5 (higher H2…H5) |
| AU7 @ **0.5** | Quieter / cleaner than AX7 |

Drive→`Vdrive` map + makeup remain **plugin control / loudness** scaffolding (see Tech debt).

## Tech debt / teaching approximations

Track these so we do not pretend the stage is a full datasheet model:

- **Drive→`Vdrive` map** (`kMaxGridGain` / `kDriveCurve`) is phenomenological, not a pot/network. `|Vdrive|` is soft-limited (`tanh` to ~18 V) so Drive 1 stays Newton-friendly under 4× OS — not a substitute for input coupling-cap topology.
- **Grid diode** Is / nVt / Rs are teaching values — not measured 12AX7 Ig curves.
- **Audio Newton budget** is capped (≤8 iters, short line search) for realtime; DC settle uses a tighter solve.
- **Cgp / Cgk** are teaching capacitances (order-of-magnitude AX7-ish); not full socket/wiring C or variable Miller tables.
- **Makeup / plate scale** stabilize plugin loudness vs Diode — not circuit physics.
- **Parked `TubeCurve.h`** still in tree until a delete PR.
- **AA:** Newton Tube is OS-only (no ADAA on the plate path).
- **Out of scope:** NFB, second stage, Champ, power-tube laws.

## Sources

| File | Role |
|------|------|
| `devices/TubeDevice.h` | Koren triode library part |
| `devices/TriodeStage.h` | Common-cathode 3×3 Newton consumer (live) |
| `devices/DiodeDevice.h` | Teaching grid conduction diode stamp |
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

`tube_stage_verify` locks idle, coupling DC block, stamped-Miller HF≤mid, and soft grid conduction (finite Vg dig-in). `tube_curve_verify` locks −18 level/harmonic contract. `tube_aliasing_verify` mirrors the diode AA proxy.

## Later

- Refine teaching Ig / Cgp/Cgk from literature or measurements if host A/B asks
- Delete parked `TubeCurve.h`
- Richer stage — still not a Champ amp
