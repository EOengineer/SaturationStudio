# SaturationStudio

JUCE **AU + VST3** plugin: frequency-selective analog-modeled saturation (multiband-ready shell).

v1 scaffold: linear-phase HP/LP band split, 4× oversampling island, topology model registry (default **Diode / Silicon** Shockley clipper), spectrum metering, aged hardware UI.

## What’s in this milestone

- One-band saturator with adjustable linear-phase low/high cuts
- **Diode** model: Shockley feedback clipper with Drive→`Rin`, Si/Ge/LED/Asym flavors
- **Tube** model: triode-ish ADAA waveshape; flavors **12AX7 / 5751 / 12AU7** (mu feel), same Drive/−18 contract
- **Tape / Transformer / Preamp** models: same methodology (symmetric soft / iron asym / 1073·API flavors)
- **Band** makeup (`outputDb`) — reserved for future per-band volume
- Modeling reference **−18 dBFS** at plugin input (`LevelReference`) — use DAW gain to hit it for now
- Model families: Diode, Tube, Tape, Transformer, Preamp (all live waveshapers)
- Diode flavors (Si / Ge / LED / Asym) and Preamp flavors (Neve 1073 / API 512) wired in UI
- Spectrum meter 20 Hz–20 kHz with in-band highlight + saturation heat placeholder
- Offline DSP verifies (`tools/`)
- Growing **physically modeled component library** (`Source/dsp/devices/`: diodes, `TubeDevice` triodes, …) composed into clippers — not a full Champ amp

**Level contract:** aim plugin input RMS near −18 dBFS when authoring/evaluating models. See [`docs/architecture.md`](docs/architecture.md).

All five family curves are live — see [`docs/models/`](docs/models/). Parts shelf: [`docs/devices/overview.md`](docs/devices/overview.md).

## Setup

### Prerequisites

- macOS with full **Xcode**
- Git

```bash
sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
xcodebuild -version
```

### Clone / submodule

JUCE is a git submodule at `libs/JUCE` (pinned to **8.0.8**).

```bash
git clone --recurse-submodules <repo-url>
cd SaturationStudio
# or, if already cloned:
git submodule update --init --recursive
ls libs/JUCE/modules/juce_core
```

### Generate Xcode project

```bash
/Applications/JUCE/Projucer.app/Contents/MacOS/Projucer --resave SaturationStudio.jucer
open Builds/MacOSX/SaturationStudio.xcodeproj
```

Build the **SaturationStudio - AU** and **SaturationStudio - VST3** schemes (or the aggregate).

**Use a Release build for Logic / DAW testing.** Debug builds are much heavier and can overload Logic even when Release is fine.

Realtime band filters use FFT convolution; **cutoff changes rebuild on the message thread** (AsyncUpdater) so twisting Low/High cut does not overload Logic.

```bash
# From Builds/MacOSX after Projucer --resave:
xcodebuild -project SaturationStudio.xcodeproj -scheme "SaturationStudio - AU" -configuration Release build
xcodebuild -project SaturationStudio.xcodeproj -scheme "SaturationStudio - VST3" -configuration Release build
```

### Install paths

| Format | Typical location |
|--------|------------------|
| AU | `~/Library/Audio/Plug-Ins/Components/SaturationStudio.component` |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/SaturationStudio.vst3` |

Xcode post-build copy may place them automatically depending on Projucer settings; otherwise copy from `Builds/MacOSX/build/...`.

### Offline verifies

```bash
clang++ -std=c++17 -O2 -I Source tools/band_split_verify_main.cpp -o tools/band_split_verify
./tools/band_split_verify

clang++ -std=c++17 -O2 -I Source tools/engine_passthrough_verify_main.cpp -o tools/engine_passthrough_verify
./tools/engine_passthrough_verify

clang++ -std=c++17 -O2 -I Source tools/diode_curve_verify_main.cpp -o tools/diode_curve_verify
./tools/diode_curve_verify

clang++ -std=c++17 -O2 -I Source tools/diode_aliasing_verify_main.cpp -o tools/diode_aliasing_verify
./tools/diode_aliasing_verify

clang++ -std=c++17 -O2 -I Source tools/tube_curve_verify_main.cpp -o tools/tube_curve_verify
./tools/tube_curve_verify

clang++ -std=c++17 -O2 -I Source tools/tube_device_verify_main.cpp -o tools/tube_device_verify
./tools/tube_device_verify

clang++ -std=c++17 -O2 -I Source tools/family_curves_verify_main.cpp -o tools/family_curves_verify
./tools/family_curves_verify
```

See [`docs/testing.md`](docs/testing.md).

## Host smoke checklist

1. Load in Logic / AU Lab (AU) and Reaper or Ableton (VST3)
2. Wide-open filters + Diode/Silicon → near bypass (latency compensated)
3. Move Low/High cut → spectrum in-band region highlights
4. Switch Model / Flavor → ParamHost updates; audio still passes

## Docs

| Doc | Topic |
|-----|--------|
| [`docs/architecture.md`](docs/architecture.md) | Signal path, params, models, component library |
| [`docs/devices/overview.md`](docs/devices/overview.md) | Physically modeled parts shelf (no full Champ) |
| [`docs/linear-phase-band.md`](docs/linear-phase-band.md) | FIR band split |
| [`docs/anti-aliasing.md`](docs/anti-aliasing.md) | Oversampling / ADAA |
| [`docs/testing.md`](docs/testing.md) | Verify suite |
| [`docs/models/overview.md`](docs/models/overview.md) | Topology roadmap → tube amps |
| [`docs/models/diode.md`](docs/models/diode.md) | Feedback OA clipper; Drive = Rin |
| [`docs/models/tube.md`](docs/models/tube.md) | Tube waveshape + TubeDevice part |
| [`docs/models/tape.md`](docs/models/tape.md) | Tape soft symmetric |
| [`docs/models/transformer.md`](docs/models/transformer.md) | Transformer iron (no hysteresis yet) |
| [`docs/models/preamp.md`](docs/models/preamp.md) | Preamp 1073 / API flavors |
