# Riff

The acid-box step sequencer as a MIDI source: sixteen steps of gate, accent, slide and note - the classic bassline pattern language - played against the transport with swing, out of a MIDI cord.

Pair it with Acid and the semantics line up by construction: accented steps leave at velocity 118 (over Acid's accent threshold, so the filter bites), and a slide step's note-off lands just after the next note-on - the overlap Acid reads as legato glide. It drives any other MIDI instrument just as well.

## The grid

Click a step to gate it; the upper part of a step toggles accent, the slide row ties it into the next step; drag on the note row for pitch.

## Parameters

**Swing** Delays every second 16th, up to a triplet feel. The groove.

**Gate** Non-slide note length, as a fraction of the step.

**Velocity** Plain (non-accent) note velocity.

**Transpose** Shifts the whole pattern in semitones - drop the riff a fifth for the breakdown without redrawing it.

Live: mute by ear with Gate at minimum, or map Transpose/Swing to knobs.

## Related Organisms

Acid, Steps, Microdot
