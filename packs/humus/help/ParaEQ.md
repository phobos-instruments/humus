# ParaEQ

The four-band equalizer. A shelf at each end to tilt the whole sound, and two sweepable bands in the middle to reach in and move one thing. Where the Filter is an instrument you play, this is a tool you aim: for deciding what a sound should be made of, not for making it sing.

Use it to take the mud out of a bass, to find the one ringing frequency in a room recording and pull it down, or to lift air into something dull.

The most useful move it makes is usually a cut, not a boost - a small cut where two parts are fighting does more for a mix than a boost on either of them. The two shelves affect everything past their corner frequency, so they tilt; the two mid bands affect a region around their centre, so they sculpt. All four run in series on each channel.

## Parameters

**Mode** switches stereo and mono in place, keeping the name, the settings and the automation.

**LSCutoffFrequency** the low shelf's corner. Everything below it is lifted or cut together.

**LSGain** how much, in dB.

**BP1CenterFrequency** the first mid band's centre.

**BP1Bandwidth** how wide it reaches, in Hz. Narrow is a scalpel for one resonance; wide is a tonal decision.

**BP1Gain** how much, in dB. Negative is a notch.

**BP2CenterFrequency** the second mid band's centre.

**BP2Bandwidth** its width, in Hz.

**BP2Gain** its cut or boost, in dB.

**HSCutoffFrequency** the high shelf's corner. Everything above it moves together.

**HSGain** how much, in dB.

## How it works

Bandwidth is given in Hz rather than as a Q number, which is the more useful of the two to think in: 200 Hz wide is 200 Hz wide wherever you put the band. The same bandwidth still sounds broader down low than it does up high, since it covers more of the octave there.

To find a problem frequency, boost a narrow band hard and sweep it until the sound gets worse. That is the frequency. Now cut it instead.

## Related Organisms

Filter, Prism, SpectralFilter
