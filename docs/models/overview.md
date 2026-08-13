# Models overview

SaturationStudio uses **topology families**, not named guitar pedals.

| Family | Teaches toward tube amps | v1 |
|--------|--------------------------|----|
| **Diode** | Device I–V, clipper networks, Newton | Shockley `DiodeDevice` + static antiparallel clipper |
| **Tube** | Smooth transfer, even harmonics, eventual triode stages | Asymmetric tanh ADAA soft-clip |
| **Tape** | Soft clip + HF loss / compression character | Soft symmetric algebraic ADAA |
| **Transformer** | Hysteresis / memory nonlinearities | Asymmetric algebraic ADAA (no hysteresis yet) |
| **Preamp** | Console drive curves + iron (1073 / API flavors) | Asymmetric tanh flavors (1073 / API) |

Clippers stay separable so each nonlinearity stays inspectable and reusable when composing larger stages.

## Skill map

1. Diode device + static Newton clipper → confidence composing diode networks  
2. Tube transfer → triode / power stage intuition  
3. Transformer hysteresis → OT / iron behavior  
4. Preamp flavors → channel-strip character without boiling the ocean  
5. Later: dynamic networks (C, feedback) reusing the same devices
