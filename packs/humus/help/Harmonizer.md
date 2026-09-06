# Harmonizer

Live harmonies for a voice, a horn, or any single melodic line. The Harmonizer listens to the pitch at its inlet and grows up to four extra voices around it, each one the same performance shifted to a new note - vibrato, phrasing and all. Harmonies only sound while the tracker hears a clear pitch, so breaths and consonants pass clean.

In MIDI mode you play the harmony: hold a chord at the MIDI inlet and the voices pitch the incoming line to the held notes - sing one note, sound the whole chord, move your hand and the stack follows. In SCALE mode it runs hands-free: set Key and Scale, give each voice an interval in scale degrees (+2 is a diatonic third up, -3 a fourth below, +7 the octave) and the harmonies stay in key wherever the melody goes.

Glide decides whether the voices leap to new targets or slide; Humanize lets each voice drift a few cents at its own slow rate so the stack stops sounding like one machine; Spread fans the voices across the stereo field around the dry line.

## Parameters

**Mode** MIDI plays the targets from held notes; SCALE derives them from Key, Scale and the Voice intervals.

**Key / Scale** the tonal home for SCALE mode.

**Voice 1-4** each voice's interval in scale degrees, with its Level knob beside it. A voice at zero level rests. In MIDI mode the held notes take over and the levels rest at full.

**Glide** how long a voice takes to reach a new target note.

**Humanize** slow per-voice detune drift, up to about 14 cents.

**Spread** stereo width of the harmony stack.

**Mix** one crossfade from the incoming line alone to the harmony bed alone, equal-power in between. Halfway keeps the lead in front with the stack behind it; full wet leaves only the harmonies - which also means silence wherever no harmony sounds (no held notes in MIDI mode, or an untracked source).

## Related Organisms

Cluster, Prism, Trellis, Decomposer
