# Acid

A mono line synth in the acid-box spirit: band-limited saw or square into a resonant diode ladder swept by an exponential envelope, with the two tricks that make lines speak. The ladder and the output stages that shape its low end are our own version of the ones in Open303 by Robin Schmidt.

**Accent** Notes with velocity 110+ hit harder - the filter opens further and the note bites louder, and runs of them bloom: consecutive accents stack, the way the original hardware's do. A drawn note is velocity 100, so it is plain; select it and press Ctrl+Up once to accent it. Accent a few notes in a line, not all of them - the effect is the contrast.

**Slide** Overlap two notes (legato) and the pitch glides between them without retriggering the envelope - the classic acid slide. Glide sets the slide time, the time the pitch takes to arrive.

## Parameters

**Wave** Oscillator shape - saw, square, pulse or triangle.

**Cutoff** Filter base frequency; the envelope sweeps up from here.

**Resonance** Filter emphasis - the squelch.

**EnvMod** How far the envelope sweeps the cutoff, in octaves.

**Decay** How fast the sweep falls back.

**Accent** How much harder accented notes hit. A note is accented at velocity 110 and above, or by its velocity when Velocity is up.

**Glide** Slide time between overlapped notes, in ms: how long a slide takes to arrive, not a time constant. 60 ms is the box.

**Drive** Saturation on the way out.

**Sustain** Where a held note settles once its AmpDecay has run: 0 is the box, which always dies away; up, the note holds until you let go.

**Squelch** How much bass the filter's feedback is allowed to eat. Low keeps the low end under heavy resonance; high thins it into the classic squelch. 0.8 is the box.

**Velocity** How much velocity shapes the accent. At 0 an accent is all or nothing above velocity 110, as on the box; at 1 every velocity sets its own accent amount, and the switch at 110 is gone.

**Punch** The filter envelope also lifts the volume, more on accents. This is part of why accented notes jump; 0 turns it off.

**Muffler** A lowpass on the way out, to tame the top when Drive is up.

**LfoWave/Rate/Depth** An extra wobble on the cutoff - free-running in Hz, or beat-synced via LfoSync and LfoBeats.

**Level** Output volume.

Sequence it from a PianoRoll cord; with its editor focused, the QWERTY keyboard plays it directly.

## Related Organisms

Riff, Steps, Microdot
