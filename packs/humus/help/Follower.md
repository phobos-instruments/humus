# Follower

An envelope follower as a patchable object: rectifies the audio at its inlet and emits its loudness contour as a control signal - around 0..1 for full-scale audio, scaled by Gain. Attack sets how fast it rises into a hit, Release how slowly it lets go.

The modular route to dynamics: patch a drum bus into a Follower and its output into a VCA's control inlet and you have a gate/expander; invert the sense with a Number (1) minus the Follower (via chaining) and the VCA ducks instead. A Follower into a Filter's Frequency- driving path is the classic auto-wah move.

For the finished, single-object versions of these patches, see the Dynamics category (SideChain, the Compressor family).

## Parameters

**Attack** how quickly the output rises into a hit. Short catches transients; long smooths them into a level.

**Release** how slowly it lets go afterwards, which decides how long a sound keeps holding a gate open once it stops.

**Gain** scales the contour, for matching the follower's travel to whatever it drives.

## Related Organisms

VCA, SideChain, Filter
