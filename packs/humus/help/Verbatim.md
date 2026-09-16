# Verbatim

A convolution reverb that plays your signal through a recorded impulse response.

Drop or pick an impulse file in the strip at the top and everything corded in happens inside that recording: a stairwell, a spring tank or a hall. Any audio file works as the impulse, up to fifteen seconds of it; files made for the purpose sound like rooms rather than notes. The impulse is normalised on load, so different files land at a comparable level and Mix keeps its meaning. The wet path runs through Predelay, then Damp, then LowCut. Cord a Sampler, a Wave or a whole Mixer bus in, or put it on a Send return.

## Parameters

**File** The impulse response to play through. Any audio file loads; the first fifteen seconds are used.

**Mix** Dry against wet, equal-power. Full left is the untouched signal, full right is only the room.

**Predelay** How long the room waits before answering, up to a quarter second. Pushes the space back behind the direct sound.

**LowCut** Trims the wet low end, where long rooms turn to mud. At minimum it only steadies the very bottom.

**Damp** A lowpass on the wet path. Pull it down and the room darkens, as if the walls grew softer.

**Reverse** Plays the impulse backwards, so the room swells into the note instead of decaying after it.

## Recipe

**Send hall** On a Send return, Mix full right, Predelay 20, LowCut 150, Damp 6000. Load a hall impulse, cord vocals and pads to the Send, and keep drums off it so the low end stays dry.

## Related Organisms

Fern, SoundSpace, Bloom
