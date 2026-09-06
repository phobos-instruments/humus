# Slider

A fader as a patchable object: one big handle whose position becomes a signal. The outlet carries the value mapped between Min and Max as a steady level (summed onto anything at the inlet), and the raw 0..1 position is published as the control value "value", so any parameter in the patch can ride it through a control route.

Map a hardware fader or knob to Value via MIDI learn and the Slider becomes a physical macro: one hand movement steering a filter sweep, a send level and a delay feedback at once, each through its own route with its own range. Give a route a switch shape and the Slider turns into a trigger - push past the threshold to flip a Bypass, fire a pad, or open a Gate.

## Parameters

**Value** the handle. 0..1, smoothed on its way to the output.

**Min / Max** the range the outlet travels. Min above Max is allowed and inverts the throw.

**Log scale** makes the travel geometric instead of linear - each equal move multiplies rather than adds, which is the natural feel for frequencies and rates (20 to 20000 with the midpoint at 632, not 10010). Both ends must be above zero; otherwise the slider quietly stays linear, the same rule control routes follow.

**Slew** how long the output takes to catch up with a move, in milliseconds. Zero snaps; long settings turn jumps into glides.

## Related Organisms

Number, VCA, LFO, Gate
