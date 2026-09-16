# SpectralFilter

A spectral shaper that tilts the tonal balance and sharpens or smooths the spectrum against its own envelope.

Where an EQ applies a fixed curve, SpectralFilter measures the input band by band and scales each one relative to the local spectral envelope. Tilt is a one-knob brightness slope and Contrast is a spectral compressor: below zero it pulls peaks toward their surroundings, above zero it lifts them. Phase is left untouched, so at Tilt 0 and Contrast 0 it is transparent, and the dry path is delayed to match so any Mix stays phase-coherent. The Mode switch picks Stereo or Mono. It adds latency. Cord it after a Substrate, a Sampler or a whole Mixer.

## Parameters

**Tilt** A gain slope across the band, up to about 18 dB each way. Below zero darkens by lifting the lows and cutting the highs; above zero brightens.

**Contrast** How far each band is pushed away from or toward the spectral envelope. Negative smooths and washes a busy sound; positive sharpens peaks over valleys for a crisper, more articulate result.

**Mix** How much of the shaped signal is heard against the dry.

## Recipe

**Presence without a boost** Tilt 0, Contrast 0.3, Mix 1 on a dull pad from a Substrate. The bands that already peak are lifted rather than a fixed frequency, so the pad gains definition without a new resonance. Cord an LFO onto Contrast for a slow spectral breathing.

## Related Organisms

SpectralFreeze, SpectralMorph, Filter, Console
