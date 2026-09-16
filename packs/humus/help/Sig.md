# Sig

Turns a control value into an audio signal, ramped so it never clicks.

Cords carry sound and numbers land on knobs; Sig is the one organism that sends a number back out as a cord, for the few places that need one. Most of the time a Follower, a sensor or an LFO goes straight onto a knob through a control cord, but a Number that has to sit inside a Math formula as a signal, a Slider driving a control-voltage output through an AuxOut with its DC guard off, or a constant added to a cord all need Sig. Whatever lands on its Value socket leaves the outlet as a steady signal.

## Parameters

**Value** The number to turn into a signal. Cord anything onto it, or type it.

**Slew** How long a change takes to arrive, in milliseconds. At 0 a change lands in one sample; the 5 ms default rounds off steps without noticeable lag.

## Recipe

**Offset into Math** Cord a Slider onto Sig's Value and Sig's outlet into Math's b inlet, with the audio on a. Write a+b in Math and the Slider adds a DC offset to the signal; set Slew to 200 so moving the fader glides rather than steps.

## Related Organisms

Number, Slider, Follower, Math
