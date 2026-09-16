# TestGen

A predictable test signal: a sine at one frequency, or white, pink or brown noise.

Use it to check a signal path, to set levels before there is any music, or to hear a filter's shape on its own: a sine shows what a Filter does to one frequency, and noise contains every frequency at once and shows what it does to all of them. It takes no input and sends the same signal to both outlets, so one organism feeds a stereo chain. A pure sine reads far higher on a meter than it sounds, so trust a VuMeter rather than your ears.

## Parameters

**Amplitude** Output level. Start low; a full-scale sine into an unknown chain is how monitors get damaged.

**Frequency** Pitch of the sine, 10 Hz to 20 kHz. Ignored while a noise waveform is selected.

**Waveform** Sine is one frequency and nothing else. White noise has equal energy per hertz, Pink noise equal energy per octave, and Brown noise falls off faster still for a low rumble.

## Recipe

**Filter check** Waveform White noise, Amplitude 0.3. Cord it into a Filter, the Filter into a Spectrum and on to a SoundOut. Sweep the Filter's Frequency and the Spectrum draws the curve the filter is applying.

## Related Organisms

Wave, Rhizome, VuMeter, Spectrum
