# Flanger

A swept short delay that combs the signal against a copy of itself.

A copy of the input is delayed by a fraction of a millisecond and mixed back in, which cancels some frequencies and reinforces others in a row of evenly spaced notches. A sine sweep moves that delay between the two ends of FrequencyRange, so the whole comb slides up and down the spectrum, and Feedback returns the delayed copy to the input so each notch sharpens into a resonance. Use it on drums, sustained chords or noise, where the moving comb is often the only pitch the sound has. Chorus delays longer and reads as detuning; Phaser makes fewer, unevenly spaced notches by shifting phase instead.

## Parameters

**FrequencyRange** The two ends of the sweep, as frequencies. A low frequency is a long delay and a dense comb, a high one a short delay and a sparse, whistling comb; set the handles close together for a fixed metallic colour instead of a sweep.

**Rate** How fast the sweep travels between the two ends, in cycles per second.

**Feedback** How much of the delayed copy returns to the input. Low is a gentle whoosh, high is the screaming resonant version.

**WetDryMix** How much of the delayed copy is heard against the original. The comb only exists where the two are heard together, so halfway is the deepest cancellation and full wet removes the effect.

## Recipe

**Jet fill** Place it after a Drums organism. FrequencyRange 100 to 3000, Rate 0.1, Feedback 0.7, WetDryMix 0.5. Cord an LFO onto Rate if you want the sweep itself to speed up and slow down across a phrase.

## Related Organisms

Chorus, Phaser, Fern
