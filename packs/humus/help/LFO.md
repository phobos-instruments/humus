# LFO

A low-frequency oscillator that emits a wave as a control signal for other organisms' knobs.

It outputs Offset plus Amplitude times the wave at Rate, added to whatever arrives at its inlet, so an LFO can wobble around a Number's value. With Sync on, one cycle lasts SyncBeats beats of the transport instead of running at Rate. The face draws one cycle of the wave it is emitting with a playhead riding it. Right-click any parameter and Control with offers the LFO twice: wave is the wave itself and phase is the rising ramp of the cycle. Cord its outlet onto a Gain's socket for tremolo or onto another LFO's Rate for drifting motion. It carries no audio.

## Parameters

**Rate** Cycles per second while Sync is off, from one every hundred seconds up to 1000 Hz. The top of the range is audio rate.

**Waveform** SIN, TRI, SQR, SAW, DSAW (a falling saw) or S&H, which steps to a new random value once per cycle.

**Amplitude** How far the wave travels either side of Offset.

**Offset** The centre the wave moves around. Raise it to keep a bipolar wobble positive.

**Sync** Locks one cycle to the transport instead of Rate.

**SyncBeats** How many beats one cycle lasts while Sync is on. One beat is a quarter note, so 0.5 is an eighth and 4 is a bar of four.

## Recipe

**Synced tremolo** Sync on, SyncBeats 0.5, Waveform SIN, Amplitude 0.4, Offset 0.6. Cord the outlet onto the Gain socket of a Gain carrying a pad and the level pulses in eighths; switch to SQR for a hard gate.

## Related Organisms

Number, Follower, Sig
