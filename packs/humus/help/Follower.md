# Follower

An envelope follower that turns the loudness of a signal into a control value and a gate.

It rectifies the audio at its inlet and smooths it into a contour, published as the control value "env" on the first outlet, around 0 to 1 for full-scale audio and scaled by Gain. A second outlet and the control value "gate" sit at 0 until the contour crosses Threshold, then hold at 1 until the level has fallen below half the threshold and Hold has run out. Each opening also sends Note from the MIDI outlet, with a velocity taken from the level of the hit. Cord env onto a Gain for a duck or expander, onto a Filter's Frequency for an auto-wah, or the MIDI outlet into a VideoPad to cut pictures on the beat. For finished single-organism dynamics, see SideChain and the Compressor family.

## Parameters

**Attack** How quickly the contour rises into a hit, in milliseconds. Short catches transients, long smooths them into a level.

**Release** How slowly the contour falls afterwards, which decides how long a sound keeps the gate open once it stops.

**Gain** Scales the contour so its travel matches whatever it drives.

**Threshold** The level the contour must reach to open the gate. 0 switches the gate off.

**Hold** The least time the gate stays open once a hit has opened it, in milliseconds, so one drum does not chatter into several notes.

**Note** The MIDI note sent on each opening of the gate.

## Recipe

**Kick-driven cuts** Cord a kick into the Follower, Attack 2, Release 150, Threshold 0.3, Hold 100. Cord its MIDI outlet into a VideoPad and every kick fires the pad; cord env onto the Gain of a pad synth at the same time and the synth ducks under each hit.

## Related Organisms

Gain, SideChain, Filter, Button
