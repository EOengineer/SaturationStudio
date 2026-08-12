# Models overview

SaturationStudio uses **topology families**, not named guitar pedals.

| Family | Teaches toward tube amps | v1 |
|--------|--------------------------|----|
| **Diode** | Device I–V feel, soft vs hard knee, asymmetry | Silicon ADAA soft-clip + flavors |
| **Tube** | Smooth transfer, even harmonics, eventual triode stages | Asymmetric tanh ADAA soft-clip |
| **Tape** | Soft clip + HF loss / compression character | Soft symmetric algebraic ADAA |
| **Transformer** | Hysteresis / memory nonlinearities | Asymmetric algebraic ADAA (no hysteresis yet) |
| **Preamp** | Console drive curves + iron (1073 / API flavors) | Asymmetric tanh flavors (1073 / API) |

AmpStudio’s Tube Screamer remains the place for a full **pedal** white-box. Here we keep clippers separable so each nonlinearity stays inspectable and reusable when you move to Champ-style amp stages.

## Skill map

1. Diode curves + OS island → confidence with Newton/MNA clippers  
2. Tube transfer → Champ triode / power stage intuition  
3. Transformer hysteresis → OT / iron behavior  
4. Preamp flavors → channel-strip character without boiling the ocean
