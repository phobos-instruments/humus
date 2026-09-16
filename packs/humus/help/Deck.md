# Deck

A DJ deck that plays a sound file at a variable rate from a pitch fader, carries a beatgrid, and can drive or follow the patch's tempo.

Two decks, one flagged Master and one flagged Sync, is the classic setup: the Master drives the transport's tempo and the follower rides it, so every sequencer, LFO and delay in the patch is already in time with the record. Loading a track detects its BPM, grid offset and key; get the grid right first, because Quantize, Sync and the beat loops are all built on it. The stereo inlets are ignored unless Timecode is on, when they carry a control record's tone through a Phono organism and the record drives the platter by hand: beatmatching, scratching, backspins, and lifting the needle stops the motor. Cord the outlets into a Mixer or a Console.

## Cues and loops

The hot cue buttons store the playhead the first time they are pressed and jump there afterwards; shift-click clears one. Set Cue stores the main cue point and Cue jumps to it. The 1, 4 and 8 buttons set a beat loop from the playhead, Loop switches it off and on, and /2 and x2 halve and double it while it runs. With Quantize on, every cue and loop point snaps to the nearest grid beat.

## Parameters

**File** The sound file the deck plays.

**Active** Play or stop.

**Loop** Loops between LoopIn and LoopOut.

**Sync** Follows the transport's tempo and lines its beats up with the grid, so it locks to a Master deck.

**Master** This deck drives the whole patch's tempo. Only one should be master at a time.

**Keylock** Changes the tempo while holding the pitch, for beatmatching without the key drifting. It falls back to plain resampling where time-stretching is not available, and is bypassed while Timecode is on.

**HQ** Higher-quality resampling. On by default; turn it off to save CPU on a crowded patch.

**Quantize** Snaps cues and loops to the nearest grid beat.

**Timecode** The stereo inlets drive the platter from a control record.

**TimecodeHz** The record's tone frequency at normal speed. Most press around 1000 Hz; check the sleeve.

**BPM** The track's tempo, the beatgrid's spacing.

**PitchPercent** The pitch fader, as a percentage. Speeding up goes sharp unless Keylock is on.

**PitchRange** How far the fader travels, from 8 percent, the classic turntable range, up to 50 for extreme effects.

**Volume** Deck output level.

**GridOffset** Where the grid's downbeat sits, in samples. BPM sets the spacing of the beats; this sets where they start.

**LoopIn** Loop start, in samples.

**LoopOut** Loop end, in samples.

**LoopBeats** Beat-loop length, from a quarter beat to 32 beats.

**Cue** The main cue point, in samples.

**Key** The key detected when the track loaded, shown on the deck.

**HotCue_1** The first hot cue, in samples; -1 means unset. And so on for 2 to 8.

## Recipe

**Two-deck mix** Load a track on each Deck and check the grid lands on the beats. Flag the first Master and the second Sync, cord both into a Mixer, and set the second's LoopBeats to 4. Start the second on a hot cue at its first downbeat and it stays locked while you fade across.

## Related Organisms

AudioTrack, Sampler, Mixer, Console
