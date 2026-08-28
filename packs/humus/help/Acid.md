# Acid

A mono line synth in the acid-box spirit: band-limited saw or square into a resonant lowpass swept by an exponential envelope, with the two tricks that make lines speak:

**Accent** Notes with velocity 110+ hit harder - the filter opens further and the note bites louder, and runs of them bloom: consecutive accents stack, the way the original hardware's do. A drawn note is velocity 100, so it is plain; select it and press Ctrl+Up once to accent it. Accent a few notes in a line, not all of them - the effect is the contrast.

**Slide** Overlap two notes (legato) and the pitch glides between them without retriggering the envelope - the classic acid slide. Glide sets the slide time.

## Parameters

**Wave** Oscillator shape - saw, square, pulse or triangle.

**Cutoff** Filter base frequency; the envelope sweeps up from here.

**Resonance** Filter emphasis - the squelch.

**EnvMod** How far the envelope sweeps the cutoff, in octaves.

**Decay** How fast the sweep falls back.

**Accent** How much harder accented (velocity 110+) notes hit.

**Glide** Slide time between overlapped notes, in ms.

**Drive** Saturation on the way out.

**LfoWave/Rate/Depth** An extra wobble on the cutoff - free-running in Hz, or beat-synced via LfoSync and LfoBeats.

**Level** Output volume.

Sequence it from a PianoRoll cord; with its editor focused, the QWERTY keyboard plays it directly.

## Related Organisms

Riff, Steps, Microdot
