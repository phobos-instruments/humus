# Limiter

The ceiling. A limiter is a compressor that has stopped negotiating: nothing gets past the line, ever. That absolute quality is the point - once you know the sound cannot exceed a level, you can push everything up against that level without listening for the one peak that would have clipped.

Use it at the end of a chain, on a master bus, or anywhere a signal is about to leave for somewhere less forgiving.

## The two levels

Threshold is where limiting begins; Ceiling is where the output is allowed to reach, and they do different jobs. The organism scales its output so a sound arriving at the threshold leaves at the ceiling, so lowering Threshold does not make the result quieter - it makes it louder and more limited at once. That is the knob you reach for; Ceiling is the safety line you set once and leave.

## Parameters

**Mode** switches stereo and mono in place. Detection is linked across channels either way: one gain for both sides, so limiting cannot tilt the stereo image.

**Threshold** where limiting begins. Lower it for more limiting and, because of the scaling above, a louder result.

**Ceiling** the highest level allowed out. Leave a little headroom below full scale for whatever comes next in the chain.

**ReleaseTime** how quickly the limiter recovers after a peak. Short releases hold loudness but can pump audibly on dense material; long ones stay clean and give up some level.

**HoldTime** how long full reduction is held before release starts. A little of this stops the limiter chattering on material with peaks arriving in quick succession.

**InputGain** trims the incoming sound before limiting.

## How it works

Attack is instant and not adjustable, which is what separates a limiter from a fast compressor. A compressor with a very short attack still lets the first fraction of a peak through while it responds; this does not, so the ceiling means what it says. Everything expressive about a limiter therefore lives in the release.

## Related Organisms

Compressor, Console, VuMeter
