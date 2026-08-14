# Tube model

Live **common-cathode Newton** tube saturator: `TubeDevice` (Koren \(I_p\)) stamped by `TriodeStage`, inside the 4× OS island.

```text
TubeDevice factory (flavor) → TriodeStage (per channel) → makeup → y
```

Waveshape `TubeCurve` (ADAA tanh) is **parked** in-tree and unused by `TubeModel`. See [`docs/devices/overview.md`](../devices/overview.md).

**Not in scope:** a full Champ amp in SaturationStudio.

## Signal path (inside OS island) — live

```text
x → Drive→grid AC gain → Newton common-cathode (Vp, Vk) → AC plate scale → makeup → y
```

`Drive ≈ 0` hard-bypasses to identity (no stage processing).

Teaching stage defaults: \(R_a=100\,\mathrm{k}\Omega\), \(R_k=1.5\,\mathrm{k}\Omega\), \(C_k=22\,\mu\mathrm{F}\), \(V_b=250\,\mathrm{V}\). No NFB.

## Flavors

| Flavor | Library part |
|--------|----------------|
| **12AX7** (default) | `devices::twelveAx7()` |
| **5751** | `devices::type5751()` |
| **12AU7** | `devices::twelveAu7()` |

## Level / Drive contract

| Setting | Intent |
|---------|--------|
| Plugin input ≈ **−18 dBFS** RMS | Modeling reference |
| Drive **0** | Transparent (identity bypass) |
| Drive **~0.5** | Warmth / mild saturation at −18 |
| Drive **1** | Clearly saturated; AU7 less dense than AX7 at the same knob |

Drive maps to **grid AC volts** inside `TriodeStage` (not a separate pre-gain on top of the stage). Mild makeup on `TubeModel` stabilizes loudness.

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

clang++ -std=c++17 -O2 -I Source tools/tube_device_verify_main.cpp -o tools/tube_device_verify
./tools/tube_device_verify

clang++ -std=c++17 -O2 -I Source tools/tube_stage_verify_main.cpp -o tools/tube_stage_verify
./tools/tube_stage_verify
```

## Later

- Makeup / plate-scale polish against −18 host listening
- Coupling HPF / Miller if needed
- Richer stage — still not a Champ amp
