# SoundSpace

A granulator played by flying a cursor across a map of grains sorted by sound.

Load up to four sound files; SoundSpace slices them into grains, analyses each one's energy, brightness, noisiness and movement, and scatters them on a two-dimensional map where similar grains land near each other, one colour per file. The cursor plays a cloud of the grains around it, so dragging across the map morphs between textures. Mix very different material, voice, machines and field recordings, and the map spreads further apart, which makes travelling it more dramatic. Right-click the map to automate or MIDI-learn X and Y, so an XY pad or a slow LFO can fly it. Cord a signal into the inlet and Rec captures it into the next empty file slot.

## Parameters

**Mute** Silences the output while the cloud keeps flying, so unmuting rejoins mid-texture.

**Record** Captures the audio inlet to a sound file under Documents and drops it into the first empty slot when it stops. Silent takes are discarded, and a take is capped at two minutes.

**X** The cursor's horizontal position on the map, 0 to 1.

**Y** The cursor's vertical position on the map, 0 to 1.

**Density** Grains per second.

**GrainSize** Grain length in milliseconds.

**Spray** Picking radius around the cursor. Small stays on one texture; large samples the whole neighbourhood.

**Spread** How far each grain is panned away from the centre at random.

**Jitter** Random pitch offset per grain, up to about a fifth each way at full.

**SizeRand** Scatter. Random variation of grain length, up to sixty percent each way at full.

**Pitch** Transpose in semitones.

**Gain** Output level.

**File1** The first corpus file, and so on for File2 to File4. Analysis runs when a file is picked.

## Recipe

**Evolving bed** Load a voice recording in File1 and a field recording in File2. Density 12, GrainSize 200, Spray 0.08, Spread 0.8, Jitter 0.1. Cord an LFO at 0.02 Hz onto X and another at 0.03 Hz onto Y so the cursor wanders the map, and send the output through a Verbatim.

## Related Organisms

CameraIn, Sampler
