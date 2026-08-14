# Models overview

SaturationStudio uses **topology families**, not named guitar pedals.

| Family | Teaches toward tube amps | v1 |
|--------|--------------------------|----|
| **Diode** | Device I–V, Newton nets, reactive C, feedback OA | Shockley `DiodeDevice` (junction + bulk Rs) + ideal-OA feedback clipper |
| **Tube** | Smooth transfer, even harmonics, eventual triode stages | Asymmetric tanh ADAA soft-clip |
| **Tape** | Soft clip + HF loss / compression character | Soft symmetric algebraic ADAA |
| **Transformer** | Hysteresis / memory nonlinearities | Asymmetric algebraic ADAA (no hysteresis yet) |
| **Preamp** | Console drive curves + iron (1073 / API flavors) | Asymmetric tanh flavors (1073 / API) |

Clippers stay separable so each nonlinearity stays inspectable and reusable when composing larger stages.

## Skill map

1. Diode device + static Newton clipper — done  
2. Dynamic diode network (shunt C) — done  
3. Feedback diode clipper (ideal OA + FB diodes + Cf) — done  
4. Component-backed Drive (Rin from Drive) — done  
5. Diode bulk series Rs on `DiodeDevice` — done  
6. Tube transfer → triode / power stage intuition  
7. Transformer hysteresis → OT / iron behavior  
8. Preamp flavors → channel-strip character without boiling the ocean  
9. Later: richer pedal islands / Tube-as-device; diode junction C stays on the part (see `docs/models/diode.md`)
