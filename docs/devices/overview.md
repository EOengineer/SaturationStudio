# Devices — physically modeled component library

SaturationStudio is building an **OO physically modeled parts shelf** under `Source/dsp/devices/`: reusable diodes, triodes, and later transformers / other parts. **Clippers and saturators compose those parts**; they are not where long-term device physics lives.

```text
Device (library part)     → stamp I / G, typed params, no Drive / OS / UI
Circuit consumer          → Newton / KCL net that stamps devices
SaturationModel           → product path (Drive, makeup, OS island, flavors)
```

Proven diode stack:

```text
DiodeDevice → FeedbackDiodeClipper (and earlier shunt nets) → DiodeModel
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
| [`TubeDevice`](../../Source/dsp/devices/TubeDevice.h) | Koren triode \(I_p(V_{gk}, V_{ak})\) + conductances | Library part — **not** yet stamped by a Tube saturator |
| Transformer / iron | Hysteresis / magnetics | Later |
| Power tubes, etc. | Beam / pentode laws | Later |

## Consumers vs parts

| Layer | Examples |
|-------|----------|
| **Parts** | `DiodeDevice`, `TubeDevice` |
| **Circuit consumers** | `FeedbackDiodeClipper`, future `TriodeStage` (common-cathode Newton) |
| **Product saturators** | `DiodeModel`, waveshape `TubeModel` (interim), Tape / Transformer / Preamp curves |

Live **Tube** UI flavors (12AX7 / 5751 / 12AU7) are still **waveshape stand-ins**. They should eventually select `TubeDevice` factories once a stage consumer exists.

## Explicit non-goal

**No full Champ (or other complete amp)** inside SaturationStudio — not V1A+V1B+NFB+6V6+OT+cab as a product target. AmpStudio remains the place for full amp islands. Here we grow **reusable parts** and **compose them into clippers / teaching saturators** that could later feed cutting-edge circuit models elsewhere.

## Related docs

- [`docs/models/diode.md`](../models/diode.md) — diode part → clipper → model
- [`docs/models/tube.md`](../models/tube.md) — interim waveshape Tube; device path
- [`docs/architecture.md`](../architecture.md) — plugin signal path
- [`docs/models/overview.md`](../models/overview.md) — family skill map
