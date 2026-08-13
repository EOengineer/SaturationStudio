# Architecture

## Signal path

```text
Input
  → LinearPhaseBandSplit BP (pre)
  → mid → 4× OS ↑ → SaturationModel → OS ↓ → BP (post, same IR)
  → side / dry-mid delayed by (OS + post-BP)
  → mix(wet mid, dry mid) + side
  → Band makeup (outputDb)
  → SpectrumAnalyzer (UI)
```

Post-BP keeps clipper harmonics inside the selected band so they don’t interfere with the bypassed out-of-band path (filters + high Mix used to sound phasey).

`SaturationEngine` owns the split, oversampler, active model, mix, and **band** `outputDb`.

Latency reported to the host = FIR cascade group delay + oversampler latency.

## Level contract

| Stage | Meaning |
|-------|---------|
| Plugin input RMS ≈ **−18 dBFS** | Modeling reference (`LevelReference::kReferenceRmsDb`) |
| Drive (Diode / Tube) | Input gain into the knee; ~0.5 = mostly clean at −18; makeup stabilizes loudness |
| Band (`outputDb`) | Makeup for the saturated band (becomes per-band later) |

Hit −18 dBFS with **DAW clip/channel gain** into the plugin for now. In-plugin global trim / master / I/O meters are out of tree until we revisit them.

## Parameters (APVTS)

| ID | Role |
|----|------|
| `lowCutHz` / `highCutHz` | Linear-phase band edges |
| `drive` | Diode / Tube: push into clip knee (0…~24 dB). Other families: reserved |
| `mix` | Wet/dry blend on mid band (0 = delayed dry mid, 1 = saturated) |
| `outputDb` | **Band** makeup ±24 dB |
| `satModel` | Family: Diode, Tube, Tape, Transformer, Preamp |
| `diodeFlavor` | Silicon / Germanium / LED / Asymmetric |
| `preampFlavor` | Neve 1073 / API 512 |

Cutoff invariant: `lowCutHz < highCutHz` (clamped in engine / processor).

## Model plug-in

`SaturationModel` is a small interface (`prepare` / `reset` / `process` at oversampled rate).  
`ModelRegistry` lists families + flavors for the UI and constructs instances.

Default: **Diode / Silicon** — Shockley antiparallel clipper (see `docs/models/diode.md`). Tube / Tape / Transformer / Preamp remain ADAA waveshapers (`docs/models/*.md`).

## Key source map

| Piece | Path |
|-------|------|
| Processor | `Source/PluginProcessor.*` |
| Engine | `Source/dsp/SaturationEngine.h` |
| Band split | `Source/dsp/LinearPhaseBandSplit.h`, `LinearPhaseBandSplitRT.h`, `FirDesign.h` |
| Level contract | `Source/dsp/LevelReference.h` |
| OS | `Source/dsp/Oversampler.h` |
| Models | `Source/dsp/models/` |
| UI | `Source/ui/` |
