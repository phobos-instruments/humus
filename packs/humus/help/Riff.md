# Riff

The acid-box step sequencer as a MIDI source: sixteen steps of gate, accent, slide and note - the classic bassline pattern language - played against the transport with swing, out of a MIDI cord.

Pair it with Acid and the semantics line up by construction: accented steps leave at velocity 118 (over Acid's accent threshold, so the filter bites), and a slide step's note-off lands just after the next note-on - the overlap Acid reads as legato glide. It drives any other MIDI instrument just as well.

## The grid

Drag in the note lane to set a step's pitch - that also gates it; right-click a step in the lane to rest it. The two rows below are accent (A) and slide (S): click to toggle. While you drag a note the grid names it, and when Transpose is not zero it also names the note that will actually sound.

Right-click the A or S rows for the pattern menu: Random (a fresh line around the pattern's own root), Clear, and Copy to another bank.

## Banks

Eight banks, A to H, each a full sixteen-step pattern. The bank buttons switch which one plays and which one the grid shows; every edit lands in the bank you are looking at, so a bank is saved the moment you draw it. Switching is a parameter like any other: right-click the buttons to map them to a MIDI control or to drive Bank from a Button, a Slider, a Steps or any other control organism, and it can be automated on the timeline. Empty banks play nothing until you draw in them or copy into them.

## Parameters

**Bank** Which of the eight patterns plays, A to H.

**Swing** Delays every second 16th, up to a triplet feel. The groove.

**Gate** Non-slide note length, as a fraction of the step.

**Velocity** Plain (non-accent) note velocity.

**Transpose** Shifts the whole pattern in semitones - drop the riff a fifth for the breakdown without redrawing it.

Live: mute by ear with Gate at minimum, or map Transpose/Swing to knobs.

## Related Organisms

Acid, Steps, Microdot
