# Gate

An audio gate opened by a signal level, by held MIDI notes or by a switch.

The signal at the first inlet passes when the gate is open and falls to Floor when it is shut. Source decides what opens it: LEVEL watches the key inlet, or the signal itself when nothing is corded there, and opens above Threshold; MIDI opens for as long as any note is held at the MIDI inlet; SWITCH hands the gate to the Open toggle. The smoothed open state is published as the control value "open", so the same gate can fire or ride anything else in the patch. Cord a Kick into the key inlet and a pad into the signal inlet for sidechain chopping, or a PianoRoll into the MIDI inlet to play the gate as an instrument.

## Parameters

**Source** What opens the gate: LEVEL, MIDI or SWITCH.

**Detector** How LEVEL listens. PEAK reacts to instantaneous swings, RMS to average loudness.

**Threshold** The level that opens the gate in LEVEL mode.

**AttackTime** How fast the gate opens, in milliseconds.

**HoldTime** How long the gate stays open after the source drops, in milliseconds.

**ReleaseTime** How slowly the gate closes, in milliseconds.

**Floor** How much signal still leaks through when shut. Zero is a hard gate; raise it for an expander feel.

**Duck** Swaps open and shut, which turns any of the three sources into a ducker.

**Open** The manual switch, used when Source is SWITCH. Cord a Slider, LFO or Follower onto it to flip the gate by control.

## Recipe

**Chopped pad** Cord a pad into the first inlet and a Kick into the key inlet. Source LEVEL, Detector PEAK, Threshold 0.2, AttackTime 1, HoldTime 60, ReleaseTime 80, Floor 0. The pad now sounds only on each kick; turn Duck on to have it drop out on each kick instead.

## Related Organisms

NoiseGate, SideChain, Gain, Follower
