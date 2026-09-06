# Cluster

Chords on demand. Eight slots each hold a handful of notes - typed as names ("C3 E3 G3", sharps and flats welcome, bare numbers work too) - and each slot fires as a chord out of the MIDI outlet when you press its button, play its trigger note, or route a control at its Fire parameter. Point the outlet at any instrument and one pad plays whole voicings.

In PADS mode the eight slots map to eight incoming notes starting at From: play the mapped key and the slot's chord sounds at your velocity, released when you let go. In FOLLOW mode one chosen slot becomes a shape that follows your playing - every incoming note becomes the root and the chord moves with it, so a single finger walks the voicing up and down the keyboard.

Hold decides what a press means. GATE is momentary: the chord sounds while the trigger is down. LATCH makes every trigger a toggle - press to start, press again to stop, and pads can layer. PEDAL is one chord at a time: each press replaces whatever was ringing and note-offs are ignored, so a single tap carries the harmony until the next tap - and a pad left empty becomes a stop button. Notes shared between two sounding slots are counted, not cut - releasing one pad never chokes the note a second pad still holds.

## Parameters

**Mode** PADS plays the eight slots; FOLLOW transposes one slot from the keyboard.

**From** the incoming note mapped to slot 1; the next seven notes up map to the next slots.

**Slot** which slot FOLLOW mode carries around.

**Vel** the velocity used when a slot is fired from its button or a control route.

**Hold** GATE while pressed, LATCH toggles, PEDAL swaps - one chord ringing until the next.

**Notes 1-8** the slots themselves.

**Fire 1-8** the triggers behind the buttons, reachable by control routes and automation.

## Related Organisms

PianoRoll, Steps, Riff, Harmonizer
