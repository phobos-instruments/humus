# LFO

A low-frequency oscillator that emits a wave as a control signal for other organisms' knobs.

It outputs Offset plus Amplitude times the wave at Rate, added to whatever arrives at its inlet, so an LFO can wobble around a Number's value. With Sync on, one cycle lasts SyncBeats beats of the transport instead of running at Rate. FN plays a shape of your own: cord a Math into the Function socket and the LFO plays the Math's formula as its wave, or cord a Wave in and it plays the Wave's table. The face draws one cycle of the wave it is emitting with a playhead riding it, or on FN its recent motion. Right-click any parameter and Control with offers the LFO twice: wave is the wave itself and phase is the rising ramp of the cycle. Cord its outlet onto a Gain's socket for tremolo or onto another LFO's Rate for drifting motion. It carries no audio.

## Parameters

**Rate** Cycles per second while Sync is off, from one every hundred seconds up to 1000 Hz. The top of the range is audio rate. On FN it is also rate in the formula, so a motion that steps by rate*dt runs at it.

**Waveform** SIN, TRI, SQR, SAW, DSAW (a falling saw), S&H, which steps to a new random value once per cycle, or FN, the formula or table handed over through Function.

**Amplitude** How far the wave travels either side of Offset.

**Offset** The centre the wave moves around. Raise it to keep a bipolar wobble positive.

**Sync** Locks one cycle to the transport instead of Rate. While the transport plays, the cycle starts on beat 1 and follows every jump, loop and scrub; while it is stopped the wave keeps running at the tempo.

**SyncBeats** How many beats one cycle lasts while Sync is on. One beat is a quarter note, so 0.5 is an eighth and 4 is a bar of four.

**Function** Cord a Math in here to hand its formula to the LFO. The cord carries the formula and the Math's X, Y, Z and W knobs rather than a value: the LFO switches to FN, follows every edit while the cord is there and keeps what it has when the cord is pulled. In the formula, phase is the LFO's own cycle and rate its Rate, so a Math's Nature presets move at the LFO's pace. A Wave corded in hands over its table instead: the LFO plays the frames at the Wave's Position and follows that knob.

## Recipe

**Synced tremolo** Sync on, SyncBeats 0.5, Waveform SIN, Amplitude 0.4, Offset 0.6. Cord the outlet onto the Gain socket of a Gain carrying a pad and the level pulses in eighths; switch to SQR for a hard gate.

**Bouncing ball** Load the Nature preset Gravity in a Math, cord the Math into the LFO's Function and the LFO's outlet onto a filter's cutoff. The tone follows a ball that bounces sooner and lower until it rests, then is thrown up again; Rate speeds the ball up and the Math's X makes it livelier.

**Wandering wave** Write `walk := walk*(1 - rate*dt) + sqrt(6*rate*dt)*noise(); sin(tau*ph(rate*(1 + 0.15*x*walk)))` in a Math and cord it into Function: a sine at the LFO's Rate whose speed drifts the way a hand on the knob never quite repeats.

## Related Organisms

Math, Number, Follower, Sig
