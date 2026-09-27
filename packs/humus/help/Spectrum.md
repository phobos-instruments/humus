# Spectrum

A live spectrum analyser with a waterfall. It only listens: it has no outlets, so cord a second cord into it from whatever you want to read.

The upper pane is the spectrum right now, with a faint peak-hold line remembering the loudest each band has been. The lower pane is the waterfall, the same spectrum drawn through time with the newest row at the top. Frequency runs on a log scale from 30 Hz to 18 kHz and level rises from a -72 dB floor. Both channels are summed for the display, and in silence the display holds its last picture. Cord a Filter's outlet into it to see a sweep, or whatever feeds the SoundOut to read the whole mix.

## Recipe

**Reading a mix** Cord the Mixer into it as well as into the SoundOut. Two sounds fighting show as ridges in the same place, hiss shows as a floor that never goes dark, and a resonance in a Filter shows as a peak that moves. Solo Mixer channels one at a time while watching to find which one owns a ridge.

## Related Organisms

Scope, VuMeter, StereoTool, Prism, SpectralFilter
