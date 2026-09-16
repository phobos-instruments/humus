# VideoMix

An A/B crossfader for two video feeds.

Inlet 1 is A and inlet 2 is B; the outlet carries the dissolve. Fade at 0 shows A, at 1 shows B, and anywhere between mixes the two under the law Curve sets. Fade is an ordinary param: map a controller to it for a hardware crossfader, put a synced LFO on it so the two feeds trade places every sixteen beats, or draw it in an automation lane so the cut list is part of the song. Chain two for more buses, or crossfade two whole Lumen composites before the VideoOut.

## Parameters

**Fade** The dissolve. 0 is all A (inlet 1), 1 is all B (inlet 2), and between is a mix shaped by Curve.

**Curve** The law of the dissolve, the same knob as on Crossfader. At 0 it is linear, so halfway through both pictures sit at half brightness and the frame dims. Higher keeps each side bright further across the fade; two thirds is the equal-power law, and 1 is the hard version.

## Recipe

**Bar-synced swap** Cord a VideoPlayer into inlet 1, a CameraIn into inlet 2 and the outlet into a VideoOut. Set Curve to two thirds, then right-click Fade, choose Modulate with LFO, and sync the LFO to 8 beats with a square wave so the feeds swap on the bar.

## Related Organisms

Lumen, VideoPlayer, VideoOut
