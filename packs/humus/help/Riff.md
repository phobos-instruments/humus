# Riff

A sixteen-step acid-box bassline sequencer that sends MIDI notes with gate, accent and slide per step.

Each step carries a note, a gate, an accent and a slide, the pattern language of the acid box. An accented step leaves at velocity 118, which is above the accent threshold in Acid, and a slide step holds its note just past the next note-on, which Acid reads as a legato glide. Eight banks each hold their own pattern; the bank buttons choose which one plays and which one the grid shows. It drives any other MIDI instrument as well, so cord its MIDI outlet to a Sampler or a Rhizome too.

## The grid

Drag in the note lane to set a step's pitch, which also gates it; right-click a step to rest it. The A and S rows toggle accent and slide. A slide into the same note ties it instead of striking it again. Right-click the A or S rows for Random, Clear, Nudge, Import pattern file and Copy to another bank. Load riffs, or a drop on the grid, reads MIDI files and acid-box pattern dumps into the bank shown and the ones after it.

## Parameters

**Transpose** Shifts the whole pattern in semitones.

**SwingFollow** Follow makes the pattern take the patch groove from the transport instead of its own Swing knob.

**Swing** Delays every second sixteenth, up to a triplet feel. Used only while Follow is off.

**Gate** Length of a non-slide note, as a fraction of the step.

**Velocity** Velocity of plain, non-accented notes.

**Bank** Which of the eight patterns plays, A to H.

**Nudge** Plays the riff that many steps later, or earlier below zero, wrapping around. The pattern itself stays as written, so it moves where a line lands against the kick without redrawing it.

## Recipe

**Acid line** Cord the MIDI outlet to an Acid. Draw a root note on steps 1, 4, 7, 11 and 14, an octave up on 8 and 16, accent steps 1 and 11, slide 7 into 8. Gate 0.5, Swing 0.2 with Follow off. Bring Transpose down by 5 for the breakdown.

## Related Organisms

Acid, Steps, Microdot
