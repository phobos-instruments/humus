# Harmonizer

A pitch-tracking harmonizer that adds up to four shifted copies of a single melodic line.

It listens to the pitch at its inlet and builds harmonies around it, each one the same performance shifted to a new note. Harmonies sound only while a clear pitch is heard, so breaths and consonants pass clean. In MIDI mode the notes held at the MIDI inlet set the targets: sing one note and the chord follows your hand. In SCALE mode Key, Scale and the four Voice intervals set them, so the harmonies stay in key wherever the melody goes. Cord a SoundIn or a FilePlayer into it, and a PianoRoll or a keyboard into the MIDI inlet for MIDI mode.

## Parameters

**Mode** MIDI takes the targets from the notes held at the MIDI inlet, up to four, at full level. SCALE derives them from Key, Scale and the Voice intervals.

**Key** The tonal home for SCALE mode, C to B.

**Scale** The scale the intervals count in, from Chromatic and Major to Blues, Whole Tone and Hirajoshi.

**Voice1** The first voice's interval in scale degrees, -7 to 7: 2 is a third up, -3 a fourth below, 7 the octave. The same for Voice2 to Voice4.

**Level1** The first voice's level in SCALE mode; a voice at zero rests. The same for Level2 to Level4.

**Glide** How long a voice takes to reach a new target, in milliseconds. 0 leaps.

**Humanize** Slow per-voice detune drift, up to about 14 cents, so the stack stops sounding like one machine.

**Spread** Fans the voices across the stereo field around the dry line.

**Mix** Crossfade from the dry line alone to the harmonies alone. Full wet is silent wherever no harmony sounds.

## Recipe

**Diatonic thirds** Mode SCALE, Key on the song's key, Scale Major, Voice1 2 with Level1 0.8, Voice3 -3 with Level3 0.5, Glide 40, Humanize 0.2, Spread 0.6, Mix 0.5. Cord a vocal from a SoundIn in and a Compressor after it to hold the stack together.

## Related Organisms

Cluster, Prism, Trellis, Decomposer
