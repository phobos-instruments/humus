# Number

A value object in the patching tradition: emits its Value as a constant control signal from its outlet, added to whatever arrives at its inlet - so Numbers offset signals and chain with each other.

Integer mode rounds the output to whole numbers (the int/float datatype switch); Float mode glides to new values over a few milliseconds so a Number driving a gain never clicks.

Because Value is an ordinary parameter, everything that drives parameters drives a Number: draw an automation lane on it, map a MIDI controller to it (Parameter Control), or morph it from the Metapad. That makes Number the bridge between MIDI/automation and anything with an audio inlet - including a SerialOut feeding a microcontroller.

## Parameters

**Value** the constant this emits, added to whatever arrives at the inlet.

**Integer** rounds the output to whole numbers. Off, the value glides to a new setting over a few milliseconds, so a Number driving a gain cannot click.

## Related Organisms

SerialOut, VCA, LFO
