# Models overview

SaturationStudio uses **topology families**, not named guitar pedals. Longer-term device physics lives in the **component library** ([`docs/devices/overview.md`](../devices/overview.md)); families compose parts into saturators.

| Family | Teaches toward tube amps | v1 |
|--------|--------------------------|----|
| **Diode** | Device I–V, Newton nets, reactive C, feedback OA | Shockley `DiodeDevice` (junction + bulk Rs) + ideal-OA feedback clipper |
| **Tube** | Smooth transfer, even harmonics, eventual triode stages | Waveshape live; `TubeDevice` + offline `TriodeStage` Newton; live flip gated |
| **Tape** | Soft clip + HF loss / compression character | Soft symmetric algebraic ADAA |
| **Transformer** | Hysteresis / memory nonlinearities | Asymmetric algebraic ADAA (no hysteresis yet) |
| **Preamp** | Console drive curves + iron (1073 / API flavors) | Asymmetric tanh flavors (1073 / API) |

Clippers stay separable so each nonlinearity stays inspectable and reusable when composing larger stages.

**Not a SaturationStudio goal:** a full Champ (or other complete amp). Grow reusable parts; compose them into clippers.

## Skill map

1. Diode device + static Newton clipper — done  
2. Dynamic diode network (shunt C) — done  
3. Feedback diode clipper (ideal OA + FB diodes + Cf) — done  
4. Component-backed Drive (Rin from Drive) — done  
5. Diode bulk series Rs on `DiodeDevice` — done  
6. Tube waveshape flavors (12AX7 / 5751 / 12AU7) — done  
6b. `TubeDevice` (Koren library part) — done  
6c. One common-cathode Newton stage that stamps `TubeDevice` — **done offline** (`TriodeStage` + `tube_stage_verify`); live `TubeModel` flip gated  
6d. Wire `TubeModel` → `TriodeStage` + flavor→device factories — later  
7. Transformer hysteresis → OT / iron behavior  
8. Preamp flavors → channel-strip character without boiling the ocean  
9. Later: richer pedal islands; diode junction C on the part (see `docs/models/diode.md`)
