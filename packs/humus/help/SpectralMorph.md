# SpectralMorph

Blends the spectra of two inputs. This is not a crossfade: at any setting in between, the timbres cross through each other rather than simply playing at once. Per bin the magnitude is interpolated between the two sources and the phase is taken from whichever source contributes more there, then the result is resynthesised - so a voice can become a string rather than sit on top of one.

Morph is continuous and is a natural Metapad target: morph the morph.

## Inlets

In stereo mode there are four inlets, two per source:

**1-2** Source A (left, right)

**3-4** Source B (left, right)

In mono mode there are two inlets - A on the first, B on the second - and one outlet. Note that the inlets change meaning when you switch, so re-check your cords afterwards.

## Parameters

**Mode** switches between stereo and mono in place.

**Morph** 0 is pure A, 1 is pure B, and everything between is a genuine spectral blend.

## Usage

It rewards sources that are sustained and harmonically rich - pads, vowels, bowed strings, drones. Two percussive sources tend to just sound like two percussive sources, because there is little steady spectrum to interpolate. Try a held vocal against a pad and automate Morph slowly.

## Notes

Both inputs must be running for the blend to mean anything: an unconnected B makes Morph a fade to silence rather than a morph. A and B are fed identical framing so their analysis frames fire in lockstep, which is what guarantees each output frame blends the matching pair of frames whatever the audio settings. Like anything spectral it carries real delay, and parallel cords stay aligned on their own.

## Related Organisms

SpectralFilter, SpectralFreeze, SoundSpace, Mixer
