# Decomposer

Audio in, MIDI notes out. Like a soil decomposer breaking matter down into reusable nutrients, it breaks a sound into the notes hiding inside it: play a line in and it emits MIDI as you play. A modular, first-principles take on the MIDI-guitar idea, built on YIN pitch tracking - no trained model, nothing phones home.

## How it listens

It follows one voice at a time. The pitch search is confined to the Low to High note range, its strongest defence against octave errors, then median-smoothed to shrug off single-frame glitches, and struck notes re-articulate from an energy onset, so playing the same note twice registers as two notes rather than one held one. It is best on a single instrument or voice: a bassline, a vocal melody, a synth lead, a guitar played one note at a time. Full polyphonic transcription needs a trained model, and a guess at it would be worse than nothing.

It reports what it hears and nothing else: no key, no scale, no correction. To pull the result into tune, put the notes through something that does that on purpose.

## Parameters

**Sensitivity** lowers the level and clarity thresholds. Raise it for quiet or breathy sources, lower it if noise triggers stray notes.

**Response** Fast, Balanced or Accurate: how many frames a pitch must hold before it commits. Fast is snappier, Accurate steadier on noisy or breathy sources.

**Low / High** the note range to listen for, C0 to C8 by default. Narrow it to your source's actual range and octave errors and rumble outside the band simply cannot be chosen.

**Channel** the MIDI channel the notes go out on.

## Notes

The tuner strip shows what it hears live: the note being emitted, a 50-cent needle for how sharp or flat you are, and level and lock bars.

Patch the MIDI outlet into any instrument - a synth organism, a hosted plugin, a Kick - or into a MidiMonitor to see what it hears. Try it on a mic for voice into notes, then into Trellis for a sung melody that also plays a synth. There is a short tracking latency of a few tens of milliseconds, inherent to hearing a pitch before naming it.

## Related Organisms

Trellis, OscMonitor, MidiMonitor
