# Spectrum

A live spectrum analyser with a waterfall that passes the audio through untouched.

The upper pane is the spectrum right now, with a faint peak-hold line remembering the loudest each band has been. The lower pane is the waterfall, the same spectrum drawn through time with the newest row at the top. Frequency runs on a log scale from 30 Hz to 18 kHz and level rises from a -72 dB floor. Both channels are summed for the display and passed through unchanged, and in silence the display holds its last picture. Cord it after a Filter to see a sweep, or before the SoundOut to read the whole mix.

## Recipe

**Reading a mix** Cord it between the Mixer and the SoundOut. Two sounds fighting show as ridges in the same place, hiss shows as a floor that never goes dark, and a resonance in a Filter shows as a peak that moves. Solo Mixer channels one at a time while watching to find which one owns a ridge.

## Related Organisms

VuMeter, StereoTool, Prism, SpectralFilter
