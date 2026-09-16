# Filter

A valve-driven state-variable filter in the tradition of the rackmount club filter.

A drive stage feeds a resonant filter whose HI, BAND and LO buttons combine rather than exclude each other: HI with LO is a notch to sweep for phasing, all three together is an all-pass that Resonance makes dramatic, and none selected passes the signal clean. An envelope follower and a triangle or square sweep can both push the cutoff, or the envelope can be sent into the drive instead. Every switch is soft, so the buttons can be ridden in a performance without clicks. Cord it after a Rhizome, a Drums organism or a whole Mixer, and cord an LFO or a Follower onto Frequency for more shapes than the built-in sweeps give.

## Parameters

**EnvFollow** How far the input's loudness opens the cutoff. Zero leaves the filter static.

**EnvDecay** Fast or Slow release for the envelope once the input drops.

**Drive** The valve stage. It adds warmth first, then soft clipping, then hard clipping as you push, with make-up gain so the level stays put.

**EnvToDrive** Sends the envelope into Drive instead of the cutoff, so loud passages distort more.

**LfoSpeed** Sweep rate from 0.2 Hz to 10 kHz. Past a few hundred hertz the sweep itself becomes a tone.

**LfoWave** Triangle glides between the ends of the sweep. Square jumps between them.

**LfoSync** Locks the sweep to the transport instead of LfoSpeed.

**LfoBeats** The length of one synced sweep in beats, so 4 is a bar of four.

**LfoDepth** How far the sweep pushes the cutoff, up to a few octaves each way.

**Resonance** From a gentle roll-off to a synth peak. It eases back on its own at high input levels and low cutoffs so full sweeps stay speaker-safe.

**HiPass** Adds the high-pass band to the output.

**BandPass** Adds the band-pass band to the output.

**LoPass** Adds the low-pass band to the output.

**Frequency** The cutoff, 20 Hz to 20 kHz. The big sweep.

**FilterOn** Switches the filter in or out. Off is a clean straight wire.

**Mono** Runs both channel filters in series from the left input for a 24 dB slope and doubled resonance, feeding both outputs.

**MixInvert** Outputs the dry signal minus the filtered one, for cancellation effects that sit well before a delay or reverb.

## Recipe

**Synced sweep** LoPass on, Resonance 6, Drive 3, Frequency 400. LfoSync on, LfoBeats 4, LfoWave Triangle, LfoDepth 6. Cord a Drums organism in and the cutoff opens and closes once a bar in time with the transport.

## Related Organisms

LFO, Gain, Follower
