# Helix

A four-strand live looper with Rec, Dub, Play and Stop on every strand.

Each strand is its own loop of up to 30 seconds. Press Rec to record and Rec again to close the loop and play, or Dub to close it and keep layering straight away. Dub starts and settles an overdub on a playing loop, and holding it overdubs only for as long as you hold. Play and Stop close a recording too, playing or silent. Every overdub is a layer that Undo peels off and Redo restores. Each strand has its own Sync: Free closes the loop exactly where you press, Beat and Bar snap presses, Rev and Half to the transport grid, and a press a little late is anchored back to the gridline you meant. Follow snaps them to the seam of a loop already running on another strand instead of the transport, so a first loop played free by hand becomes the grid the others fall in with; with nothing else running it waits for the bar. Inlets 1 and 2 are the main input and feed every strand; inlets 3 to 10 are one stereo pair per strand, and a strand records its own pair on top of the main input, so a corded pair gives it a voice the others do not hear. The main outlet carries the monitored inputs plus every audible strand, and outlets 3 to 10 are stereo direct outs, one pair per strand, after its Level, mute and solo. Play all and Stop all move every strand at once. Loops are saved with the patch as sound files in its project folder and reopen stopped, ready for Rec or Play. To put loops on the timeline, right-click the organism and choose Audio to Track: one strand, or All for every recorded strand, each on its own audio track at the bar under the playhead. You can also drag a strand out by the middle of its ring and drop it on the timeline where you want it. Cord a SoundIn into the inlet, and map a strand's Rec1 to a footswitch to record, play and overdub from a single pedal.

The ring on each strand draws what is recorded there: the loop's own waveform, bright behind the playhead and dim in front of it, with the layer count in the middle. A halo outside the ring follows the level arriving at that strand, so you can see where the sound is going before you arm anything. The two arrows under the ring nudge a playing loop: hold the left one and the strand runs three percent slow, hold the right one and it runs three percent fast, and when you let go it keeps the place it reached, so a tap is a hair and a long hold is a big move. It is the fix for a loop that sits a little ahead of or behind the others. The pitch bends slightly while you hold, the way a hand on a record does.

## Parameters

**Rec1** The one-switch flow for a footswitch or pad, not on the faceplate: record, close and play, overdub, play. Hold it during play to overdub only while held. The same for strands 2 to 4.

**Record1** Rec: starts recording an empty strand, and while recording closes the loop and plays.

**Dub1** Dub: starts an overdub on a playing or stopped loop and settles it on the next press; hold it to overdub only while held. While recording it closes the loop and goes straight into the overdub.

**Play1** Relaunches the strand from the top of its loop, on the grid when Sync says so. It also launches a loop that Stop closed silent, and while recording it closes the loop and plays.

**Stop1** Halts the strand. While recording it closes the loop silent, ready to launch.

**Undo1** Peels the newest layer off. While recording it throws the take away.

**Redo1** Puts the last undone layer back.

**Clear1** Empties the strand. On the faceplate hold it for a moment, so a stray click cannot lose a loop.

**Level1** The strand's level in the main mix and on its direct out.

**Mute1** M: silences the strand. It keeps running underneath, so it comes back in phase.

**Solo1** S: hears this strand alone.

**Sync1** Free, Beat, Bar or Follow: what presses, Rev and Half snap to. Follow uses the seam of the lowest-numbered strand that is playing, so its loop becomes the grid; clear that strand and the next playing one takes over. A new Helix starts strand 1 on Bar and strands 2 to 4 on Follow, so the first loop sets the length the others lock to.

**Rev1** Plays the strand backwards. Flipping it ends an open overdub.

**Slice1** Which quarter of the strand's loop is being held, 0 for none. Click a quarter of the ring to hold it and let go to release, the way the four pads on a slicing pad work; the parameter maps to a footswitch or a pad like any other.

**Nudge1** -1 while the left arrow is held, 1 while the right one is, 0 otherwise. A held nudge runs a playing strand three percent slow or fast and the strand keeps the place it reached; it does nothing to a strand that is recording, dubbing or stopped. It maps like any other parameter, so a pair of footswitches, or one centre-sprung wheel, nudges a loop without the mouse.


**Half1** Plays the strand at half speed, an octave down. A layer overdubbed while halved plays back at double speed when you disengage.

**Shot1** Once: the strand plays a single pass and stops. Rec launches it again.

**Decay** How much of the older layers survives each overdub pass. Below 1 a loop keeps evolving instead of piling up.

**Monitor1** Off, Mix or Out: where the strand's live input, the main input plus its own pair, is heard. Mix plays it through the main outlet, and the main input is heard once however many strands pick Mix; Out plays it on the strand's own direct out, for a strand that runs to its own channel. The same for Monitor2 to Monitor4.

**Monitor** The one monitor setting of older patches: a strand without a Monitor of its own follows it.

**Follow** Freezes every strand, silent, while the transport is stopped and picks up when it rolls, synced strands back on the bar. Off, loops keep running regardless. Recording is never interrupted.

**PlayAll** Play all: restarts every recorded strand from the top of its loop, together, on the coarsest grid any of them uses: the bar when one of them is on Bar, the beat when one is on Beat. Strands already playing restart too, so drifting loops fall back in phase. A strand set to Once that has finished waits for its own Rec or Play.

**StopAll** Stop all: halts every running strand, and a strand still recording closes its loop silent. It waits for the bar only when every running strand is on Bar, for the beat when none is Free; a single Free strand makes it stop everything on the press.

**Loop1** The strand's saved sound file, written beside the patch on save with the tempo it was recorded at. The same for Loop2 to Loop4.

**Tempo1** Pitch, Stretch or Off: what the strand does when the tempo moves away from the one it was recorded at. Pitch speeds the loop up or slows it down like tape, pitch and all. Stretch changes only its length and keeps its pitch, and an overdub keeps the pitch you played it at; past eight times the recorded tempo it plays as Pitch. Off ignores the tempo. The same for Tempo2 to Tempo4.

## Recipe

**Locked bed, free voice** Sync1 Bar, Sync2 Free, Follow transport on, every Monitor on Mix, Decay 0.85. With the transport rolling and a SoundIn corded in, press Rec on the first strand on the one, play four bars and press Dub on the next one to keep layering, then Dub again to settle. Press Rec on the second strand mid-phrase for a voice strand that drifts against the grid. Cord outlets 3 and 4 into a Fern for the first strand alone.

## Related Organisms

Repeater, Leafcutter, Sampler
