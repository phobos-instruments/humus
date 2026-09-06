# Gate

An audio gate that answers to whatever you point at it. The signal at the first inlet passes when the gate is open and falls to the Floor when it is shut; what opens it is the Source switch: the level of a signal, held MIDI notes, or a plain on/off you flip by hand or by control route. The smoothed open state is also published as the control value "open", so the same gate that shapes audio can fire or ride anything else in the patch.

LEVEL is the classic noise-gate move: the detector watches the second inlet (or the signal itself when nothing is patched there), opens above Threshold and closes below it with a little hysteresis. Patch a kick into the key inlet and a pad into the signal inlet for sidechain chopping. MIDI opens the gate for as long as any note is held at the MIDI inlet - play the gate like an instrument. SWITCH hands the gate to the Open toggle, which a Slider, LFO or Follower can flip through a control route with a switch shape.

Duck inverts the whole sense: open becomes shut and shut becomes open, which turns any of the three sources into a ducker.

## Parameters

**Source** what opens the gate: LEVEL, MIDI, or SWITCH.

**Detector** how LEVEL listens - PEAK reacts to instantaneous swings, RMS to average loudness.

**Threshold** the level that opens the gate in LEVEL mode.

**AttackTime / HoldTime / ReleaseTime** how fast it opens, how long it stays open after the source drops, and how slowly it closes.

**Floor** how much signal still leaks through when shut. Zero is a hard gate; raise it for an expander feel.

**Duck** swaps open and shut.

**Open** the manual switch, used when Source is SWITCH.

## Related Organisms

NoiseGate, SideChain, VCA, Follower, Slider
