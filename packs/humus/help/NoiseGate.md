# NoiseGate

The doorway. A gate passes sound while it is loud enough to be worth hearing and shuts when it is not, which removes the hiss, hum, spill and room tone that live in the gaps. What you notice is not the noise leaving; it is the silence arriving, and how much room that silence leaves for everything else.

Use it to clean up a recorded part between phrases, to tighten a drum by cutting its tail short, or - in Duck mode with a fast release - as a rhythmic effect rather than a repair.

## Two thresholds

The Threshold control has two handles because the gate has two levels: the upper is where it opens, the lower where it closes. Setting them apart is what stops a gate chattering, since a signal hovering right at the line would otherwise open and shut many times a second. Open high, close low, and a sound has to genuinely fall away before the door moves.

## Parameters

**Mode** Gate passes loud sounds and attenuates quiet ones - the usual way round, and the one that cleans up a signal. Duck does the opposite, holding a bed down under anything that crosses the threshold. The header dropdown switches stereo and mono in place; detection is linked across channels either way, so the gate cannot chop one side out of the image.

**Threshold** the open level (upper handle) and the close level (lower handle). Keep them apart to stop chattering.

**Range** how much the sound is attenuated while the gate is closed. At the bottom of its travel nothing passes at all; a little way up leaves the gaps quiet rather than empty, which is usually more natural.

**AttackTime** how quickly the gate opens. Fast enough not to blunt the start of a note, slow enough not to click.

**HoldTime** how long the gate stays open after the signal falls below the close threshold. The other half of the anti-chatter story, and what lets a drum keep its body before the gate shuts.

**ReleaseTime** how quickly the gate closes once the hold has expired. Short is abrupt and rhythmic, long fades the tail out.

**InputGain** trims the incoming sound before detection, so you can drive material over the threshold without moving it.

## Notes

There is no separate key input: the gate opens and closes on the sound passing through it. To gate one sound from the rhythm of a different one, use SideChain, whose key inlet exists for exactly that.

## Related Organisms

SideChain, Compressor, SideKick, TransientShaper
