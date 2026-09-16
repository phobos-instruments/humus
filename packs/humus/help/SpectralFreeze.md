# SpectralFreeze

A phase-vocoder freeze and smear that holds a spectrum while keeping it in motion.

Smear stretches how long each band's level persists across analysis frames, and Freeze latches the current spectrum and ignores the input until released. Held bands keep the phase advance measured from the source, so a frozen chord still beats and drifts. Diffusion blurs the held spectrum across neighbouring bands and adds a slow phase wander; Shimmer folds an octave-up copy back in. Smear 0 with Freeze off is an exact pass-through, and the dry path is delayed to match. Cord a Substrate, a Sampler or a vocal in and send the output on to a Verbatim or a Fern.

## Parameters

**Freeze** Grabs the spectrum playing right now and holds it, ignoring the input until switched off.

**Smear** How long each band's level persists, from about a millisecond to ten seconds. At 0 the input passes unchanged; higher grows a lengthening spectral tail. This is the knob to automate.

**Diffusion** Blurs the held levels across bands and adds a slow random phase walk, so a freeze blooms rather than standing still. Add it after freezing, or it smears the source before you catch it.

**Shimmer** Mixes an octave-up copy of the held spectrum back in.

**Mix** How much of the frozen signal is heard against the dry.

## Recipe

**Held drone** Smear 0.7, Diffusion 0.3, Shimmer 0.2, Mix 1. Cord a Sampler or a sustained pad in, play a chord, and switch Freeze on at the moment you want to keep; map Freeze to a Button or a MIDI note to grab it in time. Switch Freeze off to let the input through again.

## Related Organisms

SpectralFilter, SpectralMorph, SoundSpace, Substrate
