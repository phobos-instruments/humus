# Flanger

The jet. A copy of the sound is delayed by a very short time and mixed back in, which cancels some frequencies and reinforces others in a row of evenly spaced notches. Sweep that delay and the whole comb slides up and down the spectrum. The sound is about seventy years old: it came from two tape machines playing the same thing while someone pressed a thumb on the flange of one reel to slow it down.

Use it on drums to make a fill take off, on a sustained chord to give it somewhere to go, or on noise, where the moving comb is the only thing giving the noise any pitch at all.

## The family

Chorus, flanger and phaser are one idea at three scales. Chorus delays far enough to hear as detuning. Flanger delays so briefly that the copy combs against the original instead. Phaser makes its notches by shifting phase rather than delaying, which is why its notches are fewer and less evenly spaced, and why it swooshes where a flanger shrieks.

## Parameters

**FrequencyRange** the two ends of the sweep, as frequencies. The comb's delay tracks the reciprocal, so a low frequency here is a long delay and a dense comb, while a high one is a short delay and a sparse, whistling comb. Setting the two handles close together gives a fixed metallic colour rather than a sweep.

**Rate** how fast it travels between them.

**Feedback** returns the delayed copy to the input, sharpening every notch and deepening every peak. Low is a gentle whoosh, high is the screaming resonant version.

**WetDryMix** how much of the delayed copy is heard against the original. The comb only exists because the two are heard together, so full wet removes the effect rather than maximising it. Halfway is where the cancellation is deepest.

## How it works

A single delay is swept by an LFO shared by both channels. Feedback is what turns a shallow filter into a resonant one: without it the notches are broad scoops, and with it each pass round the loop reinforces the same peaks until they ring.

The wet/dry note above is the one that catches people. A flanger is not an effect applied to a sound; it is the interference between a sound and itself. Take either half away and there is nothing left to interfere.

## Related Organisms

Chorus, Phaser, Fern
