# Follower

An envelope follower as a patchable object: rectifies the audio at its inlet and emits its loudness contour as a control signal - around 0..1 for full-scale audio, scaled by Gain. Attack sets how fast it rises into a hit, Release how slowly it lets go.

The modular route to dynamics: patch a drum bus into a Follower and its output into a VCA's control inlet and you have a gate/expander; invert the sense with a Number (1) minus the Follower (via chaining) and the VCA ducks instead. A Follower into a Filter's Frequency- driving path is the classic auto-wah move.

For the finished, single-object versions of these patches, see the Dynamics category (SideChain, the Compressor family).

## The sensor

The second outlet is a sensor: it sits at 0 until the contour crosses Threshold, then holds at 1 until the sound has fallen back below half the threshold and Hold has run out. The same on/off is published as the control value "gate", so a cut button, a clip launcher or a toggle can follow it through a control route, and each opening sends Note from the MIDI outlet, at a velocity taken from how hard the hit landed, with the note off when the gate closes. Patch a kick into a Follower and its MIDI outlet into a VideoPad or a VideoPlayer and the picture cuts on every beat.

## Parameters

**Attack** how quickly the output rises into a hit. Short catches transients; long smooths them into a level.

**Release** how slowly it lets go afterwards, which decides how long a sound keeps holding a gate open once it stops.

**Gain** scales the contour, for matching the follower's travel to whatever it drives.

**Threshold** the level the contour must reach to open the sensor. 0 switches the sensor off.

**Hold** the least time the sensor stays open once a hit has opened it, in milliseconds, so one drum does not chatter into several notes.

**Note** the MIDI note the sensor sends on each opening.

## Related Organisms

VCA, SideChain, Filter, Button, VideoPad
