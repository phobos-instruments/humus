# TestGen

A known sound. Everything else in a patch is trying to be interesting; this is trying to be predictable, which is what makes it useful. A steady sine tells you what a filter is actually doing to one frequency. White noise contains every frequency at once, so it tells you what the filter is doing to all of them. When something in a patch sounds wrong and you cannot tell which organism is responsible, put this at the front and the answer usually arrives within seconds.

Use it to check a signal path, to set levels before there is any music to set them with, or to hear the shape of a filter sweep on its own.

Noise is a legitimate sound source in its own right, too: it is where wind, surf, breath and every cymbal come from, once something has shaped it.

## Parameters

**Waveform** sine or noise. Sine is one frequency and nothing else. Noise is all of them, at equal energy per hertz.

**Frequency** the pitch of the sine, from 10 Hz to 20 kHz. Ignored when the waveform is noise.

**Amplitude** the output level. Start low. A sine at full scale into an unknown chain is how monitors get damaged.

## How it works

It takes no input and emits two identical channels, so it feeds both sides of a stereo chain from one cord.

A note on listening: a pure sine is the hardest thing in audio to judge the loudness of, because there is nothing else in it for your ear to compare against. It will read far higher on a meter than it feels. Trust the VuMeter here, not your ears.

## Related Organisms

Wave, Rhizome, VuMeter, Spectrum
