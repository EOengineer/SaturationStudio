# Architecture

## Signal path

```text
Input
  → LinearPhaseBandSplit (HP then LP on mid; side = delayed − mid)
  → mid → 4× Oversample ↑ → SaturationModel → Oversample ↓
  → side delayed by OS latency
  → mid mix (wet/dry) + side
  → Band makeup (outputDb)   ← future per-band
  → SpectrumAnalyzer (UI)
```

`SaturationEngine` owns the split, oversampler, active model, mix, and **band** `outputDb`.

Latency reported to the host = FIR cascade group delay + oversampler latency.

## Level contract

| Stage | Meaning |
|-------|---------|
| Plugin input RMS ≈ **−18 dBFS** | Modeling reference — author diode/tube curves assuming this level into the sat (`LevelReference::kReferenceRmsDb`) |
| Band (`outputDb`) | Makeup for the saturated band (becomes per-band later) |

Hit −18 dBFS with **DAW clip/channel gain** into the plugin for now. In-plugin global trim / master / I/O meters are out of tree until we revisit them.

## Parameters (APVTS)

| ID | Role |
|----|------|
| `lowCutHz` / `highCutHz` | Linear-phase band edges |
| `drive` | Reserved for clippers (ignored by stubs) |
| `mix` | Wet amount on mid band |
| `outputDb` | **Band** makeup ±24 dB |
| `satModel` | Family: Diode, Tube, Tape, Transformer, Preamp |
| `diodeFlavor` | Silicon / Germanium / LED / Asymmetric |
| `preampFlavor` | Neve 1073 / API 512 |

Cutoff invariant: `lowCutHz < highCutHz` (clamped in engine / processor).

## Model plug-in

`SaturationModel` is a small interface (`prepare` / `reset` / `process` at oversampled rate).  
`ModelRegistry` lists families + flavors for the UI and constructs instances.

v1 default: **Diode / Silicon** identity stub.

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
