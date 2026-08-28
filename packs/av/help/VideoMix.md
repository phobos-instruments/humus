# VideoMix

The A/B crossfader. Two video inlets - inlet 1 is A, inlet 2 is B - and one outlet carrying the dissolve: Fade at 0 shows A, at 1 shows B, anywhere between a linear mix.

Fade is an ordinary param, which is the whole trick: map a MIDI controller to it and you have a hardware video crossfader; put an LFO modulation route on it (synced, 16 beats) and the two feeds trade places on a musical cycle; draw it in an automation lane and the cut list is part of the song.

Chain them for more buses (A/B into C/D), or crossfade two whole Lumen composites for scene-to-scene transitions before the VideoOut.

## Parameters

**Fade** the dissolve. 0 is all A (inlet 1), 1 is all B (inlet 2), anywhere between is a linear mix.

## Related Organisms

Lumen, VideoPlayer, VideoOut
