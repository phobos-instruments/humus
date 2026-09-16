# Rhizome

A six-voice MIDI synth that stacks extra voices above each note, spaced in scale degrees of the patch's tuning.

Every played note grows a set of runners: further oscillators standing Interval scale degrees above the root, with their pitches read from the patch's tuning rather than from the harmonic series. Change the tuning on the transport and the timbre changes with it, not only the pitch. Runners alternate left and right for width, and a lowpass over the sum keeps a tall stack of saws in check. Drive it from a PianoRoll, DNA or the keyboard when the editor is focused, and send it through Fern or SoundSpace.

## Parameters

**Runners** How many voices grow from each note, from the root alone up to eight.

**Interval** The gap between consecutive runners, in scale degrees. Shown as Degrees on the panel.

**Creep** Slow independent pitch drift per runner, in cents. A little keeps the stack from phase-locking.

**Tilt** The level slope along the chain. Low keeps the sound close to the root; high lets the far runners come through.

**Wave** The runner waveform: SIN, SAW or SQR.

**Bloom** Attack time in milliseconds, how long a note takes to open.

**Decay** Release time in milliseconds, how long a note takes to die away after note-off.

**Cutoff** A lowpass over the summed output, in Hz. Lower it when many saw runners get harsh.

**Level** Output level.

**BendRange** How far the pitch wheel reaches at full travel, in semitones. Two is the common default; zero ignores the wheel.

## Recipe

**Cluster** Interval 1, Runners 6, Creep 0, Wave SIN. Play single notes from a PianoRoll: each becomes a cluster of adjacent scale degrees that beats at the scale's own step size. Retune the patch on the transport and the beating changes with it.

## Related Organisms

Wave, Mineral, Substrate
