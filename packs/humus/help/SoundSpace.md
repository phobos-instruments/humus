# SoundSpace

A granulator played by flying a cursor across a map of grains sorted by sound, which also records what comes into it.

Load up to four sound files; SoundSpace slices them into grains, analyses each one's energy, brightness, noisiness and movement, and scatters them on a two-dimensional map where similar grains land near each other, one colour per file. The cursor plays a cloud of the grains around it, so dragging across the map morphs between textures. Mix very different material, voice, machines and field recordings, and the map spreads further apart, which makes travelling it more dramatic. Right-click the map to automate or MIDI-learn X and Y, so an XY pad or a slow LFO can fly it. Live is on to begin with, so the inlet is already a source of its own and cording something in is all it takes: the last eight seconds of it are sliced and dropped on the same map as they arrive, pale dots among the files' colours, the newest ones larger, so you can see where your playing lands and fly the cursor between it and the recordings. The meter beside Freeze shows the inlet's level, so an empty map with a dark meter means nothing is arriving. The two live side by side: play in while files are loaded and the map holds both, and Rec captures the inlet into the next empty file slot so a passage you liked becomes one of them. Live needs no files at all; with none loaded the map runs from dark to bright across and from pitched to noisy upwards.

## Parameters

**Mute** Silences the output while the cloud keeps flying, so unmuting rejoins mid-texture.

**Record** The red Rec pad. Captures the audio inlet to a sound file under Documents and drops it into the first empty slot when it stops. Silent takes are discarded, and a take is capped at two minutes.

**Live** Granulates the audio inlet as it arrives, and is on to begin with. The last eight seconds stay on the map; older sound drops off, and silence is left out. Switch it off to play the loaded files alone.

**Freeze** Holds the live buffer still, so the cloud keeps playing what it caught while the inlet is ignored. Only with Live on.

**X** The cursor's horizontal position on the map, 0 to 1.

**Y** The cursor's vertical position on the map, 0 to 1.

**Density** Grains per second.

**Scatter** How unevenly the grains arrive. At zero they come at exactly the Density rate, like a clock; turned up, each gap is drawn anywhere from almost nothing to nearly twice as long, while the average rate stays where Density puts it, so the cloud stumbles and clusters instead of ticking.

**GrainSize** Grain length in milliseconds. Open the span and every grain takes its own length from inside it.

**Fit** Scales the pad to where the sounds actually are. A bank whose points land in one corner fills the pad instead of hiding in it, and the numbers along the edges read the window rather than the whole space. It never zooms closer than a quarter of the space, so a single tight cluster does not become a microscope, and it only re-fits when the points change - never under your hand mid-drag. Off gives the whole space, as it was.

**Spray** Picking radius around the cursor. Small stays on one texture; large samples the whole neighbourhood.

**Spread** How far each grain is panned away from the centre at random.

**Pitch** Transpose in semitones. Closed it is one interval; open it and each grain is drawn from somewhere inside the span, so an upward-only span makes a rising shimmer rather than a symmetrical blur.

**Skew** Where the peak sits inside a grain, a span like the others. Closed at zero it is a smooth bell; towards one the rise shortens until the grain is struck and rings out, which turns a wash into a rhythm even at the same density.

**Shape** How much of a grain is held open at full level between its rise and its fall. At zero the grain is all rise and fall; towards one it becomes a flat-topped block, which makes a smoother, more continuous texture out of the same material.

**Input** Level of the inlet as it enters the live buffer, up to one and a half times. Bring it down to let Feedback take over, up to keep new playing on top. Only with Live on.

**Feedback** How much of the cloud is written back into the live buffer. At full the cloud feeds itself for ever and never dies away; the write-back is soft-limited, so it thickens and saturates instead of running away. With Pitch away from zero each pass lands an interval higher or lower, so a held note stacks into a chord. Only with Live on, and not while frozen.

**Mix** Balance between the dry inlet and the cloud. Fully wet by default.

**Quant** How hard a grain is pulled onto the grid chosen by Sync. At zero it is not pulled at all, at full it waits for the line. It does nothing while Sync is Free.

**Decorrelation** How much a grain's random choices agree with one another. At zero one roll decides its pitch, length, hold and panning together, so higher grains land to one side; at full each is drawn on its own.

**Voices** The most grains that may sound at once, 1 to 200. Fewer thins the texture and costs less; more thickens it.

**Sync** A rotary switch that holds each grain to the transport's grid: Free, a thirty-second to a quarter note, or one bar. Density still decides how often a grain is due; Sync makes it wait for the next line. Only while the transport plays.

**Gain** Output level.

**File1** The first corpus file, and so on for File2 to File4. Analysis runs when a file is picked.

## Recipe

**Evolving bed** Load a voice recording in File1 and a field recording in File2. Density 12, GrainSize 200, Spray 0.08, Spread 0.8, Pitch opened a little either side. Cord an LFO at 0.02 Hz onto X and another at 0.03 Hz onto Y so the cursor wanders the map, and send the output through a Verbatim.

**Live cloud** No files, Live on, a voice or an instrument corded into the inlet. Density 20, GrainSize 150, Spray 0.3, Pitch opened a touch. Play a phrase, press Freeze, then fly the cursor through what you just played. For chords, unfreeze, set Pitch to 7 and bring Feedback up to 0.6.

## Related Organisms

CameraIn, Sampler
