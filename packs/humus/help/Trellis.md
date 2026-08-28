# Trellis

A pitch corrector / autotuner. A trellis is the frame that trains a plant's growth along a line; this one trains a wandering pitch onto a scale. It tracks the incoming fundamental (the same YIN tracker the Decomposer uses), snaps it to the nearest note of the chosen Key and Scale, and shifts the audio there with a crossfading delay-line pitch shifter. Monophonic - one voice at a time, like every real-time tuner.

## Parameters

**Strength** How far toward the target the pitch is pulled. 0 leaves the performance untouched; 1 snaps fully onto the grid.

**Speed** How fast it glides to the target. Slow is transparent correction (the natural drift stays); fast is the hard robotic snap - the hard-tune radio-vocal effect at Speed near maximum.

**Key** The tonic (C.. B).

**Scale** The note grid the pitch is allowed to land on: Chromatic (every degree), the seven modes, Harmonic and Melodic Minor, both pentatonics, Blues, Whole Tone and Hirajoshi. The grid itself is the patch tuning's, so a Tuning organism set to 19-EDO puts these masks over 19 degrees, not 12.

**Mix** Dry / wet blend of the correction.

Feed a mono vocal or lead in, pick the song's key and scale, and set Strength to taste. For a natural tune, slow Speed and Strength around 0.6-0.8; for the effect, Chromatic scale, fast Speed, full Strength. The wet path carries a short delay (the shifter's window), so blend to taste rather than expecting a phase-perfect match with the dry. Pair it after a Decomposer to sing a synth in tune, or before a Deck vocal.

## Related Organisms

Decomposer, Tuning
