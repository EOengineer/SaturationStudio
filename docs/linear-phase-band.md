# Linear-phase band split

## Why linear phase

Minimum-phase IIR HP/LP are cheap and low-latency, but they shift phase differently across frequency. For transparent “saturate only this band” work—especially before multiband—linear-phase FIRs keep transient timing aligned across the spectrum.

Cost: **latency** ≈ `(N-1)/2` per FIR. We cascade HP then LP → about **1022 samples** at `N = 1023` (~21 ms @ 48 kHz), plus any convolution-engine latency. Fine for mixing; later we can offer a min-phase mode for tracking/live.

## Design

`fir::designLowpass` / `designHighpass` in `FirDesign.h`:

- Odd-length Kaiser-windowed sinc
- HP via spectral inversion of an LPF at the same cutoff
- Direct-form `FirFilter` + `DelayLine` for matching (**offline verifies**)

**Realtime plugin path** (`LinearPhaseBandSplitRT.h`): HP⋆LP cascaded into one bandpass IR, loaded into **two** convolvers (pre-split + post-sat). Cutoff changes update both IRs together.

`LinearPhaseBandSplit` / RT:

1. `mid = bandpass_pre(x)` with `bandpass = HP⋆LP`
2. `delayed = Delay(x, latency)`
3. `outOfBand = delayed − mid`
4. After saturation (in the engine): `wet = bandpass_post(sat(mid))` so new harmonics stay in-band when summed with `outOfBand`

Algebraically, `mid + outOfBand == delayed` always. Musical transparency when the band is wide open means **mid ≈ delayed** (side energy near zero)—locked by the offline verify.

## Multiband roadmap

For 2–5 bands, prefer **complementary linear-phase crossover banks** (explicit LR-style FIR pairs) so every band sums to a pure delay without relying on delayed−mid per band. The current single mid/side split is the one-band special case.
