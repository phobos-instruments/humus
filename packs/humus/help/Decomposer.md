# Decomposer

Audio in, MIDI notes out: a monophonic pitch tracker that turns a played line into notes as you play.

It follows one voice at a time, so it is best on a bassline, a sung melody, a lead or a guitar played one note at a time. The pitch search is confined to the LowNote to HighNote range, which is its strongest defence against octave errors, single-frame glitches are smoothed away, and a repeated note re-articulates from its onset instead of merging into one held note. It reports what it hears with no key or scale correction; to pull the result into tune, send the notes through a Trellis. The readout on the editor shows the note being sent and how sharp or flat the source is. Cord a SoundIn into it and its MIDI outlet into any instrument, or into a MidiMonitor to watch it. Expect a few tens of milliseconds of tracking latency.

## Parameters

**Sensitivity** Lowers the level and clarity thresholds. Raise it for quiet or breathy sources, lower it if noise triggers stray notes.

**Response** How many frames a pitch must hold before it commits. Fast is snappier, Accurate is steadier on noisy sources, Balanced sits between.

**LowNote** The lowest note it will report. Raise it to your source's real range so rumble below cannot be chosen.

**HighNote** The highest note it will report. Lower it to your source's real range so octave errors above cannot be chosen.

**Channel** The MIDI channel the notes go out on.

## Recipe

**Voice to synth** Cord a SoundIn from a microphone into the Decomposer and its outlet into a Rhizome. Response Balanced, Sensitivity 0.5, LowNote 48, HighNote 84 for a typical singing range. Hum a line and the synth doubles it; add a Trellis between the two to keep it in key.

## Related Organisms

Trellis, OscMonitor, MidiMonitor
