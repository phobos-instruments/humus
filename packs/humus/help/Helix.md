# Helix

A four-strand live looper. Each strand is its own loop with one main button that does everything: press to record, press again to close the loop and play, press again to overdub, press once more to just play. Hold the button while a loop plays and Helix overdubs for as long as you hold it, then falls back to play when you let go.

## Free or locked

Each strand has its own Sync setting, and that choice is the soul of the instrument. Free closes the loop exactly where you press, with no relation to the tempo: strands drift against each other and against the song, the way pedal loopers invite out-of-time, multi-tempo layering. Beat and Bar snap your presses to the transport grid the way tabletop loop stations do, so every strand stays locked to the song (and to Link). Press a hair late and Helix forgives it, anchoring back to the gridline you meant. Mix modes freely: a bar-locked drum bed under a free-floating voice strand is one organism.

## Layers, undo, decay

Every overdub is a layer. Undo peels the newest layer off; Redo puts it back. Undo while recording throws the take away. Decay below 1.0 makes older layers quietly sink as you overdub new ones, so a loop stays alive instead of piling up: the endless evolving loop of the tape-echo tradition.

## Following the song

On a synced strand, Rev and Half wait for the same gridline as the buttons, so a flip lands musically instead of mid-phrase. And the loops follow the playhead: stop the transport and they freeze in place (see Follow); press play from the top and every playing loop restarts from its beginning; relocate or resume mid-song and synced strands land on the phase the bar implies, while free strands keep drifting.

## Inlets

**1-2** The live signal. It passes through while Monitor is on, and is written into whichever strands are recording or overdubbing.

## Outlets

**1-2** The main mix: the monitored input plus every audible strand.

**3-10** Stereo direct outs, one pair per strand, after its Level, mute and solo. Cord a single strand into its own effect chain, four strands onto four desk channels - and leave the main out unpatched if the strands should only live on their directs.

## Performing it

**Rec** the strand's main button. Map each one to a pad or footswitch (right-click, MIDI Learn) and Helix plays like a hardware loop station.

**Play** relaunches the strand from the top of its loop, on the grid when Sync says so - retrigger a phrase on the one, or launch a loop that Stop closed silent.

**Stop** halts the strand; Rec starts it again from the top. While recording, Stop closes the loop silent, ready to launch.

**Undo** peels the newest layer; Redo restores it.

**Clear** empties the strand.

**Level** the strand's volume in the mix.

**Sync** Free, Beat or Bar, per strand.

**Rev** the strand plays backward; overdub while reversed for the classic backwards layering. Flipping it ends an open overdub.

**Half** the strand plays at half speed, an octave down, tape style. Overdub while halved and the layer chipmunks back up when you disengage.

**M / S** mute and solo per strand, mixer style. A muted strand keeps running underneath, so it comes back in phase.

**Once** one-shot: the strand plays a single pass and stops. Rec launches it again - a long stab you fire on cue.

**Decay** how much of the older layers survives each overdub pass.

**Monitor** pass the live input through to the output.

**Follow** the strands freeze, silent, whenever the transport stops and pick up again when it rolls (synced strands back on the bar). Off, loops keep running regardless of the transport, pedal-looper style. Recording is never interrupted.

Loops live up to 30 seconds per strand. Saving the document saves them too: each strand is written as a sound file in a folder beside the patch, and reopening the patch brings every loop back, stopped and ready to launch with Rec or Play.

## Related Organisms

Repeater, Leafcutter, Sampler
