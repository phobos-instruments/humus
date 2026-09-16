# Wave

A wavetable oscillator whose cycles are drawn, grown from a seed file or rolled at random.

The display is the sound: draw on it, start from the sine, triangle, saw or square tile, or seed the table from any file. A table holds up to sixteen frames and Position morphs between the two nearest; Warp reshapes the cycle, Unison stacks detuned copies, and a sub sine, drive and a lowpass finish the voice. Playback is band-limited per frame, so high notes lose only the harmonics that cannot fit. It is an eight-voice MIDI instrument that follows the patch Tuning; cord a PianoRoll, DNA or Steps in and send the output to a Filter or a Fern.

## The display

Drag on the wave to draw the frame nearest Position; the rest of a seeded table is untouched, so a grown wave can be retouched frame by frame. Random rolls a fresh spread of partials. Drop a file on the display or use Seed file: a sound file is chopped into up to sixteen frames, one cycle per slice, and any other file is read as a shape. + frame and - frame build a table by hand, and the strip under the display shows every frame with the current one lit; click a slice to jump to it. The same seed always grows the same table, saved with the patch.

## Parameters

**Attack** How long a note takes to open, in milliseconds.

**Release** How long it takes to fade after the key lifts.

**Sub** A pure sine one octave below every voice, for weight without changing the drawn cycle. 0 is off.

**Unison** How many detuned copies each voice plays, 1 to 7. One is dead centre; more thicken and widen.

**Detune** How far apart in pitch the unison copies sit.

**Width** How far the unison copies spread across the stereo field.

**Cutoff** A lowpass over the summed voices. Parked at the top of its travel it leaves the path entirely.

**Reso** Resonance of that lowpass.

**Drive** Saturation on the sum, before the filter, so Cutoff tames what it makes. 0 is clean.

**WarpMode** The cycle shaper: Off, Sync bends the read faster within the period, Bend skews the phase ramp, Fold reflects the peaks back into range, PWM squeezes the duty.

**Warp** Depth of the warp. It is smoothed, so sweeping it never zippers, and the shaped result is drawn as a second trace over the base cycle.

**Position** Scans and morphs between the table's frames. Does nothing on a one-frame table.

**Spectral** Morphs by harmonic magnitude rather than by samples, so a partial that sits in opposite phase between two frames fades instead of cancelling mid-sweep.

**PositionMod** How far the amp envelope sweeps Position on its own, per voice, so a note travels through the table as it plays. 0 is off.

**Level** Output level.

**BendRange** How far the pitch wheel reaches at full travel, in semitones. Two is the common default; zero ignores the wheel.

**Table** The table itself, saved with the patch. The dice rolls it along with the shaping knobs, and undo brings back the wave you had.

## Recipe

**Evolving sample table** Seed file with a short vocal recording, Unison 3, Detune 0.25, Width 0.7, Cutoff 6000, Drive 0.2, Release 800. Cord a PianoRoll in and an LFO onto Position with a period of two bars so each note travels through the recording as it plays; switch Spectral on if the sweep dips or phases.

## Related Organisms

Rhizome, Prism, Tuning
