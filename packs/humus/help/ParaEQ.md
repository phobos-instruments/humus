# ParaEQ

A four-band equalizer with a shelf at each end and two sweepable bands in the middle.

The two shelves lift or cut everything past their corner, so they tilt the whole sound; the two mid bands work on a region around their centre, so they reach in and move one thing. All four run in series on each channel. Cord it after a Sampler, a FilePlayer or a Mixer bus to take mud out of a bass, to pull down one ringing frequency in a room recording, or to add air to something dull. A small cut where two parts fight usually does more than a boost on either. Bandwidth is given in hertz, so 200 Hz wide is 200 Hz wide wherever the band sits, though the same width covers more of the octave down low.

## Parameters

**HSCutoffFrequency** The high shelf's corner. Everything above it moves together.

**HSGain** The high shelf's cut or boost in dB.

**BP1CenterFrequency** The centre of the first mid band.

**BP1Bandwidth** How wide the first band reaches, in Hz. Narrow for one resonance, wide for a tonal decision.

**BP1Gain** The first band's cut or boost in dB. Negative is a notch.

**BP2CenterFrequency** The centre of the second mid band.

**BP2Bandwidth** The second band's width in Hz.

**BP2Gain** The second band's cut or boost in dB.

**LSCutoffFrequency** The low shelf's corner. Everything below it moves together.

**LSGain** The low shelf's cut or boost in dB.

## Recipe

**Finding a resonance** Set BP1Gain to 12 and BP1Bandwidth to 60, then sweep BP1CenterFrequency until the sound gets worse. That is the frequency: set BP1Gain to -6 there and widen the band until the cut stops sounding like a hole.

## Related Organisms

Filter, Prism, SpectralFilter
