# Limiter

A brickwall limiter with instant attack: nothing passes the ceiling.

Threshold is where limiting begins and Ceiling is where the output may reach, and the output is scaled so a sound arriving at the threshold leaves at the ceiling. Lowering Threshold therefore makes the result louder and more limited at once, which is the knob to ride; Ceiling is the safety line set once. Attack is instant and a hard clamp at the ceiling backs it up, so everything expressive lives in the release. Detection is linked across channels, so limiting never tilts the stereo image. Cord it last, after a Console or a Mixer and before the SoundOut.

## Parameters

**InputGain** Trims the incoming sound before limiting.

**Threshold** Where limiting begins. Lower it for more limiting and a louder result.

**Ceiling** The highest level allowed out. Leave a little headroom below full scale for whatever comes next.

**HoldTime** How long full reduction is held before release starts, in milliseconds. A little stops chatter on peaks in quick succession.

**ReleaseTime** How quickly the limiter recovers after a peak, in milliseconds. Short holds loudness but can pump on dense material; long stays clean and gives up some level.

## Recipe

**Master ceiling** Cord the Console's outlet into the Limiter and the Limiter into the SoundOut. Ceiling 0.95, ReleaseTime 150, HoldTime 10, then lower Threshold until the loudest section shows a few dB of reduction and no more.

## Related Organisms

Compressor, Console, VuMeter
