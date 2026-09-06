# MidiTrack

A MIDI track the timeline way: a lane with no sound of its own and no box in the patcher. The row's chip shows where it plays - click it and pick any instrument in the patch; change the pick and the same part moves to a different contraption, notes untouched. New tracks start at "(nothing)": silent on purpose, waiting for the chip.

The R button on the row arms it: hardware keyboards and the on-screen keys flow through the track into whatever the chip points at, and what you play can land in a clip when recording. Mute and solo work like any track, and the right-click menu renames or deletes the lane.

This replaces the old two-headed arrangement where a MIDI track had to *be* its instrument. The part now lives on the lane; the player is a choice. One part auditioned through five instruments, or a keeper part safe while the patch is rebuilt around it.

## Parameters

**Target** the instrument this track plays into, always visible on the row's chip.

## Related Organisms

MidiBus, PianoRoll, Steps
