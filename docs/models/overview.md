# Models overview

SaturationStudio uses **topology families**, not named guitar pedals.

| Family | Teaches toward tube amps | v1 |
|--------|--------------------------|----|
| **Diode** | Device I–V, clipper networks, Newton, reactive C | Shockley `DiodeDevice` + antiparallel + shunt-C clipper |
| **Tube** | Smooth transfer, even harmonics, eventual triode stages | Asymmetric tanh ADAA soft-clip |
| **Tape** | Soft clip + HF loss / compression character | Soft symmetric algebraic ADAA |
| **Transformer** | Hysteresis / memory nonlinearities | Asymmetric algebraic ADAA (no hysteresis yet) |
| **Preamp** | Console drive curves + iron (1073 / API flavors) | Asymmetric tanh flavors (1073 / API) |

Clippers stay separable so each nonlinearity stays inspectable and reusable when composing larger stages.

## Skill map

1. Diode device + static Newton clipper — done  
2. Dynamic diode network (shunt C, trap companion) — current  
3. Tube transfer → triode / power stage intuition  
4. Transformer hysteresis → OT / iron behavior  
5. Preamp flavors → channel-strip character without boiling the ocean  
6. Later: richer nets (feedback) and Tube-as-device, reusing the same patterns
