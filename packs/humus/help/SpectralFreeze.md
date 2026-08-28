# SpectralFreeze

A phase-vocoder freeze and smear. The point of this organism is the SOUND: rather than a dead magnitude hold that sits there like a screenshot, the frozen spectrum stays alive and keeps evolving. Every control is continuous, so the whole effect morphs beautifully from the Metapad.

Feed it a pad, a vocal or a room and you can lift a single moment out of it and hold that moment forever, still breathing.

## Parameters

**Mode** switches stereo and mono in place. Each channel keeps its own spectral state, so the image is preserved rather than collapsed.

**Smear** magnitude sustain across analysis frames, from about 1 ms to 10 s. At 0 it is transparent; as it climbs toward 1 the sound grows an ever-lengthening spectral tail. This is the knob to automate.

**Freeze** a latch: grab the current spectrum and hold it, ignoring the input from then on.

**Diffusion** blurs magnitude across bins and adds a slow per-frame phase random walk, so the freeze blooms and breathes instead of sitting perfectly still.

**Shimmer** folds an octave-up copy of the held spectrum back in - glow.

**Mix** dry/wet blend.

## Technical discussion

A naive freeze holds each bin's magnitude and invents a phase, which is why most of them sound static and metallic. Here each bin's true per-hop phase advance is measured from the input and then integrated on output, so held partials keep their real micro-detuning and beat against each other naturally - the freeze has motion because the source had motion. The output crossfades from the input's own phase (transparent) to that integrated phase (frozen) as the effective "frozenness" rises, so there are no clicks on the way in, and Smear 0 is an exact pass-through rather than an approximation.

## Notes

Any Mix stays phase-coherent, and parallel cords line up on their own. Like anything spectral it carries real delay, so it is not one to monitor a live input through.

## Usage

For a held drone, ride Smear up and then latch Freeze at a moment you like. For a texture, leave Freeze off and let Smear alone stretch the source. Add Diffusion once it is held, or it will smear the source before you catch it.

## Related Organisms

SpectralFilter, SpectralMorph, SoundSpace, Substrate
