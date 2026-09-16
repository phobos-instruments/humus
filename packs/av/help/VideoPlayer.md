# VideoPlayer

A video deck that loops one tape into the video chain.

Load a tape into the File slot, or drop a clip from the timeline onto it, and cord the video outlet into a Lumen layer, a VideoFX, a VideoMix or a VideoOut; the tape plays there, looping. Rate is the motor and Opacity and Blend set how the layer composites, all ordinary params, so a Follower can drive the speed with the music. The MIDI inlet is a relaunch trigger: any note-on rewinds and restarts the tape, so one deck per launch lane, fed by a Sequence or a PianoRoll, cuts video in time. The deck rolls while its editor is open or something downstream watches its outlet, and rests otherwise.

## Parameters

**File** The video to play.

**Rate** The Speed knob, in percent. 100% is as recorded, 0% holds a still frame, 200% doubles it.

**Opacity** How strongly this layer shows over the ones beneath it in a Lumen.

**Blend** How it combines with those layers: Normal, Add, Multiply or Screen.

## Recipe

**Note-cut loop** Load a short tape, cord the outlet into a VideoOut and a Sequence into the MIDI inlet with one note every bar, so the tape restarts on the downbeat. Put a slow, shallow LFO on Rate around 100% and the loop breathes against the grid.

## Related Organisms

Lumen, VideoMix, VideoOut
