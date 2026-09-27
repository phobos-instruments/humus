# Deck

A DJ deck for sound files and videos: pitch fader, beatgrid, cues, loops, and timecode vinyl control.

**Timecode vinyl** Tested and supported with Traktor timecode vinyl MK2 and with Phase Essential. Switch Timecode on, cord the turntable through a Phono organism into the deck's inlets, and the record drives the platter: scratching, backspins, and lifting the needle stops it. The deck recognises the pressing and calibrates to the tone as the needle lands, so there is nothing else to set. If nothing tracks, look at the scope beside the switch: a clean ring is timecode, a diagonal line means both channels carry the same thing, a cloud means it is not timecode at all.

**Beatmatching** Loading a track detects its BPM, grid and key. Flag two decks Sync and both ride the patch's tempo, in time with every sequencer and delay in the patch.

**Videos** Load a video and the deck plays its sound and sends its picture from the video outlet; the picture follows the platter, scratching included. PictureSync trims the picture against the sound in milliseconds if your screen is late or early.

**Playhead anywhere** Right-click any knob in the patch and the deck's position, speed and seconds are under Control with.

## Parameters

**File** The sound file or video the deck plays.

**AudioTrack** Which sound track of a video to play, when it has more than one.

**Record** Arms the deck: what reaches its inlets while the patch records lands in the project's Recordings folder and back onto this platter.

**FileSound** The sound played when it is not the one inside File, set by a recorded take.

**Active** Play or stop, with a short fade so there is no click.

**Loop** Loops between LoopIn and LoopOut.

**Sync** Follows the patch's tempo and lines the beats up with the grid.

**Keylock** Changes tempo without changing pitch. Bypassed while Timecode is on.

**PictureSync** Moves the picture earlier or later against the sound, in milliseconds, per deck.

**Timestamp** Burns the deck's playhead time onto the picture it sends out.

**Quantize** Snaps cues and loops to the nearest grid beat.

**Timecode** Lets a control record drive the platter.

**TimecodeHz** The control tone's rest frequency, measured as the needle lands and held while the record plays. Read-only in practice: drop the needle again at the proper speed to re-measure.

**TimecodeFlip** Turn on if the track plays backwards when the record goes forwards.

**BPM** The track's tempo.

**PitchPercent** The pitch fader, in percent.

**PitchRange** How far the fader travels, 8 to 50 percent.

**Volume** Output level, not on the faceplate: set the level at the Mixer, or map this.

**GridOffset** Where the grid's first downbeat sits, in samples.

**LoopIn** Loop start, in samples.

**LoopOut** Loop end, in samples.

**LoopBeats** Beat-loop length, a quarter beat to 32 beats.

**Cue** The main cue point, in samples. Cue jumps to it without starting or stopping the deck.

**Key** The key detected when the track loaded.

**HotCue_1** The first hot cue, in samples; -1 means unset. Pressing an empty one stores the playhead, pressing a stored one plays from it, Shift-click clears it. And so on for 2 to 8.

## Recipe

**Two-deck mix** Load a track on each Deck and check the grid lands on the beats. Flag both Sync, cord them into a Mixer, and drop the second in on a hot cue at its first downbeat.

## Related Organisms

AudioTrack, Sampler, Mixer, Console, Phono
