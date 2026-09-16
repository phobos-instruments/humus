# SpectralMorph

Blends the spectra of two inputs so one timbre passes through the other rather than playing on top of it.

Per band the level is interpolated between source A and source B and the phase is taken from whichever contributes more there, then the result is resynthesised. In Stereo mode inlets 1 and 2 are A and 3 and 4 are B; in Mono mode inlet 1 is A and inlet 2 is B, so re-check the cords after switching Mode. An unconnected B turns Morph into a fade to silence. Sustained, harmonically rich sources morph best: cord a Substrate against a Sampler vocal and sweep Morph from an LFO or a Metapad.

## Parameters

**Morph** 0 is pure A, 1 is pure B, and everything between is a spectral blend of the two.

## Recipe

**Voice into pad** Cord a sung vowel from SoundIn to inlets 1 and 2 and a Substrate chord to inlets 3 and 4. Cord an LFO onto Morph with a period of eight bars so the voice turns into the pad and back. Keep both sources playing for the whole sweep.

## Related Organisms

SpectralFilter, SpectralFreeze, SoundSpace, Mixer
