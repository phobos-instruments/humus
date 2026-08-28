# DigiRust

A lo-fi channel: the sound of a cheap converter slowly going to rust. Bits coarsens the signal into fewer and fewer levels, Rate holds each sample longer so the top end aliases down into grit, and the rest of the panel models the machine around the converter rather than the math alone.

## The converter

Bits is the word length, stepped like a real converter, from a clean 16 down to a 1-bit growl. Rate is the sampling clock in hertz: parked at the top it is out of the path, pulled down it holds each sample longer, the way early samplers did. Jitter wobbles that clock so the hold lengths vary sample to sample - the warble and smear of a converter running on a bad crystal. Noise adds a hiss that rides the signal and ducks out with silence, so an idle patch stays quiet.

## Around it

Tone is a tilt: below centre it fades toward a dark one-pole lowpass, above centre it pushes the presence back in. Mix blends the rusted signal against the dry input for parallel grit, and Level sets the output.

The dice rolls Bits, Rate, Jitter, Noise and Tone; Mix and Level stay where you put them, so a roll changes the colour of the rust, never the balance of the patch.

## Parameters

**Bits** word length, 1 to 16, in whole steps. Lower is coarser.

**Rate** sampling clock in hertz. Top of travel is off; low is crunchy.

**Jitter** clock wobble - warble and smear. 0 is a steady clock.

**Noise** signal-ducked hiss floor. 0 is silent.

**Tone** dark to present tilt around the middle.

**Mix** dry to rusted blend.

**Level** output level.

## Signal flow

Input -> bit quantize + jittered sample hold -> noise -> tone tilt -> Mix against dry -> Level -> Output. Stereo in, stereo out.

## Related Organisms

Sampler, Fern, Filter
