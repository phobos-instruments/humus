# Wave

A wavetable oscillator whose cycles are grown, not chosen. The big display in the editor is the sound: draw on it with the mouse, start from a classic shape - sine, triangle, saw, square - or roll the Random tile for a fresh spread of partials, sometimes hollow, sometimes vowel-ish, sometimes gritty.

## One cycle or many

Wave holds up to sixteen frames. Seed a sound file and it is chopped into frames, one cycle per slice, so a whole sample becomes a table that evolves across its length. + frame and - frame build a table by hand from the frame you are on, and the strip under the display shows every frame with the current one lit - click a slice to jump to it.

Position scans the table. At rest it sits on one frame; swept, it morphs between the two nearest frames, and morphing never dips in level because the frames are phase-aligned first. Spectral changes how the morph is made: it blends the frames' harmonic magnitudes rather than their samples, so a partial that sits opposite between two frames fades away instead of cancelling mid-sweep. PositionMod lets the amp envelope sweep Position on its own, so a note can travel through the table as it plays.

Warp reshapes the single cycle the way the modern wavetable tradition does: Sync bends the read faster within the period, Bend skews the phase ramp, Fold reflects the peaks back into range, PWM squeezes the duty. Amount sets the depth (it is smoothed, so sweeping it never zippers), and the shaped result is drawn as a second trace over the base cycle.

Unison stacks up to seven detuned copies per voice (Voices), spread in pitch by Detune and across the stereo field by Width. One voice is dead centre; more voices thicken and widen.

However wild the cycle, playback stays clean: Wave rebuilds a band-limited mipmap per frame on every edit and always plays the brightest copy whose harmonics still fit under half the sample rate. High notes lose only what physics says they must, never gaining the fold-over hash a naive wavetable player produces.

## Playing it

Wave is a MIDI instrument: eight voices, played from a PianoRoll, DNA, Steps, a MidiIn cord, or a virtual keyboard. Notes are read through the patch's Tuning, so Wave follows your scale. The dice on the property box rolls the wave along with the shaping pots; a rolled cycle lands in the patch like any edit, so undo brings back exactly the wave you had.

## Parameters

**Attack** how long a note takes to open, in milliseconds.

**Release** how long it takes to die after the key lifts.

**Sub** a pure sine one octave below every voice - weight without changing the drawn cycle's character. 0 is off.

**Position** scans and morphs between the table's frames. Does nothing on a one-frame table - seed a file, or add frames, for more.

**Spectral** morph by harmonic magnitude, not by samples - smoother through frames whose partials sit in opposite phase.

**PositionMod** how far the amp envelope sweeps Position, per voice. 0 is off.

**WarpMode / Amount** the cycle shaper (Off, Sync, Bend, Fold, PWM) and its depth.

**Voices / Detune / Width** unison: how many detuned copies, how far apart in pitch, how wide across the stereo field.

**Cutoff / Reso** a lowpass over the voices. Cutoff parked at the top of its travel takes the filter out of the path entirely.

**Drive** saturation on the sum - warmth low, growl high. 0 is clean.

**Level** output level.

## Signal flow

Table (drawn / preset / seeded frames) -> Position morph -> Warp -> band-limited oscillator, unison + Sub -> envelope -> lowpass -> Drive -> Level -> stereo Output. No audio inputs; one MIDI inlet.

## Notes

Drawing edits the frame nearest Position - the rest of a seeded table is untouched, so you can retouch a grown wave frame by frame. Seed file composts anything into frames: a recording contributes the cycles found along it, an image the light of its rows, any other file its raw bytes. The same seed always grows the same table, baked into the patch, so a wave grown from a photo still plays on a machine that never had it. Wave pairs naturally with Substrate and Rhizome: they are architectures of many voices, Wave is one voice whose cycles are the instrument.

## Related Organisms

Rhizome, Prism, Tuning
