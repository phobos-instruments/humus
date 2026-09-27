# VideoMix

An A/B crossfader for two video feeds, or a channel mixer for up to eight.

Inputs sets how many feeds it takes. At 2 it is a crossfader: inlet 1 is A and inlet 2 is B, and the outlet carries the dissolve. Fade at 0 shows A, at 1 shows B, and anywhere between mixes the two under the law Curve sets. Fade is an ordinary param: map a controller to it for a hardware crossfader, put a synced LFO on it so the two feeds trade places every sixteen beats, or draw it in an automation lane so the cut list is part of the song. Crossfade two whole Lumen composites before the VideoOut, or pick more inputs for a channel mixer.

From 3 inputs up it becomes a channel mixer with one vertical fader per inlet. The channels stack in order, channel 1 at the bottom, and each fader is how strongly that channel covers the ones below it: at the top it hides them, halfway it shows through, at the bottom it drops out. Channel 1 starts up and the rest start down, so raising a fader brings its feed in over the picture. Cords stay on their inlets when you change the count; a cord on an inlet the new size does not have moves to a free one or is dropped.

## Parameters

**Inputs** How many feeds, from 2 to 8. Two is the crossfader below; more is the channel mixer.

**Level 1 to 8** The channel mixer's faders, one per inlet: how strongly that channel covers the channels under it.

**Fade** The dissolve. 0 is all A (inlet 1), 1 is all B (inlet 2), and between is a mix shaped by Curve.

**Curve** The crossfader's law of the dissolve, the same knob as on Crossfader. At 0 it is linear, so halfway through both pictures sit at half brightness and the frame dims. Higher keeps each side bright further across the fade; two thirds is the equal-power law, and 1 is the hard version.

## Recipe

**Bar-synced swap** Cord a VideoPlayer into inlet 1, a CameraIn into inlet 2 and the outlet into a VideoOut. Set Curve to two thirds, then right-click Fade, choose Modulate with LFO, and sync the LFO to 8 beats with a square wave so the feeds swap on the bar.

**Four feeds, one screen** Set Inputs to 4 and cord a CameraIn, two VideoPlayers and a VideoPad into inlets 1 to 4. Leave channel 1 up as the base, then map faders 2 to 4 to three hardware faders and push each one up to cut its feed in over the rest.

## Related Organisms

Lumen, VideoPlayer, VideoOut
