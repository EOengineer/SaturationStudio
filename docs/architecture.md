# Architecture

## Signal path

```text
Input
  → LinearPhaseBandSplit (HP then LP on mid; side = delayed − mid)
  → mid → 4× Oversample ↑ → SaturationModel → Oversample ↓
  → side delayed by OS latency
  → mid mix (wet/dry) + side
  → output gain (dB)
  → SpectrumAnalyzer (UI)
```

`SaturationEngine` owns the split, oversampler, active model, and mix/output.

Latency reported to the host = FIR cascade group delay + oversampler latency.

## Parameters (APVTS)

| ID | Role |
|----|------|
| `lowCutHz` / `highCutHz` | Linear-phase band edges |
| `drive` | Reserved for clippers (ignored by stubs) |
| `mix` | Wet amount on mid band |
| `outputDb` | Makeup ±24 dB |
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
| Band split | `Source/dsp/LinearPhaseBandSplit.h`, `FirDesign.h` |
| OS | `Source/dsp/Oversampler.h` |
| Models | `Source/dsp/models/` |
| UI | `Source/ui/` |
