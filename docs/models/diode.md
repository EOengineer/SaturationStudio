# Diode model

## v1 behavior

`DiodeModel` is an **identity passthrough**. Flavors (Silicon, Germanium, LED, Asymmetric) are stored for the UI/APVTS but do not change audio yet.

## Next implementation (AmpStudio map)

Study these before lifting ideas (do **not** copy the whole TS pedal EQ/tone path into this saturator):

| Topic | AmpStudio path |
|-------|----------------|
| Shockley diode I–V | `Projects/AmpStudio/Source/dsp/circuit/DiodeModel.h` |
| Oversampled MNA clipper island | `.../dsp/fx/ts/TsClippingAmp.h`, `TsEngine.h` |
| Shared OS wrapper | `.../dsp/circuit/Oversampler.h` |
| Geofex learning notes | `Projects/AmpStudio/docs/tube-screamer.md` |

Goal for SaturationStudio diodes: **memoryless (or lightly filtered) clip curves** selected by flavor, running inside our existing 4× island—learning the device physics that later shows up inside amp Newton islands.
