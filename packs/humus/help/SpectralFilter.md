# SpectralFilter

A spectral SHAPER rather than an EQ. An EQ applies a fixed curve; this looks at what the sound is actually doing frequency by frequency and shapes it relative to itself. Two continuous controls, both of them lovely Metapad morph targets.

Phase is left completely untouched - the output is the input scaled bin by bin - so at Tilt 0 and Contrast 0 the organism is transparent.

## Parameters

**Mode** switches stereo and mono in place; name, settings, cords and automation survive.

**Tilt** a frequency-dependent gain slope, up to about +/-18 dB across the band. Below 0 it darkens (lifts the lows, cuts the highs); above 0 it brightens. One knob for the whole tonal balance.

**Contrast** the interesting one - a spectral compressor. It raises each bin's magnitude against the local spectral envelope. Below 0 it smooths the spectrum toward its envelope: softer, more diffuse, more washed. Above 0 it sharpens peaks over valleys: crisper, more resonant, more articulated.

**Mix** dry/wet blend.

## Notes

The blend stays phase-coherent at any Mix, and parallel cords are compensated for, so a Mix below 1 will not comb-filter against a dry path elsewhere in the patch. Being spectral, this organism has real latency. It is not the one to reach for on a live-monitored input.

## Usage

Tilt is the fast tonal move: use it where you would reach for a broad shelf, but with one control. Contrast is where the character lives - pull it negative to push a busy sound back into a haze, or positive to make a dull pad suddenly articulate. Both are continuous with no discontinuities, so automating them or driving them from the Metapad sounds smooth rather than stepped.

Small positive values of Contrast are an unusual and effective way to add presence without an EQ boost, because they lift whatever is already peaking rather than a fixed frequency.

## Related Organisms

SpectralFreeze, SpectralMorph, Filter, Console
