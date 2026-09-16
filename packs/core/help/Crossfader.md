# Crossfader

A stereo crossfade between two sources on one control.

Inlets 1-2 are side A and inlets 3-4 are side B. Fade all the way down passes A, all the way up passes B, and anywhere between blends the two under the law that Curve chooses. Cord two Deck organisms, two loops, or a dry and a processed copy of one signal into it, and cord the outlet on to a Mixer or SoundOut. Match the two sides with TrimA and TrimB before touching Curve: a fade that lurches is usually two sources at different levels rather than the wrong law. Fade can also drive other parameters through Parameter Control, so one sweep can open a filter as it crosses.

## Parameters

**MasterGain** Output level after the fade and the trims.

**TrimA** Level of side A before the fade, so the two sources can be matched first.

**TrimB** The same for side B.

**Fade** The crossfade position. Down is A, up is B.

**Curve** The fade law. Zero is a straight line, right for a dry and a processed copy of one sound; about two thirds is constant power, right for two unrelated sources; one is a fast cut where a small move near either end swaps the sound outright.

**CutA** Held, the mix jumps to side A whatever Fade says, and returns to Fade on release. Holding A and B together parks the fade at its middle.

**CutB** The same for side B.

**CutTime** How long a cut takes to land, in milliseconds. Zero snaps; a few milliseconds takes the click out of a sustained bass.

## Recipe

**Two-deck blend** Cord one Deck into inlets 1-2 and another into 3-4, set Curve to about 0.67, and adjust TrimA and TrimB until each deck reads the same on the Mixer meter with Fade at either end. Map CutA and CutB to two pads and set CutTime to about 5 to chop the incoming track over the outgoing one without clicks.

## Related Organisms

Mixer, Gain, Console, Send
