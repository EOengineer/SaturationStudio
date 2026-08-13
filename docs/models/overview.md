# Models overview

SaturationStudio uses **topology families**, not named guitar pedals.

| Family | Teaches toward tube amps | v1 |
|--------|--------------------------|----|
| **Diode** | Device I–V, Newton nets, reactive C, feedback OA | Shockley `DiodeDevice` + ideal-OA feedback clipper |
| **Tube** | Smooth transfer, even harmonics, eventual triode stages | Asymmetric tanh ADAA soft-clip |
| **Tape** | Soft clip + HF loss / compression character | Soft symmetric algebraic ADAA |
| **Transformer** | Hysteresis / memory nonlinearities | Asymmetric algebraic ADAA (no hysteresis yet) |
| **Preamp** | Console drive curves + iron (1073 / API flavors) | Asymmetric tanh flavors (1073 / API) |

Clippers stay separable so each nonlinearity stays inspectable and reusable when composing larger stages.

## Skill map

1. Diode device + static Newton clipper — done  
2. Dynamic diode network (shunt C) — done  
3. Feedback diode clipper (ideal OA + FB diodes + Cf) — current  
4. Tube transfer → triode / power stage intuition  
5. Transformer hysteresis → OT / iron behavior  
6. Preamp flavors → channel-strip character without boiling the ocean  
7. Later: richer pedal islands / Tube-as-device
