# FilePlayer

Plays a mono or stereo sound file into the patch.

Use it for backing tracks, field recordings and one-shot cues that do not need to follow the transport. It runs free: Active starts and stops it, and the position bar on the box scrubs to any point while it plays or sits paused. Loop repeats the file with an optional gap of silence between passes, and AutoRewind decides whether each start picks up where it stopped or from the top. For loops that stay locked to the beat, use LoopPlayer instead. Cord the output into a Mixer or straight to SoundOut.

## Parameters

**File** The sound file to play. Choose it with the file button or drop a file onto the slot.

**Loop** Plays the file again from the start each time it reaches the end. Off plays it once and then goes silent.

**LoopDelay** Seconds of silence inserted between the end of one pass and the start of the next. Only used while Loop is on.

**AutoRewind** Jumps back to the beginning each time Active turns on. Off resumes from wherever playback last stopped.

**Active** Starts and stops playback. Automate it to bring a file in at a set point in the patch.

## Recipe

**Cued backing track** Load the file, AutoRewind on, Loop off. Cord the output into a Gain, then to SoundOut, and map Active to a Button so one press starts the track from the top every time; a second press stops it and the next press starts it clean again.

## Related Organisms

SoundIn, LoopPlayer, Drums
