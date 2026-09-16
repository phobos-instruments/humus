# Helix

A four-strand live looper with one main button per strand for record, play, overdub and play again.

Each strand is its own loop of up to 30 seconds. Press Rec to record, again to close the loop and play, again to overdub, and once more to play; hold Rec while a loop plays and it overdubs only for as long as you hold it. Every overdub is a layer that Undo peels off and Redo restores. Each strand has its own Sync: Free closes the loop exactly where you press, Beat and Bar snap presses, Rev and Half to the transport grid, and a press a little late is anchored back to the gridline you meant. The main outlet carries the monitored input plus every audible strand, and outlets 3 to 10 are stereo direct outs, one pair per strand, after its Level, mute and solo. Loops are saved with the patch as sound files beside it and reopen stopped, ready for Rec or Play. Cord a SoundIn into the inlet and map each Rec to a pad or a footswitch.

## Parameters

**Rec1** The strand's main button: record, close and play, overdub, play. Hold it during play to overdub only while held. The same for strands 2 to 4.

**Play1** Relaunches the strand from the top of its loop, on the grid when Sync says so. It also launches a loop that Stop closed silent.

**Stop1** Halts the strand. While recording it closes the loop silent, ready to launch.

**Undo1** Peels the newest layer off. While recording it throws the take away.

**Redo1** Puts the last undone layer back.

**Clear1** Empties the strand.

**Level1** The strand's level in the main mix and on its direct out.

**Mute1** M: silences the strand. It keeps running underneath, so it comes back in phase.

**Solo1** S: hears this strand alone.

**Sync1** Free, Beat or Bar: what presses, Rev and Half snap to.

**Rev1** Plays the strand backwards. Flipping it ends an open overdub.

**Half1** Plays the strand at half speed, an octave down. A layer overdubbed while halved plays back at double speed when you disengage.

**Shot1** Once: the strand plays a single pass and stops. Rec launches it again.

**Decay** How much of the older layers survives each overdub pass. Below 1 a loop keeps evolving instead of piling up.

**Monitor** Passes the live input through to the main outlet.

**Follow** Freezes every strand, silent, while the transport is stopped and picks up when it rolls, synced strands back on the bar. Off, loops keep running regardless. Recording is never interrupted.

**Loop1** The strand's saved sound file, written beside the patch on save. The same for Loop2 to Loop4.

## Recipe

**Locked bed, free voice** Sync1 Bar, Sync2 Free, Follow on, Monitor on, Decay 0.85. With the transport rolling and a SoundIn corded in, press Rec1 on the one, play four bars and press it again on the next one; hold Rec1 to overdub a second pass. Press Rec2 mid-phrase for a voice strand that drifts against the grid. Cord outlets 3 and 4 into a Fern for the first strand alone.

## Related Organisms

Repeater, Leafcutter, Sampler
