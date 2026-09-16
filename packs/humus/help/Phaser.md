# Phaser

A six-stage phaser whose notches sweep between two frequencies.

A chain of all-pass filters leaves the level alone and shifts only phase; mixed back with the dry signal, the places where the two disagree cancel into notches, and a sine sweep moves those notches between the ends of FrequencyRange. Because nothing is delayed in time, transients arrive when they arrived, so it can sit on a whole submix without smearing it. Where a Flanger's notches are evenly spaced and metallic, a phaser's are few and uneven, which is the subtler, more vocal sweep. Cord it after an electric piano, a pad or a Mixer bus.

## Parameters

**FrequencyRange** The low and high ends of the sweep. A narrow range is a slow tonal wobble; a wide one is the full sweep from bottom to top.

**Rate** How fast the notches travel between the ends, in Hz.

**Feedback** Returns the output to the input, sharpening the notches into resonant peaks.

**Depth** How much of the phase-shifted signal is mixed with the dry one. Zero is dry; the deepest notches are near the middle, where the two are evenly matched.

## Recipe

**Slow pad sweep** FrequencyRange 200 to 2000, Rate 0.15, Feedback 0.5, Depth 0.5. Cord a sustained Rhizome pad in and the sweep takes about seven seconds to cross once; push Feedback towards 0.8 for a sharper peak.

## Related Organisms

Flanger, Chorus, Filter
