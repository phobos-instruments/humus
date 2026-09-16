# NoiseGate

A gate that passes sound while it is loud enough and shuts when it is not, or ducks it the other way round.

It removes the hiss, hum and spill that live in the gaps between phrases, tightens a drum by cutting its tail short, and in Duck mode holds the sound down while it crosses the threshold. The Threshold control has two handles because the gate has two levels: the upper opens it, the lower closes it, and keeping them apart stops the gate chattering on a signal that hovers at the line. Detection is linked across channels, so it never chops one side out of the image. There is no key input: to gate one sound from the rhythm of another, use SideChain. Cord it after a SoundIn or before a Compressor.

## Parameters

**Mode** Off passes loud sounds and attenuates quiet ones. Duck does the opposite, holding the sound down while it crosses the threshold.

**InputGain** Trims the incoming sound before detection, so material can be driven over the threshold without moving it.

**Threshold** The open level on the upper handle and the close level on the lower. Keep them apart to stop chattering.

**Range** How far the sound is attenuated while the gate is closed. At the bottom nothing passes; a little way up leaves the gaps quiet rather than empty.

**AttackTime** How quickly the gate opens, in milliseconds. Fast enough not to blunt a note, slow enough not to click.

**HoldTime** How long the gate stays open after the signal falls below the close level, in milliseconds. It lets a drum keep its body before the gate shuts.

**ReleaseTime** How quickly the gate closes once the hold has expired, in milliseconds. Short is abrupt and rhythmic, long fades the tail.

## Recipe

**Tight snare** Cord the snare in. Threshold open 0.3 and close 0.15, Range 0, AttackTime 0.5, HoldTime 40, ReleaseTime 60. Lower HoldTime until the tail is cut short, and raise Range a little if the gaps sound empty.

## Related Organisms

SideChain, Compressor, SideKick, TransientShaper
