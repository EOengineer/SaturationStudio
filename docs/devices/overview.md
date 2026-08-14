# Devices — physically modeled component library

SaturationStudio is building an **OO physically modeled parts shelf** under `Source/dsp/devices/`: reusable diodes, triodes, and later transformers / other parts. **Clippers and saturators compose those parts**; they are not where long-term device physics lives.

```text
Device (library part)     → stamp I / G, typed params, no Drive / OS / UI
Circuit consumer          → Newton / KCL net that stamps devices
SaturationModel           → product path (Drive, makeup, OS island, flavors)
```

Proven stacks:

```text
DiodeDevice → FeedbackDiodeClipper → DiodeModel (live)
TubeDevice  → TriodeStage          → TubeModel (live)
```

## What a device is

| Rule | Meaning |
|------|---------|
| Topology-agnostic | No saturator Drive maps, oversampling, or APVTS |
| Stampable | Exposes current and conductances suitable for Newton islands |
| Factory params | Named part sets (`siliconSignal()`, `twelveAx7()`, …) |
| JUCE-free | Offline verifies can exercise parts without the plugin |

## Parts shelf (now / next)

| Part | Role | Status |
|------|------|--------|
| [`DiodeDevice`](../../Source/dsp/devices/DiodeDevice.h) | Shockley junction + bulk \(R_s\) | Live — used by diode clippers |
| [`TubeDevice`](../../Source/dsp/devices/TubeDevice.h) | Koren triode \(I_p(V_{gk}, V_{ak})\) + conductances | Library part |
| [`TriodeStage`](../../Source/dsp/devices/TriodeStage.h) | Common-cathode 3×3 Newton (Rg, grid diode, Cgp/Cgk, Cc coupling) | Live via `TubeModel` |
| [`NewtonSolver`](../../Source/dsp/devices/NewtonSolver.h) | Tiny dense Newton helper | Shared by nets |
| Transformer / iron | Hysteresis / magnetics | Later |
| Power tubes, etc. | Beam / pentode laws | Later |

## Consumers vs parts

| Layer | Examples |
|-------|----------|
| **Parts** | `DiodeDevice`, `TubeDevice` |
| **Circuit consumers** | `FeedbackDiodeClipper`, `TriodeStage` |
| **Product saturators** | `DiodeModel`, Newton `TubeModel`, Tape / Transformer / Preamp curves |

Live **Tube** UI flavors (12AX7 / 5751 / 12AU7) select `TubeDevice` factories. Parked waveshape `TubeCurve` is unused by the live path.

## Explicit non-goal

**No full Champ (or other complete amp)** inside SaturationStudio — not V1A+V1B+NFB+6V6+OT+cab as a product target. AmpStudio remains the place for full amp islands. Here we grow **reusable parts** and **compose them into clippers / teaching saturators** that could later feed cutting-edge circuit models elsewhere.

## Related docs

- [`docs/models/diode.md`](../models/diode.md) — diode part → clipper → model
- [`docs/models/tube.md`](../models/tube.md) — live TriodeStage Tube path
- [`docs/architecture.md`](../architecture.md) — plugin signal path
- [`docs/models/overview.md`](../models/overview.md) — family skill map
