# Rhizome

A rhizome is a root that spreads sideways by subdividing. This one grows on the tuning: a played note sends out runners - further voices standing Interval scale degrees above the root, read from the patch's tuning rather than from the harmonic series.

That is what makes it different from Substrate and Mineral. Their intervals are numbers (a fifth is always 1.5), so they sound the same in any tuning. Rhizome's intervals are degrees, so the scale is not just what it is played in - it is what it is made of. Retune the patch and the timbre moves, not only the pitch.

## Parameters

**Wave** The runner's waveform: sine, saw or square.

**Runners** How many shoots grow from the root (1 = just the root).

**Degrees** The gap between consecutive runners, in scale degrees.

**Creep** Slow independent pitch drift per runner, in cents. Roots never grow straight, and it stops the stack phase-locking.

**Tilt** The level slope along the chain. Low keeps the growth close to the root; high lets the far runners sing.

**Bloom** How long a note takes to open.

**Decay** How long it takes to die away after release.

**Cutoff** A lowpass over the sum - a tall stack of saws is a lot.

**Level** Output level.

Six voices, MIDI-driven (PianoRoll, DNA, or the QWERTY keyboard when this editor is focused). Runners alternate left and right for width.

Try: Degrees 1, Runners 6, Creep 0, sine - a cluster of adjacent degrees that beats at the scale's own step size. In 12-TET it is a semitone pile; in 19-EDO it is something else entirely. This is the setting that makes a tuning audible as a texture rather than as a tuning.

## Related Organisms

Wave, Mineral, Substrate
